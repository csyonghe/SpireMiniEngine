#ifndef GAME_ENGINE_ATMOSPHERE_ACTOR
#define GAME_ENGINE_ATMOSPHERE_ACTOR

#include "Actor.h"
#include "Atmosphere.h"

namespace GameEngine
{
	class AtmosphereActor : public Actor
	{
	public:
		PROPERTY(VectorMath::Vec3, SunDir);
		PROPERTY_DEF(float, AtmosphericFogScaleFactor, 0.5f);
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
		AtmosphereParameters GetParameters()
		{
			AtmosphereParameters rs;
			rs.AtmosphericFogScaleFactor = AtmosphericFogScaleFactor.GetValue();
			rs.SunDir = SunDir.GetValue();
			return rs;
		}
	};
}

#endif