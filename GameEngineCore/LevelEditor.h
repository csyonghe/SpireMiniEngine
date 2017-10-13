#ifndef GAME_ENGINE_LEVEL_EDITOR_H
#define GAME_ENGINE_LEVEL_EDITOR_H

#include "CoreLib/Basic.h"

namespace GameEngine
{
	class LevelEditor : public CoreLib::RefObject
	{
	public:
		virtual void Tick() = 0;
		virtual void OnLoad() = 0;
	};

	LevelEditor * CreateLevelEditor();
}

#endif