#ifndef GAME_ENGINE_RENDER_CONTEXT_H
#define GAME_ENGINE_RENDER_CONTEXT_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Graphics/TextureFile.h"
#include "HardwareRenderer.h"
#include "Spire/Spire.h"
#include "Skeleton.h"
#include "DeviceMemory.h"
#include "Mesh.h"
#include "Common.h"
#include "FrustumCulling.h"

namespace GameEngine
{
	class Mesh;
	class Skeleton;
	class Material;
	class SceneResource;
	class Renderer;
	class CameraActor;
	class RendererSharedResource;
	class Level;

	struct BoneTransform
	{
		VectorMath::Matrix4 TransformMatrix;
		VectorMath::Vec4 NormalMatrix[3];
	};

	enum class RenderAPI
	{
		OpenGL, Vulkan
	};

	class DrawableMesh : public CoreLib::RefObject
	{
	private:
		RendererSharedResource * renderRes;
	public:
		VertexFormat vertexFormat;
		int vertexBufferOffset;
		int indexBufferOffset;
		int vertexCount = 0;
		int indexCount = 0;
		Buffer * GetVertexBuffer();
		Buffer * GetIndexBuffer();
		DrawableMesh(RendererSharedResource * pRenderRes)
		{
			renderRes = pRenderRes;
		}
		~DrawableMesh();
	};

	struct Viewport
	{
		int X, Y, Width, Height;
		Viewport() = default;
		Viewport(int x, int y, int w, int h)
		{
			X = x;
			Y = y;
			Width = w;
			Height = h;
		}
		Viewport(int w, int h)
		{
			X = 0;
			Y = 0;
			Width = w;
			Height = h;
		}
	};

	class PipelineClass
	{
	public:
		CoreLib::List<Shader*> shaders;
		CoreLib::RefPtr<Pipeline> pipeline;
		CoreLib::List<CoreLib::RefPtr<DescriptorSetLayout>> descriptorSetLayouts;
	};

	class ModuleInstance
	{
	public:
		CoreLib::RefPtr<DescriptorSetLayout> DescriptorLayout;
		CoreLib::RefPtr<DescriptorSet> Descriptors;
		DeviceMemory * UniformMemory;
		int BufferOffset = 0, BufferLength = 0;
		unsigned char * UniformPtr = nullptr;
		CoreLib::String BindingName;

		void SetUniformData(void * data, int length)
		{
#ifdef _DEBUG
			if (length > BufferLength)
				throw HardwareRendererException("insufficient uniform buffer.");
#endif
			UniformMemory->GetBuffer()->SetData(BufferOffset, data, CoreLib::Math::Min(length, BufferLength));
		}
		~ModuleInstance()
		{
			if (UniformMemory)
				UniformMemory->Free(UniformPtr, BufferLength);
		}
	};

	enum class DrawableType
	{
		Static, Skeletal
	};
	
	class Drawable : public CoreLib::RefObject
	{
		friend class RendererImpl;
		friend class SceneResource;
		friend class RendererServiceImpl;
	private:
		SceneResource * scene = nullptr;
		DrawableMesh * mesh = nullptr;
		Material * material = nullptr;
		Skeleton * skeleton = nullptr;
		DrawableType type = DrawableType::Static;
		CoreLib::Array<CoreLib::RefPtr<PipelineClass>, MaxWorldRenderPasses> pipelineInstances;
		CoreLib::RefPtr<ModuleInstance> transformModule;
	public:
		CoreLib::Graphics::BBox Bounds;
		Drawable(SceneResource * sceneRes)
		{
			scene = sceneRes;
			Bounds.Min = VectorMath::Vec3::Create(-1e9f);
			Bounds.Max = VectorMath::Vec3::Create(1e9f);
		}
		inline PipelineClass * GetPipeline(int passId)
		{
			return pipelineInstances[passId].Ptr();
		}
		inline ModuleInstance * GetTransformModule()
		{
			return transformModule.Ptr();
		}
		inline DrawableMesh * GetMesh()
		{
			return mesh;
		}
		inline Material* GetMaterial()
		{
			return material;
		}
		void UpdateMaterialUniform();
		void UpdateTransformUniform(const VectorMath::Matrix4 & localTransform);
		void UpdateTransformUniform(const VectorMath::Matrix4 & localTransform, const Pose & pose);
	};

	class RenderTarget : public CoreLib::RefObject
	{
	public:
		GameEngine::StorageFormat Format;
		CoreLib::RefPtr<Texture2D> Texture;
		CoreLib::RefPtr<Texture2DArray> TextureArray;
		int Layer = 0;
		float ResolutionScale = 1.0f;
		bool UseFixedResolution = false;
		int FixedWidth = 1024, FixedHeight = 1024;
		int Width = 0, Height = 0;
		RenderTarget() = default;
		RenderTarget(GameEngine::StorageFormat format, CoreLib::RefPtr<Texture2DArray> texArray, int layer, int w, int h);
	};

