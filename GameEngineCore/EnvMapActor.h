#ifndef GAME_ENGINE_ENVMAP_ACTOR_H
#define GAME_ENGINE_ENVMAP_ACTOR_H

#include "Actor.h"

namespace GameEngine
{
	class EnvMapActor : public Actor
	{
	private:
		int envMapId;
	public:
		int GetEnvMapId()
		{
			return envMapId;
		}
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::EnvMap;
		}
		virtual void OnLoad() override;
	};
}

#endif