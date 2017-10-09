#ifndef GAME_ENGINE_ATMOSPHERE_H
#define GAME_ENGINE_ATMOSPHERE_H

#include "CoreLib/VectorMath.h"
#include "CoreLib/Basic.h"
#include "CoreLib/Tokenizer.h"

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
		bool operator == (const AtmosphereParameters & other) const
		{
			return const_cast<VectorMath::Vec3&>(SunDir) == other.SunDir && AtmosphericFogScaleFactor == other.AtmosphericFogScaleFactor;
		}
		void Parse(CoreLib::Text::TokenReader & parser);
		void Serialize(CoreLib::StringBuilder & sb);
	};
}

#endif