#ifndef GAME_ENGINE_RENDER_CONTEXT_H
#define GAME_ENGINE_RENDER_CONTEXT_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Graphics/TextureFile.h"
#include "HardwareRenderer.h"
#include "HardwareApiFactory.h"
#include "Skeleton.h"
#include "DeviceMemory.h"
#include "Mesh.h"
#include "Common.h"

namespace GameEngine
{
	class Mesh;
	class Skeleton;
	class Material;
	class SceneResource;
	class Renderer;
	class CameraActor;
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
	public:
		CoreLib::RefPtr<Buffer> vertexBuffer, indexBuffer;
		VertexFormat vertexFormat;
		int vertexCount;
		int indexCount;
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
	};

	enum class DrawableType
	{
		Static, Skeletal
	};

	class DrawableSharedUniformBuffer
	{
	public:
		unsigned char * instanceUniform = nullptr;
		unsigned char * transformUniform = nullptr;
		int instanceUniformCount = 0;
		int transformUniformCount = 0;
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
		DrawableSharedUniformBuffer uniforms;
		CoreLib::Array<CoreLib::RefPtr<PipelineInstance>, MaxWorldRenderPasses> pipelineInstances;
	public:
		Drawable(SceneResource * sceneRes)
		{
			scene = sceneRes;
		}
		~Drawable();
		inline PipelineInstance * GetPipelineInstance(int passId)
		{
			return pipelineInstances[passId].Ptr();
		}
		inline DrawableMesh * GetMesh()
		{
			return mesh;
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
		friend class RenderPass;
		friend class RendererImpl;
	private:
		int renderPassId = -1;
		Viewport viewport;
		void * viewUniformPtr = nullptr;
		int viewUniformSize = 0;
		CommandBuffer * commandBuffer = nullptr;
		RenderOutput * renderOutput = nullptr;
	public:
		template<typename TQueryable, typename TEnumerator>
		void RecordCommandBuffer(const CoreLib::Queryable<TQueryable, TEnumerator, Drawable*> & drawables)
		{
			renderOutput->GetSize(viewport.Width, viewport.Height);
			commandBuffer->BeginRecording(renderOutput->GetFrameBuffer());
			commandBuffer->SetViewport(viewport.X, viewport.Y, viewport.Width, viewport.Height);
			commandBuffer->ClearAttachments(renderOutput->GetFrameBuffer());
			for (auto&& obj : drawables)
			{
				if (auto pipelineInst = obj->GetPipelineInstance(renderPassId))
				{
					auto mesh = obj->GetMesh();
					commandBuffer->BindIndexBuffer(mesh->indexBuffer.Ptr());
					commandBuffer->BindVertexBuffer(mesh->vertexBuffer.Ptr());
					commandBuffer->BindPipeline(pipelineInst);
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
		virtual void Run(CoreLib::List<RenderPassInstance> & renderPasses, const RenderProcedureParameters & params) = 0;
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
	
	class RendererSharedResource
	{
	private:
		RenderAPI api;
		
	public:
		CoreLib::RefPtr<HardwareRenderer> hardwareRenderer;
		CoreLib::RefPtr<HardwareApiFactory> hardwareFactory;
		CoreLib::RefPtr<TextureSampler> textureSampler, nearestSampler, linearSampler, shadowSampler;
		CoreLib::RefPtr<Buffer> viewUniformBuffer, lightUniformBuffer;
		void * viewUniformPtr = nullptr;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<RenderTarget>> renderTargets;
		ShadowMapResource shadowMapResources;
		CoreLib::List<CoreLib::RefPtr<RenderOutput>> renderOutputs;
		void UpdateRenderResultFrameBuffer(RenderOutput * output);
	public:
		CoreLib::RefPtr<Buffer> fullScreenQuadVertBuffer;
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
		int GetTextureBindingStart();
		RendererSharedResource(RenderAPI pAPI)
		{
			this->api = pAPI;
		}
		void SetViewUniformData(void * data, int size);
		void Init(HardwareApiFactory * hwFactory, HardwareRenderer * phwRenderer);
		void Destroy();
	};

	class SceneResource
	{
	private:
		RendererSharedResource * rendererResource;
		CoreLib::Dictionary<int, VertexFormat> vertexFormats;
		CoreLib::EnumerableDictionary<Mesh*, CoreLib::RefPtr<DrawableMesh>> meshes;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Shader>> shaders;
		CoreLib::EnumerableDictionary<CoreLib::String, PipelineClass> pipelineClassCache;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Texture2D>> textures;
	public:
		CoreLib::RefPtr<DrawableMesh> LoadDrawableMesh(Mesh * mesh);
		Texture2D* LoadTexture2D(const CoreLib::String & name, CoreLib::Graphics::TextureFile & data);
		Texture2D* LoadTexture(const CoreLib::String & filename);
		Shader* LoadShader(const CoreLib::String & src, void* data, int size, ShaderType shaderType);
		VertexFormat LoadVertexFormat(MeshVertexFormat vertFormat);
		PipelineClass LoadMaterialPipeline(CoreLib::String identifier, Material * material, RenderTargetLayout * renderTargetLayout, MeshVertexFormat vertFormat, CoreLib::String entryPointShader, CoreLib::Procedure<PipelineBuilder*> setAdditionalPipelineArguments);
	public:
		DeviceMemory instanceUniformMemory, skeletalTransformMemory, staticTransformMemory;
	public:
		SceneResource(RendererSharedResource * resource);
		
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