	class RenderOutput : public CoreLib::RefObject
	{
		friend class RendererSharedResource;
	private:
		CoreLib::RefPtr<FrameBuffer> frameBuffer;
		CoreLib::List<CoreLib::RefPtr<RenderTarget>> bindings;
		RenderTargetLayout * renderTargetLayout = nullptr;
		RenderOutput() {}
	public:
		void GetSize(int &w, int &h)
		{
			w = bindings.First()->Width;
			h = bindings.First()->Height;
		}
		FrameBuffer * GetFrameBuffer()
		{
			return frameBuffer.Ptr();
		}
	};

	struct DescriptorSetBindings
	{
		struct Binding
		{
			int index;
			DescriptorSet * descriptorSet;
		};
		CoreLib::Array<Binding, 16> bindings;
		void Bind(int id, DescriptorSet * set)
		{
			bindings.Add(Binding{ id, set });
		}
	};

	class RenderPassInstance
	{
		friend class WorldRenderPass;
		friend class PostRenderPass;
		friend class RendererImpl;
	private:
		int renderPassId = -1;
		int numDrawCalls = 0;
		Viewport viewport;
		CommandBuffer * commandBuffer = nullptr;
		RenderOutput * renderOutput = nullptr;
	public:
		template<typename TQueryable, typename TEnumerator>
		void RecordCommandBuffer(const DescriptorSetBindings & passBindings, CullFrustum frustum, const CoreLib::Queryable<TQueryable, TEnumerator, Drawable*> & drawables)
		{
			renderOutput->GetSize(viewport.Width, viewport.Height);
			commandBuffer->BeginRecording(renderOutput->GetFrameBuffer());
			commandBuffer->SetViewport(viewport.X, viewport.Y, viewport.Width, viewport.Height);
			commandBuffer->ClearAttachments(renderOutput->GetFrameBuffer());
			for (auto & binding : passBindings.bindings)
				commandBuffer->BindDescriptorSet(binding.index, binding.descriptorSet);
			numDrawCalls = 0;
			for (auto&& obj : drawables)
			{
				if (!frustum.IsBoxInFrustum(obj->Bounds))
					continue;
				numDrawCalls++;
				if (auto pipelineInst = obj->GetPipeline(renderPassId))
				{
					auto mesh = obj->GetMesh();
					commandBuffer->BindPipeline(pipelineInst->pipeline.Ptr());
					commandBuffer->BindIndexBuffer(mesh->GetIndexBuffer(), mesh->indexBufferOffset);
					commandBuffer->BindVertexBuffer(mesh->GetVertexBuffer(), mesh->vertexBufferOffset);
					commandBuffer->BindDescriptorSet(2, obj->GetMaterial()->MaterialModule->Descriptors.Ptr());
					commandBuffer->BindDescriptorSet(3, obj->GetTransformModule()->Descriptors.Ptr());
					commandBuffer->DrawIndexed(0, mesh->indexCount);
				}
			}
			commandBuffer->EndRecording();
		}
	};

	class RendererService;

	struct RenderProcedureParameters
	{
		Renderer * renderer;
		CoreLib::ArrayView<CameraActor *> cameras;
		Level * level;
		RendererService * rendererService;
	};

	class IRenderProcedure : public CoreLib::RefObject
	{
	public:
		virtual void Init(Renderer * renderer) = 0;
		virtual void ResizeFrame(int width, int height) = 0;
		virtual void Run(CoreLib::List<RenderPassInstance> & renderPasses, CoreLib::List<PostRenderPass*> & postPasses, const RenderProcedureParameters & params) = 0;
		virtual RenderTarget* GetOutput() = 0;
	};

	class ShadowMapResource
	{
	private:
		int shadowMapArraySize;
		CoreLib::IntSet shadowMapArrayFreeBits;
	public:
		CoreLib::RefPtr<Texture2DArray> shadowMapArray;
		CoreLib::RefPtr<RenderTargetLayout> shadowMapRenderTargetLayout;
		CoreLib::List<CoreLib::RefPtr<RenderOutput>> shadowMapRenderOutputs;
		int AllocShadowMaps(int count);
		void FreeShadowMaps(int id, int count);
		void Init(HardwareRenderer * hwRenderer, RendererSharedResource * res);
		void Destroy();
		void Reset();
	};

	CoreLib::String GetSpireOutput(SpireDiagnosticSink * sink);
	
