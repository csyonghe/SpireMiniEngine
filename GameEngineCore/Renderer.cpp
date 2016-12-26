#include "Renderer.h"
#include "HardwareRenderer.h"

#include "Engine.h"
#include "SkeletalMeshActor.h"
#include "CoreLib/Graphics/TextureFile.h"
#include "CoreLib/Imaging/Bitmap.h"
#include "CoreLib/Imaging/TextureData.h"
#include "CoreLib/WinForm/Debug.h"

#include "Spire/Spire.h"
#include "TextureCompressor.h"
#include <fstream>

#include "DeviceMemory.h"
#include "WorldRenderPass.h"
#include "PostRenderPass.h"

using namespace CoreLib;
using namespace VectorMath;

namespace GameEngine
{
	int Align(int ptr, int alignment)
	{
		int m = ptr % alignment;
		if (m)
		{
			int padding = alignment - m;
			return ptr + padding;
		}
		return ptr;
	}
	
	IRenderProcedure * CreateStandardRenderProcedure();

	class RendererImpl : public Renderer
	{
		class RendererServiceImpl : public RendererService
		{
		private:
			RendererImpl * renderer;
			RefPtr<Drawable> CreateDrawableShared(Mesh * mesh, Material * material)
			{
				auto sceneResources = renderer->sceneRes.Ptr();
				RefPtr<Drawable> rs = new Drawable(sceneResources);
				rs->mesh = sceneResources->LoadDrawableMesh(mesh).Ptr();
				rs->material = material;
				return rs;
			}
		public:
			RendererServiceImpl(RendererImpl * pRenderer)
				: renderer(pRenderer)
			{}
			ModuleInstance * CreateTransformModuleInstance(const char * name, int uniformBufferSize)
			{
				auto sceneResources = renderer->sceneRes.Ptr();
				return renderer->sharedRes.CreateModuleInstance(spFindModule(renderer->sharedRes.spireContext, name), &sceneResources->transformMemory, uniformBufferSize);
			}

			virtual CoreLib::RefPtr<Drawable> CreateStaticDrawable(Mesh * mesh, Material * material) override
			{
				RefPtr<Drawable> rs = CreateDrawableShared(mesh, material);
				rs->type = DrawableType::Static;
				rs->transformModule = CreateTransformModuleInstance("NoAnimation", (int)(sizeof(Vec4) * 7));
				rs->pipelineInstances.SetSize(renderer->worldRenderPasses.Count());
				for (int i = 0; i < renderer->worldRenderPasses.Count(); i++)
				{
					rs->pipelineInstances[i] = renderer->worldRenderPasses[i]->CreatePipelineStateObject(renderer->sceneRes.Ptr(), material, mesh, rs->type);
				}
				return rs;
			}
			virtual CoreLib::RefPtr<Drawable> CreateSkeletalDrawable(Mesh * mesh, Skeleton * skeleton, Material * material) override
			{
				RefPtr<Drawable> rs = CreateDrawableShared(mesh, material);
				rs->type = DrawableType::Skeletal;
				rs->skeleton = skeleton;
				int poseMatrixSize = skeleton->Bones.Count() * (sizeof(Vec4) * 7);
				rs->transformModule = CreateTransformModuleInstance("SkeletalAnimation", poseMatrixSize);
				rs->pipelineInstances.SetSize(renderer->worldRenderPasses.Count());

				for (int i = 0; i < renderer->worldRenderPasses.Count(); i++)
				{
					rs->pipelineInstances[i] = renderer->worldRenderPasses[i]->CreatePipelineStateObject(renderer->sceneRes.Ptr(), material, mesh, rs->type);
				}
				return rs;
			}
		};
	private:
		RendererSharedResource sharedRes;
		RefPtr<SceneResource> sceneRes;
		RefPtr<RendererServiceImpl> renderService;
		RefPtr<IRenderProcedure> renderProcedure;
		List<RefPtr<WorldRenderPass>> worldRenderPasses;
		List<RefPtr<PostRenderPass>> postRenderPasses;
		List<RenderPassInstance> renderPassInstances;
		List<PostRenderPass*> postPassInstances;
		HardwareRenderer * hardwareRenderer = nullptr;
		RenderStat renderStats;
		Level* level = nullptr;

