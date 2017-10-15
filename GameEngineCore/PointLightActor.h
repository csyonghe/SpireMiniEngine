#ifndef GAME_ENGINE_POINT_LIGHT_ACTOR
#define GAME_ENGINE_POINT_LIGHT_ACTOR

#include "LightActor.h"
#include "CoreLib/VectorMath.h"

namespace GameEngine
{
	class PointLightActor : public LightActor
	{
	public:
		PROPERTY_DEF(bool, EnableShadows, false);
		PROPERTY_DEF(float, Radius, 2000.0f);
		PROPERTY_DEF(float, DecayDistance90Percent, 1200.0f);
		PROPERTY_DEF(bool, IsSpotLight, false);
		PROPERTY_DEF(VectorMath::Vec3, Color, VectorMath::Vec3::Create(1.0f, 1.0f, 1.0f));
		PROPERTY_DEF(float, SpotLightStartAngle, 0.0f);
		PROPERTY_DEF(float, SpotLightEndAngle, 0.0f);
		virtual CoreLib::String GetTypeName() override
		{
			return "PointLight";
		}
		PointLightActor()
		{
			lightType = LightType::Point;
		}

	};
}

#endif