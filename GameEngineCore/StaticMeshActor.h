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
		CoreLib::String materialFileName;
		CoreLib::String modelFileName;
	protected:
		CoreLib::RefPtr<Model> model = nullptr;
		CoreLib::RefPtr<Drawable> drawable;
		CoreLib::RefPtr<ModelPhysicsInstance> physInstance;
		ModelDrawableInstance modelInstance;
		bool localTransformChanged = true;
		virtual bool ParseField(CoreLib::String fieldName, CoreLib::Text::TokenReader & parser) override;
		virtual void SerializeFields(CoreLib::StringBuilder & sb);
	public:
		CoreLib::String MeshName;
		Mesh * Mesh = nullptr;
		Material * MaterialInstance;
		
		virtual void OnLoad() override;
		virtual void OnUnload() override;
		virtual void GetDrawables(const GetDrawablesParameter & params) override;
		virtual void SetLocalTransform(const VectorMath::Matrix4 & val) override;
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