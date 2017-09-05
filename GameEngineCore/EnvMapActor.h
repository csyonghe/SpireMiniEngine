#ifndef GAME_ENGINE_ENVMAP_ACTOR_H
#define GAME_ENGINE_ENVMAP_ACTOR_H

#include "Actor.h"

namespace GameEngine
{
	class EnvMapActor : public Actor
	{
	private:
		int envMapId = -1;
	public:
		float Radius = 1000.0f;
		VectorMath::Vec3 TintColor;
		int GetEnvMapId()
		{
			return envMapId;
		}
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::EnvMap;
		}
		virtual void OnLoad() override;
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid) override;

		virtual CoreLib::String GetTypeName() override
		{
			return "EnvMap";
		}
	};
}

#endif