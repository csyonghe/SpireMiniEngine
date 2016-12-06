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

#define DEFERRED 0
const char* lightingString = DEFERRED ? "DeferredLighting" : "ForwardLighting";

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


	class SystemUniforms
	{
	public:
		Matrix4 ViewTransform, ViewProjectionTransform, InvViewTransform, InvViewProjTransform;
		Vec3 CameraPos;
        float Time;
	};

	struct BoneTransform
	{
		Matrix4 TransformMatrix;
		Vec4 NormalMatrix[3];
	};
	

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
			List<Drawable*> drawables;
			virtual void Add(Drawable * object) override
			{
				drawables.Add(object);
			}
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
		List<RefPtr<WorldRenderPass>> worldRenderPasses;
		List<RefPtr<PostRenderPass>> postRenderPasses;
		HardwareRenderer * hardwareRenderer = nullptr;
		Level* level = nullptr;
		Array<RefPtr<CommandBuffer>, MaxWorldRenderPasses> worldRenderPassCommandBuffers;
		SystemUniforms sysUniforms;

		int uniformBufferAlignment = 256;
		int storageBufferAlignment = 32;
	private:
		void RecordCommandBuffer()
		{
			if (!level) return;

			for (int i = 0; i < worldRenderPasses.Count(); i++)
			{
				auto cmdBuffer = worldRenderPassCommandBuffers[i].Ptr();
				cmdBuffer->BeginRecording(worldRenderPasses[i]->GetRenderTargetLayout(), worldRenderPasses[i]->GetFrameBuffer());
				cmdBuffer->SetViewport(0, 0, sharedRes.screenWidth, sharedRes.screenHeight);
				for (auto& obj : renderService->drawables)
				{
					if (obj->pipelineInstances[i])
					{
						cmdBuffer->BindIndexBuffer(obj->mesh->indexBuffer.Ptr());
						cmdBuffer->BindVertexBuffer(obj->mesh->vertexBuffer.Ptr());
						cmdBuffer->BindPipeline(obj->pipelineInstances[i].Ptr());
						cmdBuffer->DrawIndexed(0, obj->mesh->indexCount);
					}
				}
				cmdBuffer->EndRecording();
			}
		}
	public:
		RendererImpl(WindowHandle window, RenderAPI api)
			: sharedRes(api)
		{
			switch (api)
			{
			case RenderAPI::Vulkan:
				sharedRes.hardwareFactory = CreateVulkanFactory();
				break;
			case RenderAPI::OpenGL:
				sharedRes.hardwareFactory = CreateOpenGLFactory();
				break;
			}
			sharedRes.hardwareRenderer = sharedRes.hardwareFactory->CreateRenderer(Engine::Instance()->GpuId);
			hardwareRenderer = sharedRes.hardwareRenderer.Ptr();
			hardwareRenderer->BindWindow(window, 640, 480);
			
			// Fetch uniform buffer alignment requirements
			uniformBufferAlignment = hardwareRenderer->UniformBufferAlignment();
			storageBufferAlignment = hardwareRenderer->StorageBufferAlignment();
			
			// Vertex buffer for VS bypass
			const float fsTri[] =
			{
				-1.0f, -1.0f, 0.0f, 0.0f,
				1.0f, -1.0f, 1.0f, 0.0f,
				1.0f, 1.0f, 1.0f, 1.0f,
				-1.0f, 1.0f, 0.0f, 1.0f
			};
			sharedRes.fullScreenQuadVertBuffer = hardwareRenderer->CreateBuffer(BufferUsage::ArrayBuffer);
			sharedRes.fullScreenQuadVertBuffer->SetData((void*)&fsTri[0], sizeof(fsTri));

			// Create and resize uniform buffer
			sharedRes.sysUniformBuffer = hardwareRenderer->CreateMappedBuffer(BufferUsage::UniformBuffer);
			sharedRes.sysUniformBuffer->SetData(nullptr, sizeof(SystemUniforms));

			// Create common texture samplers
			sharedRes.nearestSampler = hardwareRenderer->CreateTextureSampler();
			sharedRes.nearestSampler->SetFilter(TextureFilter::Nearest);

			sharedRes.linearSampler = hardwareRenderer->CreateTextureSampler();
			sharedRes.linearSampler->SetFilter(TextureFilter::Linear);

			sharedRes.textureSampler = hardwareRenderer->CreateTextureSampler();
			sharedRes.textureSampler->SetFilter(TextureFilter::Anisotropic16x);

			sceneRes = new SceneResource(&sharedRes);
			renderService = new RendererServiceImpl(this);
		}
		~RendererImpl()
		{
			sceneRes = nullptr;
			sharedRes.Destroy();
		}

		virtual void Wait() override
		{
			hardwareRenderer->Wait();
		}

		virtual void RegisterWorldRenderPass(WorldRenderPass * renderPass) override
		{
			if (worldRenderPasses.Count() >= MaxWorldRenderPasses)
				throw InvalidOperationException("Number of registered world render passes exceeds engine limit.");
			renderPass->Init(&sharedRes, sceneRes.Ptr());
			worldRenderPasses.Add(renderPass);
			worldRenderPassCommandBuffers.Add(hardwareRenderer->CreateCommandBuffer());
		}

		virtual void RegisterPostRenderPass(PostRenderPass * renderPass) override
		{
			if (postRenderPasses.Count() >= MaxPostRenderPasses)
				throw InvalidOperationException("Number of registered post render passes exceeds engine limit.");
			renderPass->Init(&sharedRes, sceneRes.Ptr());
			postRenderPasses.Add(renderPass);
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

			renderService->drawables.Clear();
			for (auto & actor : level->Actors)
			{
				actor.Value->GetDrawables(renderService.Ptr());
			}
			for (int i = 0; i < renderService->drawables.Count(); i++)
			{
				auto drawable = renderService->drawables[i];

				drawable->UpdateMaterialUniform();
				if (drawable->instanceUniformUpdated)
				{
					if (drawable->uniforms.instanceUniform)
						sceneRes->instanceUniformMemory.Sync(drawable->uniforms.instanceUniform, drawable->uniforms.instanceUniformCount);
					drawable->instanceUniformUpdated = false;
				}
				if (drawable->transformUniformUpdated)
				{
					switch (drawable->type)
					{
					case DrawableType::Static:
						sceneRes->staticTransformMemory.Sync(drawable->uniforms.transformUniform, drawable->uniforms.transformUniformCount);
						break;
					case DrawableType::Skeletal:
						sceneRes->skeletalTransformMemory.Sync(drawable->uniforms.transformUniform, drawable->uniforms.transformUniformCount);
						break;
					}
					drawable->transformUniformUpdated = false;
				}
			}

			RecordCommandBuffer();

            this->sysUniforms.Time = Engine::Instance()->GetTime();
			// Update system uniforms
			if (level->CurrentCamera)
			{
				this->sysUniforms.CameraPos = level->CurrentCamera->GetPosition();
				this->sysUniforms.ViewTransform = level->CurrentCamera->GetLocalTransform();
				Matrix4 projMatrix;
				Matrix4::CreatePerspectiveMatrixFromViewAngle(projMatrix,
					level->CurrentCamera->FOV, sharedRes.screenWidth / (float)sharedRes.screenHeight,
					level->CurrentCamera->ZNear, level->CurrentCamera->ZFar);
				Matrix4::Multiply(this->sysUniforms.ViewProjectionTransform, projMatrix, this->sysUniforms.ViewTransform);
			}
			else
			{
				this->sysUniforms.CameraPos = Vec3::Create(0.0f);
				Matrix4::CreateIdentityMatrix(this->sysUniforms.ViewTransform);
				Matrix4::CreatePerspectiveMatrixFromViewAngle(this->sysUniforms.ViewProjectionTransform,
					75.0f, sharedRes.screenWidth / (float)sharedRes.screenHeight, 40.0f, 200000.0f);
			}
			this->sysUniforms.ViewTransform.Inverse(this->sysUniforms.InvViewTransform);
			this->sysUniforms.ViewProjectionTransform.Inverse(this->sysUniforms.InvViewProjTransform);
			sharedRes.sysUniformBuffer->SetData(&sysUniforms, sizeof(SystemUniforms));
		}
		virtual void RenderFrame() override
		{
			if (!level) return;
			Array<GameEngine::CommandBuffer*, 2> commandBufferList1;
			for (int i = 0; i < worldRenderPassCommandBuffers.Count(); i++)
			{
				auto clearCmd = worldRenderPasses[i]->GetClearCommandBuffer();
				if (clearCmd)
					commandBufferList1.Add(clearCmd);
				commandBufferList1.Add(worldRenderPassCommandBuffers[i].Ptr());
				hardwareRenderer->ExecuteCommandBuffers(worldRenderPasses[i]->GetRenderTargetLayout(), worldRenderPasses[i]->GetFrameBuffer(), commandBufferList1.GetArrayView());
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
			for (auto & renderPass : worldRenderPasses)
				renderPass->UpdateFrameBuffer();
			for (auto & post : postRenderPasses)
				post->UpdateFrameBuffer();
			
			hardwareRenderer->Resize(w, h);
			RecordCommandBuffer();
		}
		Texture2D * GetRenderedImage()
		{
			return sharedRes.AcquireRenderTarget("litColor", StorageFormat::RGBA_8)->Texture.Ptr();
		}
	};

	Renderer* CreateRenderer(WindowHandle window, RenderAPI api)
	{
		return new RendererImpl(window, api);
	}
}