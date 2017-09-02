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
#include "AsyncCommandBuffer.h"
#include "Drawable.h"
#include "ViewResource.h"
#include "View.h"
#include "ViewResource.h"
#include "Renderer.h"
#include "CoreLib/PerformanceCounter.h"

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

	const StorageFormat DepthBufferFormat = StorageFormat::Depth32;

	class RenderStat
	{
	public:
		float TotalTime;
		int Divisor = 0;
		int NumDrawCalls = 0;
		int NumPasses = 0;
		int NumShaders = 0;
		int NumMaterials = 0;
		float CpuTime = 0.0f;
		float PipelineLookupTime = 0.0f;
		CoreLib::Diagnostics::TimePoint StartTime;
		void Clear()
		{
			Divisor = 0;
			NumDrawCalls = 0;
			NumPasses = 0;
			NumShaders = 0;
			NumMaterials = 0;
			CpuTime = 0.0f;
			PipelineLookupTime = 0.0f;
		}
	};

	struct BoneTransform
	{
		VectorMath::Matrix4 TransformMatrix;
		VectorMath::Vec4 NormalMatrix[3];
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

	class SharedModuleInstances
	{
	public:
		ModuleInstance * View;
	};

	class RenderTask : public CoreLib::RefObject
	{
	public:
		virtual void Execute(HardwareRenderer * hw, RenderStat & stats) = 0;
	};

	class WorldPassRenderTask : public RenderTask
	{
	public:
		int renderPassId = -1; 
		int numDrawCalls = 0; 
		int numMaterials = 0; 
		int numShaders = 0;
		SharedModuleInstances sharedModules; 
		AsyncCommandBuffer * commandBuffer = nullptr; 
		RenderOutput * renderOutput = nullptr; 
		FixedFunctionPipelineStates * fixedFunctionStates = nullptr;
		Viewport viewport; 
		bool clearOutput = false;
		virtual void Execute(HardwareRenderer * hw, RenderStat & stats) override;
		void SetFixedOrderDrawContent(PipelineContext & pipelineManager, CoreLib::ArrayView<Drawable*> drawables);
		void SetDrawContent(PipelineContext & pipelineManager, CoreLib::List<Drawable*>& reorderBuffer, CoreLib::ArrayView<Drawable*> drawables);
	};

	class PostPassRenderTask : public RenderTask
	{
	public:
		PostRenderPass * postPass = nullptr;
		SharedModuleInstances sharedModules;
		virtual void Execute(HardwareRenderer * hw, RenderStat & stats) override;
	};

	class ImageTransferRenderTask : public RenderTask
	{
	private:
		ImageTransferRenderTask() = default;
	public:
		ImageTransferRenderTask(CoreLib::ArrayView<Texture*> renderTargetTextures, CoreLib::ArrayView<Texture*> samplingTextures);
		CoreLib::RefPtr<AsyncCommandBuffer> commandBuffer = nullptr;
		virtual void Execute(HardwareRenderer * hw, RenderStat & stats) override;
	};


	CoreLib::String GetSpireOutput(SpireDiagnosticSink * sink);
	
	struct SpireModuleStruct
	{
		int dummy;
	};

	class ShadowMapResource
	{
	private:
		int shadowMapArraySize;
		CoreLib::IntSet shadowMapArrayFreeBits;
		CoreLib::RefPtr<ViewResource> shadowView;
	public:
		CoreLib::RefPtr<Texture2DArray> shadowMapArray;
		CoreLib::RefPtr<RenderTargetLayout> shadowMapRenderTargetLayout;
		CoreLib::List<CoreLib::RefPtr<RenderOutput>> shadowMapRenderOutputs;
		int AllocShadowMaps(int count);
		void FreeShadowMaps(int id, int count);
		void Init(HardwareRenderer * hwRenderer);
		void Destroy();
		void Reset();
	};

	class RendererSharedResource
	{
	private:
		RenderAPI api;
		void LoadShaderLibrary();
		CoreLib::EnumerableDictionary<CoreLib::String, SpireShader*> entryPointShaders;
	public:
		RenderStat renderStats;
		CoreLib::RefPtr<HardwareRenderer> hardwareRenderer;
		CoreLib::RefPtr<TextureSampler> textureSampler, nearestSampler, linearSampler, shadowSampler;
		CoreLib::EnumerableDictionary<SpireModuleStruct*, CoreLib::RefPtr<DescriptorSetLayout>> descLayouts;
		SpireCompilationContext * spireContext = nullptr;
		SpireDiagnosticSink * spireSink = nullptr;
		ShadowMapResource shadowMapResources;
		SpireShader * LoadSpireShader(const char * key, const char * source);
		void CreateModuleInstance(ModuleInstance & mInst, SpireModule * shaderModule, DeviceMemory * uniformMemory, int uniformBufferSize = 0);
	public:
		CoreLib::RefPtr<TextureCube> envMap;
		CoreLib::RefPtr<Buffer> fullScreenQuadVertBuffer;
		DeviceMemory indexBufferMemory, vertexBufferMemory;
		PipelineContext pipelineManager;
	public:
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
		CoreLib::EnumerableDictionary<Mesh*, CoreLib::RefPtr<DrawableMesh>> meshes;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Texture2D>> textures;
		void CreateMaterialModuleInstance(ModuleInstance & mInst, Material* material, const char * moduleName, bool isPatternModule);
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
}
#endif