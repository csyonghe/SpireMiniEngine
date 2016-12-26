#ifndef GAME_ENGINE_DRAW_CALL_STAT_FORM_H
#define GAME_ENGINE_DRAW_CALL_STAT_FORM_H

#include "CoreLib/LibUI/LibUI.h"

namespace GameEngine
{
	class DrawCallStatForm : public GraphicsUI::Form
	{
	private:
		GraphicsUI::Label * lblNumDrawCalls;
		GraphicsUI::Label * lblNumWorldPasses;
		GraphicsUI::Label * lblFps;

	public:
		DrawCallStatForm(GraphicsUI::UIEntry * parent);
		void SetNumDrawCalls(int val);
		void SetNumWorldPasses(int val);
		void SetFrameRenderTime(float val);

	};
}

#endif