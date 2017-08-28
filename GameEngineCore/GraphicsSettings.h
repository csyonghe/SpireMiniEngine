#ifndef GAME_ENGINE_GRAPHICS_SETTINGS_H
#define GAME_ENGINE_GRAPHICS_SETTINGS_H

#include "CoreLib/Basic.h"

namespace GameEngine
{
	class GraphicsSettings
	{
	public:
		int ShadowMapArraySize = 8;
		int ShadowMapResolution = 1024;
		bool UsePipelineCache = true;
		void LoadFromFile(CoreLib::String fileName);
		void SaveToFile(CoreLib::String fileName);
	};
}

#endif