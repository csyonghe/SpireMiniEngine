#ifndef GAME_ENGINE_ATMOSPHERE_ACTOR
#define GAME_ENGINE_ATMOSPHERE_ACTOR

#include "Actor.h"
#include "Atmosphere.h"

namespace GameEngine
{
	class AtmosphereActor : public Actor
	{
	public:
		AtmosphereParameters Parameters;
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
	protected:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid) override;

	};
}

#endif