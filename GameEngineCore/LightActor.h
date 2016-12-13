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

		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Light;
		}

	};
}

#endif