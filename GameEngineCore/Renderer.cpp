#include "Renderer.h"
#include "HardwareRenderer.h"
#include "Level.h"
#include "Engine.h"
#include "SkeletalMeshActor.h"
#include "CoreLib/Graphics/TextureFile.h"
#include "CoreLib/Imaging/Bitmap.h"
#include "CoreLib/Imaging/TextureData.h"
#include "CoreLib/WinForm/Debug.h"
#include "LightProbeRenderer.h"
#include "Spire/Spire.h"
#include "TextureCompressor.h"
#include <fstream>

#include "DeviceMemory.h"
#include "WorldRenderPass.h"
#include "PostRenderPass.h"
#include "RenderProcedure.h"

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

	class RendererImpl : public Renderer
	{
		class RendererServiceImpl : public RendererService
		{
		private:
			RendererImpl * renderer;
			RefPtr<Drawable> CreateDrawableShared(Mesh * mesh, Material * material, bool cacheMesh)
			{
				auto sceneResources = renderer->sceneRes.Ptr();
				RefPtr<Drawable> rs = new Drawable(sceneResources);
                if (cacheMesh)
                    rs->mesh = sceneResources->LoadDrawableMesh(mesh);
                else
                    rs->mesh = sceneResources->CreateDrawableMesh(mesh);
				rs->material = material;
				return rs;
			}
		public:
			RendererServiceImpl(RendererImpl * pRenderer)
				: renderer(pRenderer)
			{}
			void CreateTransformModuleInstance(ModuleInstance & rs, const char * name, int uniformBufferSize)
			{
				auto sceneResources = renderer->sceneRes.Ptr();
				renderer->sharedRes.CreateModuleInstance(rs, spFindModule(renderer->sharedRes.spireContext, name), &sceneResources->transformMemory, uniformBufferSize);
			}

			virtual CoreLib::RefPtr<Drawable> CreateStaticDrawable(Mesh * mesh, Material * material, bool cacheMesh) override
			{
				if (!material->MaterialPatternModule)
					renderer->sceneRes->RegisterMaterial(material);
				RefPtr<Drawable> rs = CreateDrawableShared(mesh, material, cacheMesh);
				rs->type = DrawableType::Static;
				CreateTransformModuleInstance(*rs->transformModule, "NoAnimation", (int)(sizeof(Vec4) * 4));
				rs->vertFormat = mesh->GetVertexFormat();
				return rs;
			}
			virtual CoreLib::RefPtr<Drawable> CreateSkeletalDrawable(Mesh * mesh, Skeleton * skeleton, Material * material, bool cacheMesh) override
			{
				if (!material->MaterialPatternModule)
					renderer->sceneRes->RegisterMaterial(material);
				RefPtr<Drawable> rs = CreateDrawableShared(mesh, material, cacheMesh);
				rs->type = DrawableType::Skeletal;
				rs->skeleton = skeleton;
				int poseMatrixSize = skeleton->Bones.Count() * (sizeof(Vec4) * 4);
				CreateTransformModuleInstance(*rs->transformModule, "SkeletalAnimation", poseMatrixSize);
				rs->vertFormat = mesh->GetVertexFormat();
				return rs;
			}
		};
	private:
		RendererSharedResource sharedRes;
		RefPtr<SceneResource> sceneRes;
		RefPtr<ViewResource> mainView;
		RefPtr<RendererServiceImpl> renderService;
		RefPtr<IRenderProcedure> renderProcedure;
		List<RefPtr<WorldRenderPass>> worldRenderPasses;
		List<RefPtr<PostRenderPass>> postRenderPasses;
		HardwareRenderer * hardwareRenderer = nullptr;
		Level* level = nullptr;
		FrameRenderTask frameTask;
		int uniformBufferAlignment = 256;
		int storageBufferAlignment = 32;
	private:
		void RunRenderProcedure()
		{
			if (!level) return;
			RenderProcedureParameters params;
			params.renderStats = &sharedRes.renderStats;
			params.level = level;
			params.renderer = this;
			auto curCam = level->CurrentCamera.Ptr();
			if (curCam)
				params.view = curCam->GetView();
			else
				params.view = View();
			params.rendererService = renderService.Ptr();
			frameTask.NewFrame();
			renderProcedure->Run(frameTask, params);
		}
	public:
		RendererImpl(RenderAPI api)
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
			hardwareRenderer->SetMaxTempBufferVersions(DynamicBufferLengthMultiplier);
			hardwareRenderer->BeginDataTransfer();
			sharedRes.Init(hardwareRenderer);
			sharedRes.envMap = hardwareRenderer->CreateTextureCube(TextureUsage::SampledColorAttachment, 256, 8, StorageFormat::RGBA_F16);
			
			mainView = new ViewResource(hardwareRenderer);
			mainView->Resize(1024, 1024);
			
			renderProcedure = CreateStandardRenderProcedure(true);
			renderProcedure->Init(this, mainView.Ptr());

			// Fetch uniform buffer alignment requirements
			uniformBufferAlignment = hardwareRenderer->UniformBufferAlignment();
			storageBufferAlignment = hardwareRenderer->StorageBufferAlignment();
			
			sceneRes = new SceneResource(&sharedRes, sharedRes.spireContext);
			renderService = new RendererServiceImpl(this);
			hardwareRenderer->EndDataTransfer();
		}
		~RendererImpl()
		{
			for (auto & postPass : postRenderPasses)
				postPass = nullptr;

			renderProcedure = nullptr;
			mainView = nullptr;
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

		RefPtr<ViewResource> cubemapRenderView;
		RefPtr<IRenderProcedure> cubemapRenderProc;

		virtual void InitializeLevel(Level* pLevel) override
		{
			if (!pLevel) return;

			sceneRes->Clear();
			level = pLevel;
			hardwareRenderer->BeginDataTransfer();
			cubemapRenderView = new ViewResource(hardwareRenderer);
			cubemapRenderView->Resize(256, 256);
			cubemapRenderProc = CreateStandardRenderProcedure(false);
			cubemapRenderProc->Init(this, cubemapRenderView.Ptr());
			hardwareRenderer->EndDataTransfer();

			LightProbeRenderer lpRenderer(this, renderService.Ptr(), cubemapRenderProc.Ptr(), cubemapRenderView.Ptr());

			sharedRes.envMap = lpRenderer.RenderLightProbe(level, Vec3::Create(0.0f, 1000.0f, 0.0f));
			
			hardwareRenderer->BeginDataTransfer();
			renderProcedure->UpdateSharedResourceBinding();
			RunRenderProcedure();
			hardwareRenderer->EndDataTransfer();
			RenderFrame();
			Wait();
			sharedRes.renderStats.Clear();
		}
		virtual void TakeSnapshot() override
		{
			if (!level)
				return;
			static int frameId = 0;
			frameId++;
			RunRenderProcedure();
			sharedRes.renderStats.Divisor++;
		}
		virtual RenderStat & GetStats() override
		{
			return sharedRes.renderStats;
		}

		struct DescriptorSetUpdate
		{
			DescriptorSet * descSet;
			int index;
			Buffer * buffer;
			int offset;
			int length;
		};
		/*RefPtr<Buffer> testBuf;
		void TestCompute()
		{
			// debug test compute shader
			hardwareRenderer->BeginDataTransfer();
			const char * shaderSrc = R"(#version 430
			layout(std430, binding=0) buffer storageBuf { float data[];};
			layout(local_size_x=256) in;
			void main() { data[gl_GlobalInvocationID.x] = float(gl_GlobalInvocationID.x);
             }
			)";
			RefPtr<Shader> shader = hardwareRenderer->CreateShader(ShaderType::ComputeShader, shaderSrc, (int)strlen(shaderSrc));
			RefPtr<PipelineBuilder> builder = hardwareRenderer->CreatePipelineBuilder();
			RefPtr<DescriptorSetLayout> descLayout = hardwareRenderer->CreateDescriptorSetLayout(DescriptorLayout(StageFlags::sfCompute, 0, BindingType::StorageBuffer, 0));
			RefPtr<Pipeline> pipeline = builder->CreateComputePipeline(MakeArrayView(descLayout.Ptr()), shader.Ptr());
			RefPtr<AsyncCommandBuffer> cmdBuffer = new AsyncCommandBuffer(hardwareRenderer);
			RefPtr<DescriptorSet> descSet = hardwareRenderer->CreateDescriptorSet(descLayout.Ptr());
			descSet->BeginUpdate();

			if (!testBuf)
				testBuf = hardwareRenderer->CreateBuffer(BufferUsage::StorageBuffer, sizeof(float) * 512);
			float bufData[512];
			bufData[0] = 2.0f;
			testBuf->SetData(bufData, sizeof(float) * 512);
			descSet->Update(0, testBuf.Ptr());
			descSet->EndUpdate();
			auto cmdBuf = cmdBuffer->BeginRecording();
			cmdBuf->BindPipeline(pipeline.Ptr());
			cmdBuf->BindDescriptorSet(0, descSet.Ptr());
			cmdBuf->DispatchCompute(2, 1, 1);
			cmdBuf->MemoryAccessBarrier(MemoryBarrierType::ShaderWriteToHostRead);
			cmdBuf->EndRecording();
			hardwareRenderer->EndDataTransfer();
			hardwareRenderer->ExecuteNonRenderCommandBuffers(MakeArrayView(cmdBuf));
			hardwareRenderer->Wait();

			bufData[0] = 0.0f;
			testBuf->GetData(bufData, 0, sizeof(float) * 512);
			for (int i = 0; i < 512; i++)
				printf("%f ", bufData[i]);
		}*/
		virtual void RenderFrame() override
		{
			if (!level) return;

			sharedRes.renderStats.NumMaterials = 0;
			sharedRes.renderStats.NumShaders = 0;
			for (auto & pass : frameTask.GetTasks())
			{
				pass->Execute(hardwareRenderer, sharedRes.renderStats);
			}

			//TestCompute();
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
			mainView->Resize(w, h);
		}
		Texture2D * GetRenderedImage()
		{
			if (renderProcedure)
				return renderProcedure->GetOutput()->Texture.Ptr();
			return nullptr;
		}
	};

	Renderer* CreateRenderer(RenderAPI api)
	{
		return new RendererImpl(api);
	}
}