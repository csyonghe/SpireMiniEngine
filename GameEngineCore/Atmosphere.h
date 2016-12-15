#ifndef GAME_ENGINE_ATMOSPHERE_H
#define GAME_ENGINE_ATMOSPHERE_H

#include "CoreLib/VectorMath.h"

namespace GameEngine
{
	struct AtmosphereParameters
	{
		VectorMath::Vec3 SunDir; float padding = 0.0f;
		float AtmosphericFogScaleFactor = 0.5f;
		AtmosphereParameters()
		{
			SunDir = VectorMath::Vec3::Create(1.0f, 1.0f, 0.5f).Normalize();
		}
	};
}

#endif