	class RendererSharedResource
	{
	private:
		RenderAPI api;
		void LoadShaderLibrary();
	public:
		CoreLib::RefPtr<HardwareRenderer> hardwareRenderer;
		CoreLib::RefPtr<TextureSampler> textureSampler, nearestSampler, linearSampler, shadowSampler;
		SpireCompilationContext * spireContext = nullptr;
		void * viewUniformPtr = nullptr;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<RenderTarget>> renderTargets;
		ShadowMapResource shadowMapResources;
		CoreLib::List<CoreLib::RefPtr<RenderOutput>> renderOutputs;
		void UpdateRenderResultFrameBuffer(RenderOutput * output);
		ModuleInstance * CreateModuleInstance(SpireModule * shaderModule, DeviceMemory * uniformMemory, int uniformBufferSize = 0);
	public:
		CoreLib::RefPtr<Buffer> fullScreenQuadVertBuffer;
		DeviceMemory indexBufferMemory, vertexBufferMemory;
	public:
		int screenWidth = 0, screenHeight = 0;
		CoreLib::RefPtr<RenderTarget> LoadSharedRenderTarget(CoreLib::String name, StorageFormat format, float ratio = 1.0f, int w0 = 1024, int h0 = 1024);
		template<typename ...TRenderTargets>
		RenderOutput * CreateRenderOutput(RenderTargetLayout * renderTargetLayout, TRenderTargets ... renderTargets)
		{
			RefPtr<RenderOutput> result = new RenderOutput();
			typename FirstType<TRenderTargets...>::type targets[] = { renderTargets... };
			for (auto & target : targets)
			{
				result->bindings.Add(target);
			}
			result->renderTargetLayout = renderTargetLayout;
			renderOutputs.Add(result);
			UpdateRenderResultFrameBuffer(result.Ptr());
			return result.Ptr();
		}
		void DestroyRenderOutput(RenderOutput * output)
		{
			int id = renderOutputs.IndexOf(output);
			if (id != -1)
			{
				renderOutputs[id] = nullptr;
				renderOutputs.FastRemoveAt(id);
			}
		}
		void Resize(int w, int h);

		RendererSharedResource(RenderAPI pAPI)
		{
			this->api = pAPI;
		}
		void Init(HardwareRenderer * phwRenderer);
		void Destroy();
	};

	class SceneResource
	{
	private:
		RendererSharedResource * rendererResource;
		SpireCompilationContext * spireContext = nullptr;
		CoreLib::RefPtr<ModuleInstance> defaultMaterialModule;
		CoreLib::Dictionary<int, VertexFormat> vertexFormats;
		CoreLib::EnumerableDictionary<Mesh*, CoreLib::RefPtr<DrawableMesh>> meshes;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Shader>> shaders;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<PipelineClass>> pipelineClassCache;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Texture2D>> textures;
		//CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Material>> materials;
		CoreLib::RefPtr<ModuleInstance> CreateMaterialModuleInstance(Material* material, const char * moduleName);
		void RegisterMaterial(Material * material);
	public:
		CoreLib::RefPtr<DrawableMesh> LoadDrawableMesh(Mesh * mesh);
		Texture2D* LoadTexture2D(const CoreLib::String & name, CoreLib::Graphics::TextureFile & data);
		Texture2D* LoadTexture(const CoreLib::String & filename);
		Shader* LoadShader(const CoreLib::String & src, void* data, int size, ShaderType shaderType);
		VertexFormat LoadVertexFormat(MeshVertexFormat vertFormat);
		CoreLib::RefPtr<PipelineClass> LoadMaterialPipeline(CoreLib::String identifier, Material * material, RenderTargetLayout * renderTargetLayout, MeshVertexFormat vertFormat, CoreLib::String entryPointShader, CoreLib::Procedure<PipelineBuilder*> setAdditionalPipelineArguments);
	public:
		DeviceMemory instanceUniformMemory, transformMemory;
	public:
		SceneResource(RendererSharedResource * resource, SpireCompilationContext * spireCtx);
		~SceneResource()
		{
			Clear();
		}
		void Clear();
	};

	class RendererService : public CoreLib::Object
	{
	public:
		virtual CoreLib::RefPtr<Drawable> CreateStaticDrawable(Mesh * mesh, Material * material) = 0;
		virtual CoreLib::RefPtr<Drawable> CreateSkeletalDrawable(Mesh * mesh, Skeleton * skeleton, Material * material) = 0;
	};

	class DrawableSink
	{
	private:
		CoreLib::List<Drawable*> drawables;
	public:
		void AddDrawable(Drawable * drawable)
		{
			drawable->UpdateMaterialUniform();
			drawables.Add(drawable);
		}
		void Clear()
		{
			drawables.Clear();
		}
		CoreLib::ArrayView<Drawable*> GetDrawables()
		{
			return drawables.GetArrayView();
		}
	};
}
#endif