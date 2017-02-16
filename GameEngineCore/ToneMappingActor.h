#ifndef GAME_ENGINE_TONE_MAPPING_ACTOR_H
#define GAME_ENGINE_TONE_MAPPING_ACTOR_H

#include "Actor.h"
#include "ToneMapping.h"

namespace GameEngine
{
	class ToneMappingActor : public Actor
	{
	public:
		ToneMappingParameters Parameters;
		virtual CoreLib::String GetTypeName() override
		{
			return "ToneMapping";
		}
		ToneMappingActor()
		{
			Bounds.Init();
		}
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::ToneMapping;
		}
	protected:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid) override;

	};
}

#endif