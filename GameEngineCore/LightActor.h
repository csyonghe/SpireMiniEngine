#ifndef GAME_ENGINE_LIGHT_ACTOR_H
#define GAME_ENGINE_LIGHT_ACTOR_H

#include "Actor.h"
#include "RendererService.h"
#include "Model.h"
#include "Material.h"

namespace GameEngine
{
	enum class LightType
	{
		Directional, Point
	};
	class LightActor : public Actor
	{
	private:
		bool localTransformChanged = false;
		CoreLib::RefPtr<Model> model = nullptr;
		CoreLib::RefPtr<Drawable> drawable;
		CoreLib::RefPtr<ModelPhysicsInstance> physInstance;
		ModelDrawableInstance modelInstance;
		Material gizmoMaterial;
	protected:
		virtual Mesh CreateGizmoMesh() = 0;
		void LocalTransform_Changing(VectorMath::Matrix4 & value);
	public:
		LightType lightType;
		VectorMath::Vec3 GetDirection();
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Light;
		}
		virtual bool ParseField(CoreLib::String, CoreLib::Text::TokenReader &) override;
		virtual void GetDrawables(const GetDrawablesParameter & params) override;
		virtual void OnLoad() override;
	};
}

#endif