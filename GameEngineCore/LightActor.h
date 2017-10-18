#ifndef GAME_ENGINE_LIGHT_ACTOR_H
#define GAME_ENGINE_LIGHT_ACTOR_H

#include "GizmoActor.h"

namespace GameEngine
{
	enum class LightType
	{
		Directional, Point
	};
	class LightActor : public GizmoActor
	{
	protected:
		virtual Mesh CreateGizmoMesh() = 0;
	public:
		LightType lightType;
		VectorMath::Vec3 GetDirection();
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Light;
		}
		virtual bool ParseField(CoreLib::String, CoreLib::Text::TokenReader &) override;
		virtual void OnLoad() override;
	};
}

#endif