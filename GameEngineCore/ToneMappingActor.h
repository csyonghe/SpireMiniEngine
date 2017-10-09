#ifndef GAME_ENGINE_TONE_MAPPING_ACTOR_H
#define GAME_ENGINE_TONE_MAPPING_ACTOR_H

#include "Actor.h"
#include "ToneMapping.h"

namespace GameEngine
{
	class ToneMappingActor : public Actor
	{
	private:
		CoreLib::String lookupTextureFileName = "";
	public:
		ToneMappingParameters Parameters;
		CoreLib::RefPtr<Texture3D> lookupTexture;
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
		void LoadColorLookupTexture(CoreLib::String fileName);
		virtual bool ParseField(CoreLib::String fieldName, CoreLib::Text::TokenReader & parser) override;
		virtual void SerializeFields(CoreLib::StringBuilder & sb) override;
	};
}

#endif