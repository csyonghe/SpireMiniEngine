#ifndef GAME_ENGINE_TONE_MAPPING_H
#define GAME_ENGINE_TONE_MAPPING_H

#include "CoreLib/VectorMath.h"

namespace GameEngine
{
	struct ToneMappingParameters
	{
		float Exposure = 0.6f;
		float padding[3];
		bool operator == (const ToneMappingParameters & other) const
		{
			return Exposure == other.Exposure;
		}
	};
}

#endif