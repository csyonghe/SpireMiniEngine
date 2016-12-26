#ifndef GAME_ENGINE_DIRECTIONAL_LIGHT_ACTOR
#define GAME_ENGINE_DIRECTIONAL_LIGHT_ACTOR

#include "LightActor.h"

namespace GameEngine
{
	class DirectionalLightActor : public LightActor
	{
	public:
		bool EnableCascadedShadows = false;
		float ShadowDistance = 40000.0f;
		int NumShadowCascades = 1;
		float TransitionFactor = 0.8f;
		VectorMath::Vec3 Direction = Vec3::Create(0.0f, 1.0f, 0.0f);
		VectorMath::Vec3 Color = Vec3::Create(1.0f, 1.0f, 1.0f);
		virtual CoreLib::String GetTypeName() override
		{
			return "DirectionalLight";
		}
		DirectionalLightActor()
		{
			lightType = LightType::Directional;
		}
	protected:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid) override;

	};
}

#endif