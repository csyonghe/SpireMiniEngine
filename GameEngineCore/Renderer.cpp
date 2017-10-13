#include "Renderer.h"
#include "HardwareRenderer.h"
#include "Level.h"
#include "Engine.h"
#include "SkeletalMeshActor.h"
#include "EnvMapActor.h"
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
				renderer->sharedRes.CreateModuleInstance(rs, spEnvFindModule(renderer->sharedRes.sharedSpireEnvironment, name), &sceneResources->transformMemory, uniformBufferSize);
			}

			virtual CoreLib::RefPtr<Drawable> CreateStaticDrawable(Mesh * mesh, int elementId, Material * material, bool cacheMesh) override
			{
				if (!material->MaterialPatternModule)
					renderer->sceneRes->RegisterMaterial(material);
				RefPtr<Drawable> rs = CreateDrawableShared(mesh, material, cacheMesh);
				rs->type = DrawableType::Static;
				rs->elementRange = mesh->ElementRanges[elementId];
				CreateTransformModuleInstance(*rs->transformModule, "NoAnimation", (int)(sizeof(Vec4) * 4));
				rs->vertFormat = mesh->GetVertexFormat();
				return rs;
			}
			virtual CoreLib::RefPtr<Drawable> CreateSkeletalDrawable(Mesh * mesh, int elementId, Skeleton * skeleton, Material * material, bool cacheMesh) override
			{
				if (!material->MaterialPatternModule)
					renderer->sceneRes->RegisterMaterial(material);
				RefPtr<Drawable> rs = CreateDrawableShared(mesh, material, cacheMesh);
				rs->type = DrawableType::Skeletal;
				rs->elementRange = mesh->ElementRanges[elementId];
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
		EnumerableDictionary<int, int> worldRenderPassIds;
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

			sharedRes.Init(hardwareRenderer);

			mainView = new ViewResource(hardwareRenderer);
			mainView->Resize(1024, 1024);
			
			renderProcedure = CreateStandardRenderProcedure(true, true);
			renderProcedure->Init(this, mainView.Ptr());

			// Fetch uniform buffer alignment requirements
			uniformBufferAlignment = hardwareRenderer->UniformBufferAlignment();
			storageBufferAlignment = hardwareRenderer->StorageBufferAlignment();
			
			sceneRes = new SceneResource(&sharedRes);
			renderService = new RendererServiceImpl(this);
			hardwareRenderer->Wait();
		}
		~RendererImpl()
		{
			Wait();
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

		virtual int RegisterWorldRenderPass(SpireShader * renderPass) override
		{
			int shaderId = spShaderGetId(renderPass);
			int passId;
			if (worldRenderPassIds.TryGetValue(shaderId, passId))
				return passId;
			int newId = worldRenderPassIds.Count();
			worldRenderPassIds[shaderId] = newId;
			return newId;
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
			level = pLevel;
			cubemapRenderView = new ViewResource(hardwareRenderer);
			cubemapRenderView->Resize(EnvMapSize, EnvMapSize);
			cubemapRenderProc = CreateStandardRenderProcedure(false, false);
			cubemapRenderProc->Init(this, cubemapRenderView.Ptr());

			LightProbeRenderer lpRenderer(this, renderService.Ptr(), cubemapRenderProc.Ptr(), cubemapRenderView.Ptr());
			int lightProbeCount = 0;
			for (auto & actor : level->Actors)
			{
				if (actor.Value->GetEngineType() == EngineActorType::EnvMap)
				{
					auto envMapActor = dynamic_cast<EnvMapActor*>(actor.Value.Ptr());
					if (envMapActor->GetEnvMapId() != -1)
						lpRenderer.RenderLightProbe(sharedRes.envMapArray.Ptr(), envMapActor->GetEnvMapId(), level, envMapActor->GetPosition());
					lightProbeCount++;
				}
			}
			if (lightProbeCount == 0)
				lpRenderer.RenderLightProbe(sharedRes.envMapArray.Ptr(), sharedRes.AllocEnvMap(), level, Vec3::Create(0.0f, 1000.0f, 0.0f));
			Wait();
			renderProcedure->UpdateSharedResourceBinding();
			RunRenderProcedure();
			hardwareRenderer->TransferBarrier(DynamicBufferLengthMultiplier);
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
		virtual void Resize(int w, int h) override
		{
			mainView->Resize(w, h);
			Wait();
		}
		Texture2D * GetRenderedImage()
		{
			if (renderProcedure)
				return renderProcedure->GetOutput()->Texture.Ptr();
			return nullptr;
		}
		virtual void DestroyContext() override
		{
			sharedRes.ResetEnvMapAllocation();
			sceneRes->Clear();
		}
	};

	Renderer* CreateRenderer(RenderAPI api)
	{
		return new RendererImpl(api);
	}
}