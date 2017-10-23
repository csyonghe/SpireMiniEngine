#ifndef GAME_ENGINE_TONE_MAPPING_ACTOR_H
#define GAME_ENGINE_TONE_MAPPING_ACTOR_H

#include "Actor.h"
#include "ToneMapping.h"

namespace GameEngine
{
	class ToneMappingActor : public Actor
	{
	private:
        void ColorLUT_Changing(CoreLib::String & newFileName);
        void Exposure_Changed();
	public:
        PROPERTY_DEF(float, Exposure, 0.7f);
        PROPERTY_ATTRIB(CoreLib::String, ColorLUT, "resource(Texture, clut)");
		ToneMappingParameters Parameters;
		CoreLib::UniquePtr<Texture3D> lookupTexture;
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
		bool LoadColorLookupTexture(CoreLib::String fileName);
        virtual void OnLoad() override;
        ~ToneMappingActor();
	};
}

#endif