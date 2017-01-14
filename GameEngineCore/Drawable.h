#ifndef GAME_ENGINE_DRAWABLE_H
#define GAME_ENGINE_DRAWABLE_H

#include "HardwareRenderer.h"
#include "Mesh.h"
#include "EngineLimits.h"

namespace GameEngine
{
	class RendererSharedResource;
	class Material;
	class Skeleton;
	class ModuleInstance;
	class PipelineClass;
	class PipelineContext;
	class SceneResource;
	class Pose;

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
		DrawableType type = DrawableType::Static;
		MeshVertexFormat vertFormat;
		CoreLib::RefPtr<DrawableMesh> mesh = nullptr;
		Material * material = nullptr;
		ModuleInstance * transformModule = nullptr;
		Skeleton * skeleton = nullptr;
		CoreLib::Array<PipelineClass*, MaxWorldRenderPasses> pipelineCache;
		SceneResource * scene = nullptr;
	public:
		CoreLib::Graphics::BBox Bounds;
		unsigned int ReorderKey = 0;
		Drawable(SceneResource * sceneRes);
		~Drawable();
		PipelineClass * GetPipeline(int passId, PipelineContext & pipelineManager);
		inline ModuleInstance * GetTransformModule()
		{
			return transformModule;
		}
		bool IsTransparent();
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

	class DrawableSink
	{
	private:
		CoreLib::List<Drawable*> opaqueDrawables;
		CoreLib::List<Drawable*> transparentDrawables;

	public:
		void AddDrawable(Drawable * drawable)
		{
			if (drawable->IsTransparent())
				transparentDrawables.Add(drawable);
			else
				opaqueDrawables.Add(drawable);
			drawable->UpdateMaterialUniform();
		}
		void Clear()
		{
			opaqueDrawables.Clear();
			transparentDrawables.Clear();
		}
		CoreLib::ArrayView<Drawable*> GetDrawables(bool transparent)
		{
			return transparent ? transparentDrawables.GetArrayView() : opaqueDrawables.GetArrayView();
		}
	};
}

#endif