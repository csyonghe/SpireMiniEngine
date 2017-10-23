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
		PROPERTY_DEF(float, Radius, 1000.0f);
		PROPERTY_DEF(VectorMath::Vec3, TintColor, VectorMath::Vec3::Create(1.0f));
		int GetEnvMapId()
		{
			return envMapId;
		}
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::EnvMap;
		}
		virtual void OnLoad() override;

		virtual CoreLib::String GetTypeName() override
		{
			return "EnvMap";
		}
	};
}

#endif