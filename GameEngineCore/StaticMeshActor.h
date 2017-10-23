#ifndef GAME_ENGINE_STATIC_ACTOR_H
#define GAME_ENGINE_STATIC_ACTOR_H

#include "Actor.h"
#include "RendererService.h"
#include "Model.h"

namespace GameEngine
{
	class Level;

	class StaticMeshActor : public Actor
	{
	private:
		bool useInlineMaterial = false;
		CoreLib::String inlineMeshSpec;
	protected:
		CoreLib::RefPtr<Model> model = nullptr;
		CoreLib::RefPtr<ModelPhysicsInstance> physInstance;
		ModelDrawableInstance modelInstance;
		bool localTransformChanged = true;
		virtual bool ParseField(CoreLib::String fieldName, CoreLib::Text::TokenReader & parser) override;
		virtual void SerializeFields(CoreLib::StringBuilder & sb);
		void MeshFile_Changing(CoreLib::String & newMeshFile);
		void MaterialFile_Changing(CoreLib::String & newMaterialFile);
		void ModelFile_Changing(CoreLib::String & newModelFile);
		void LocalTransform_Changing(VectorMath::Matrix4 & value);
		void ModelChanged();
	public:
		PROPERTY_ATTRIB(CoreLib::String, MeshFile, "resource(Mesh, mesh)");
		PROPERTY_ATTRIB(CoreLib::String, MaterialFile, "resource(Material, material)");
		PROPERTY_ATTRIB(CoreLib::String, ModelFile, "resource(Mesh, model)");
		Mesh * Mesh = nullptr;
		Material * MaterialInstance;
		
		virtual void OnLoad() override;
		virtual void OnUnload() override;
		virtual void GetDrawables(const GetDrawablesParameter & params) override;
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Drawable;
		}
		virtual CoreLib::String GetTypeName() override
		{
			return "StaticMesh";
		}
	};
}

#endif