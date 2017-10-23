#ifndef GAME_ENGINE_COMPUTE_PASS_H
#define GAME_ENGINE_COMPUTE_PASS_H

#include "HardwareRenderer.h"

namespace GameEngine
{
	class LightBinningPassImpl;

	class LightBinningPass
	{
	private:
		LightBinningPassImpl * impl;
	public:
		void Execute(HardwareRenderer * renderer);
	};
}

#endif