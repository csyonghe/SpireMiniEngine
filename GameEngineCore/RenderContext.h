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
#include "FrustumCulling.h"
#include "PipelineContext.h"
#include "EngineLimits.h"

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
		CoreLib::RefPtr<DrawableMesh> mesh = nullptr;
		Material * material = nullptr;
		Skeleton * skeleton = nullptr;
		MeshVertexFormat vertFormat;
		DrawableType type = DrawableType::Static;
		CoreLib::RefPtr<ModuleInstance> transformModule;
		CoreLib::Array<PipelineClass*, MaxWorldRenderPasses> pipelineCache;
	public:
		CoreLib::Graphics::BBox Bounds;
		void * ReorderKey = nullptr;
		Drawable(SceneResource * sceneRes)
		{
			scene = sceneRes;
			Bounds.Min = VectorMath::Vec3::Create(-1e9f);
			Bounds.Max = VectorMath::Vec3::Create(1e9f);
			pipelineCache.SetSize(pipelineCache.GetCapacity());
			for (auto & p : pipelineCache)
				p = nullptr;
		}
		PipelineClass * GetPipeline(int passId, PipelineContext & pipelineManager);
		inline ModuleInstance * GetTransformModule()
		{
			return transformModule.Ptr();
		}
		inline DrawableMesh * GetMesh()
		{
			return mesh.Ptr();
		}
		inline Material* GetMaterial()
		{
			return material;
		}
		MeshVertexFormat & GetVertexFormat()
		{
			return vertFormat;
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
		FixedFunctionPipelineStates * fixedFunctionStates = nullptr;
	public:
		void SetFixedOrderDrawContent(PipelineContext & pipelineManager, CullFrustum frustum, CoreLib::ArrayView<Drawable*> drawables);
		void SetDrawContent(PipelineContext & pipelineManager, CoreLib::List<Drawable*> & reorderBuffer, CullFrustum frustum, CoreLib::ArrayView<Drawable*> drawables);
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
		PipelineContext pipelineManager;
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
		CoreLib::RefPtr<ModuleInstance> defaultMaterialPatternModule, defaultMaterialGeometryModule;
		CoreLib::EnumerableDictionary<Mesh*, CoreLib::RefPtr<DrawableMesh>> meshes;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Texture2D>> textures;
		CoreLib::RefPtr<ModuleInstance> CreateMaterialModuleInstance(Material* material, const char * moduleName, bool isPatternModule);
	public:
		CoreLib::RefPtr<DrawableMesh> LoadDrawableMesh(Mesh * mesh);
        CoreLib::RefPtr<DrawableMesh> CreateDrawableMesh(Mesh * mesh);

		Texture2D* LoadTexture2D(const CoreLib::String & name, CoreLib::Graphics::TextureFile & data);
		Texture2D* LoadTexture(const CoreLib::String & filename);
	public:
		DeviceMemory instanceUniformMemory, transformMemory;
		void RegisterMaterial(Material * material);
		
	public:
		SceneResource(RendererSharedResource * resource, SpireCompilationContext * spireCtx);
		~SceneResource()
		{
			Clear();
			spPopContext(spireContext);
		}
		void Clear();
	};

	class RendererService : public CoreLib::Object
	{
	public:
		virtual CoreLib::RefPtr<Drawable> CreateStaticDrawable(Mesh * mesh, Material * material, bool cacheMesh = true) = 0;
		virtual CoreLib::RefPtr<Drawable> CreateSkeletalDrawable(Mesh * mesh, Skeleton * skeleton, Material * material, bool cacheMesh = true) = 0;
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