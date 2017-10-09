#ifndef GAME_ENGINE_ATMOSPHERE_ACTOR
#define GAME_ENGINE_ATMOSPHERE_ACTOR

#include "Actor.h"
#include "Atmosphere.h"

namespace GameEngine
{
	class AtmosphereActor : public Actor
	{
	public:
		PROPERTY(AtmosphereParameters, Parameters);
		virtual CoreLib::String GetTypeName() override
		{
			return "Atmosphere";
		}
		AtmosphereActor()
		{
			Bounds.Init();
		}
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Atmosphere;
		}
		virtual void OnLoad() override;
	};
}

#endif