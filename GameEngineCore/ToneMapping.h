#ifndef GAME_ENGINE_TONE_MAPPING_H
#define GAME_ENGINE_TONE_MAPPING_H

#include "CoreLib/VectorMath.h"

namespace GameEngine
{
	class Texture3D;

	struct ToneMappingParameters
	{
		float Exposure = 0.6f;
		float padding[3];
		Texture3D* lookupTexture = nullptr;
		bool operator == (const ToneMappingParameters & other) const
		{
			return Exposure == other.Exposure && lookupTexture == other.lookupTexture;
		}
	};
}

#endif