		int uniformBufferAlignment = 256;
		int storageBufferAlignment = 32;
	private:
		void RunRenderProcedure()
		{
			if (!level) return;
			renderPassInstances.Clear();
			postPassInstances.Clear();
			RenderProcedureParameters params;
            Array<CameraActor*, 8> cameras;
            cameras.Add(level->CurrentCamera.Ptr());
			params.level = level;
			params.renderer = this;
            params.cameras = cameras.GetArrayView();
			params.rendererService = renderService.Ptr();
			renderProcedure->Run(renderPassInstances, postPassInstances, params);
		}
	public:
		RendererImpl(WindowHandle window, RenderAPI api)
			: sharedRes(api)
		{
			switch (api)
			{
			case RenderAPI::Vulkan:
				hardwareRenderer = CreateVulkanHardwareRenderer(Engine::Instance()->GpuId);
				break;
			case RenderAPI::OpenGL:
				hardwareRenderer = CreateGLHardwareRenderer();
				break;
			}

			hardwareRenderer->BindWindow(window, 640, 480);

			sharedRes.Init(hardwareRenderer);
			
			// Fetch uniform buffer alignment requirements
			uniformBufferAlignment = hardwareRenderer->UniformBufferAlignment();
			storageBufferAlignment = hardwareRenderer->StorageBufferAlignment();
			
			sceneRes = new SceneResource(&sharedRes, sharedRes.spireContext);
			renderService = new RendererServiceImpl(this);

			renderProcedure = CreateStandardRenderProcedure();
			renderProcedure->Init(this);
		}
		~RendererImpl()
		{
			renderProcedure = nullptr;
			sceneRes = nullptr;
			sharedRes.Destroy();
		}

		virtual void Wait() override
		{
			hardwareRenderer->Wait();
		}

		virtual int RegisterWorldRenderPass(WorldRenderPass * renderPass) override
		{
			if (worldRenderPasses.Count() >= MaxWorldRenderPasses)
				throw InvalidOperationException("Number of registered world render passes exceeds engine limit.");
			renderPass->Init(&sharedRes);
			worldRenderPasses.Add(renderPass);
			renderPass->SetId(worldRenderPasses.Count() - 1);
			return worldRenderPasses.Count() - 1;
		}

		virtual int RegisterPostRenderPass(PostRenderPass * renderPass) override
		{
			if (postRenderPasses.Count() >= MaxPostRenderPasses)
				throw InvalidOperationException("Number of registered post render passes exceeds engine limit.");
			renderPass->Init(&sharedRes);
			postRenderPasses.Add(renderPass);
			return postRenderPasses.Count() - 1;
		}

		virtual HardwareRenderer * GetHardwareRenderer() override
		{
			return hardwareRenderer;
		}

		virtual void InitializeLevel(Level* pLevel) override
		{
			if (!pLevel) return;

			sceneRes->Clear();
			level = pLevel;
		}
		virtual void TakeSnapshot() override
		{
			if (!level)
				return;
			RunRenderProcedure();
		}
		virtual RenderStat GetStats() override
		{
			return renderStats;
		}
		virtual void RenderFrame() override
		{
			if (!level) return;
			renderStats.NumDrawCalls = 0;
			renderStats.NumPasses = 0;
			for (auto & pass : renderPassInstances)
			{
				hardwareRenderer->ExecuteCommandBuffers(pass.renderOutput->GetFrameBuffer(), MakeArrayView(pass.commandBuffer));
				renderStats.NumPasses++;
				renderStats.NumDrawCalls += pass.numDrawCalls;
			}

			for (auto pass : postPassInstances)
				pass->Execute();
		}
		virtual RendererSharedResource * GetSharedResource() override
		{
			return &sharedRes;
		}
		virtual SceneResource * GetSceneResource() override
		{
			return sceneRes.Ptr();
		}
		virtual void DestroyContext() override
		{
			sceneRes = nullptr;
		}
		virtual void Resize(int w, int h) override
		{
			sharedRes.Resize(w, h);
			renderProcedure->ResizeFrame(w, h);
			
			hardwareRenderer->Resize(w, h);
		}
		Texture2D * GetRenderedImage()
		{
			return renderProcedure->GetOutput()->Texture.Ptr();
		}
	};

	Renderer* CreateRenderer(WindowHandle window, RenderAPI api)
	{
		return new RendererImpl(window, api);
	}
}