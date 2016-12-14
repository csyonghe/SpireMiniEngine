#include "Renderer.h"
#include "HardwareRenderer.h"
#include "HardwareApiFactory.h"

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

	template<typename T>
	void use(const T &) {}

	Drawable::~Drawable()
	{
		if (uniforms.instanceUniform)
			scene->instanceUniformMemory.Free(uniforms.instanceUniform, uniforms.instanceUniformCount);
		if (uniforms.transformUniform)
		{
			if (type == DrawableType::Static)
				scene->staticTransformMemory.Free(uniforms.transformUniform, uniforms.transformUniformCount);
			else
				scene->skeletalTransformMemory.Free(uniforms.transformUniform, uniforms.transformUniformCount);
		}
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

				int ptr = 0;
				material->FillInstanceUniformBuffer([&](const String &)
				{
				},
					[&](auto & val)
				{
					use(val);
					ptr += sizeof(val);
				},
					[&](int alignment)
				{
					ptr = GameEngine::Align(ptr, alignment);
				}
				);

				rs->uniforms.instanceUniform = (unsigned char*)sceneResources->instanceUniformMemory.Alloc(ptr);
				rs->uniforms.instanceUniformCount = ptr;

				return rs;
			}
		public:
			RendererServiceImpl(RendererImpl * pRenderer)
				: renderer(pRenderer)
			{}
			virtual CoreLib::RefPtr<Drawable> CreateStaticDrawable(Mesh * mesh, Material * material) override
			{
				auto sceneResources = renderer->sceneRes.Ptr();

				RefPtr<Drawable> rs = CreateDrawableShared(mesh, material);
				rs->type = DrawableType::Static;

				rs->uniforms.transformUniform = (unsigned char*)sceneResources->staticTransformMemory.Alloc(sizeof(Vec4) * 7);
				rs->uniforms.transformUniformCount = sizeof(Vec4) * 7;

				rs->pipelineInstances.SetSize(renderer->worldRenderPasses.Count());
				for (int i = 0; i < renderer->worldRenderPasses.Count(); i++)
				{
					rs->pipelineInstances[i] = renderer->worldRenderPasses[i]->CreatePipelineStateObject(material, mesh, rs->uniforms, rs->type);
				}
				return rs;
			}
			virtual CoreLib::RefPtr<Drawable> CreateSkeletalDrawable(Mesh * mesh, Skeleton * skeleton, Material * material) override
			{
				auto sceneResources = renderer->sceneRes.Ptr();

				RefPtr<Drawable> rs = CreateDrawableShared(mesh, material);
				rs->type = DrawableType::Skeletal;
				rs->skeleton = skeleton;

				int poseMatrixSize = skeleton->Bones.Count() * (sizeof(Vec4) * 7);

				rs->uniforms.transformUniform = (unsigned char*)sceneResources->skeletalTransformMemory.Alloc(poseMatrixSize);
				rs->uniforms.transformUniformCount = poseMatrixSize;

				rs->pipelineInstances.SetSize(renderer->worldRenderPasses.Count());
				for (int i = 0; i < renderer->worldRenderPasses.Count(); i++)
				{
					rs->pipelineInstances[i] = renderer->worldRenderPasses[i]->CreatePipelineStateObject(material, mesh, rs->uniforms, rs->type);
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
		HardwareRenderer * hardwareRenderer = nullptr;
		Level* level = nullptr;

		int uniformBufferAlignment = 256;
		int storageBufferAlignment = 32;
	private:
		void RunRenderProcedure()
		{
			if (!level) return;
			renderPassInstances.Clear();
			RenderProcedureParameters params;
            Array<CameraActor*, 8> cameras;
            cameras.Add(level->CurrentCamera.Ptr());
			params.level = level;
			params.renderer = this;
            params.cameras = cameras.GetArrayView();
			params.rendererService = renderService.Ptr();
			renderProcedure->Run(renderPassInstances, params);
		}
	public:
		RendererImpl(WindowHandle window, RenderAPI api)
			: sharedRes(api)
		{
			HardwareApiFactory * hwFactory = nullptr;
			switch (api)
			{
			case RenderAPI::Vulkan:
				hwFactory = CreateVulkanFactory();
				break;
			case RenderAPI::OpenGL:
				hwFactory = CreateOpenGLFactory();
				break;
			}
			hardwareRenderer = hwFactory->CreateRenderer(Engine::Instance()->GpuId);
			hardwareRenderer->BindWindow(window, 640, 480);

			sharedRes.Init(hwFactory, hardwareRenderer);
			
			// Fetch uniform buffer alignment requirements
			uniformBufferAlignment = hardwareRenderer->UniformBufferAlignment();
			storageBufferAlignment = hardwareRenderer->StorageBufferAlignment();
			
			sceneRes = new SceneResource(&sharedRes);
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
			renderPass->Init(&sharedRes, sceneRes.Ptr());
			worldRenderPasses.Add(renderPass);
			renderPass->SetId(worldRenderPasses.Count() - 1);
			return worldRenderPasses.Count() - 1;
		}

		virtual int RegisterPostRenderPass(PostRenderPass * renderPass) override
		{
			if (postRenderPasses.Count() >= MaxPostRenderPasses)
				throw InvalidOperationException("Number of registered post render passes exceeds engine limit.");
			renderPass->Init(&sharedRes, sceneRes.Ptr());
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
		virtual void RenderFrame() override
		{
			if (!level) return;

			for (auto & pass : renderPassInstances)
			{
				if (pass.viewUniformSize)
					sharedRes.SetViewUniformData(pass.viewUniformPtr, pass.viewUniformSize);
				hardwareRenderer->ExecuteCommandBuffers(pass.renderOutput->GetFrameBuffer(), MakeArrayView(pass.commandBuffer));
			}

			for (auto & post : postRenderPasses)
			{
				post->Execute();
			}
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
			for (auto & post : postRenderPasses)
				post->RecordCommandBuffer(w, h);
			
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