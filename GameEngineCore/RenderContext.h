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
		bool instanceUniformUpdated = true;
		bool transformUniformUpdated = true;
		CoreLib::Array<CoreLib::RefPtr<PipelineInstance>, MaxWorldRenderPasses> pipelineInstances;
	public:
		Drawable(SceneResource * sceneRes)
		{
			scene = sceneRes;
		}
		~Drawable();
		void UpdateMaterialUniform();
		void UpdateTransformUniform(const VectorMath::Matrix4 & localTransform);
		void UpdateTransformUniform(const VectorMath::Matrix4 & localTransform, const Pose & pose);
	};

	class RenderTarget
	{
	public:
		GameEngine::StorageFormat Format;
		CoreLib::RefPtr<Texture2D> Texture;
		float ResolutionScale = 1.0f;
	};

	class RendererSharedResource
	{
	private:
		RenderAPI api;
	public:
		CoreLib::RefPtr<HardwareRenderer> hardwareRenderer;
		CoreLib::RefPtr<HardwareApiFactory> hardwareFactory;
		CoreLib::RefPtr<TextureSampler> textureSampler, nearestSampler, linearSampler;
		CoreLib::RefPtr<Buffer> sysUniformBuffer;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<RenderTarget>> renderTargets;
	public:
		CoreLib::RefPtr<Buffer> fullScreenQuadVertBuffer;
	public:
		int screenWidth = 0, screenHeight = 0;
		CoreLib::RefPtr<RenderTarget> AcquireRenderTarget(CoreLib::String name, StorageFormat format);
		CoreLib::RefPtr<RenderTarget> ProvideRenderTarget(CoreLib::String name, StorageFormat format);
		CoreLib::RefPtr<RenderTarget> ProvideOrModifyRenderTarget(CoreLib::String name, StorageFormat format);
		void Resize(int w, int h);
		int GetTextureBindingStart();
		RendererSharedResource(RenderAPI pAPI)
		{
			this->api = pAPI;
		}
		void Destroy()
		{
			textureSampler = nullptr;
			nearestSampler = nullptr;
			linearSampler = nullptr;
			sysUniformBuffer = nullptr;
			renderTargets = CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<RenderTarget>>();
			fullScreenQuadVertBuffer = nullptr;
		}
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
		PipelineClass LoadMaterialPipeline(CoreLib::String identifier, Material * material, RenderTargetLayout * renderTargetLayout, MeshVertexFormat vertFormat, CoreLib::String entryPointShader);
	public:
		DeviceMemory instanceUniformMemory, skeletalTransformMemory, staticTransformMemory;
	public:
		SceneResource(RendererSharedResource * resource);
		
		void Clear();
	};

	class RendererService : public CoreLib::Object
	{
	public:
		virtual void Add(Drawable * object) = 0;
		virtual CoreLib::RefPtr<Drawable> CreateStaticDrawable(Mesh * mesh, Material * material) = 0;
		virtual CoreLib::RefPtr<Drawable> CreateSkeletalDrawable(Mesh * mesh, Skeleton * skeleton, Material * material) = 0;
	};
}
#endif