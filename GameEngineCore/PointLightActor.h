#ifndef GAME_ENGINE_POINT_LIGHT_ACTOR
#define GAME_ENGINE_POINT_LIGHT_ACTOR

#include "LightActor.h"

namespace GameEngine
{
	class PointLightActor : public LightActor
	{
	public:
		bool EnableShadows = false;
		float Radius = 2000.0f;
		float DecayDistance90Percent = 1200.0f;
		bool IsSpotLight = false;
		VectorMath::Vec3 Direction = Vec3::Create(0.0f, 1.0f, 0.0f);
		VectorMath::Vec3 Color = Vec3::Create(1.0f, 1.0f, 1.0f);
		float SpotLightStartAngle = 0.0f, SpotLightEndAngle = 0.0f;
		virtual CoreLib::String GetTypeName() override
		{
			return "PointLight";
		}
		PointLightActor()
		{
			lightType = LightType::Point;
		}
	protected:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid) override;

	};
}

#endif