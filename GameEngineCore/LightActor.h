#ifndef GAME_ENGINE_LIGHT_ACTOR_H
#define GAME_ENGINE_LIGHT_ACTOR_H

#include "Actor.h"

namespace GameEngine
{
	enum class LightType
	{
		Directional, Point
	};
	class LightActor : public Actor
	{
	public:
		LightType lightType;
		VectorMath::Vec3 GetDirection();
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Light;
		}
		virtual bool ParseField(CoreLib::String, CoreLib::Text::TokenReader &) override;
	};
}

#endif