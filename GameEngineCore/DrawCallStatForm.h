#ifndef GAME_ENGINE_DRAW_CALL_STAT_FORM_H
#define GAME_ENGINE_DRAW_CALL_STAT_FORM_H

#include "CoreLib/LibUI/LibUI.h"

namespace GameEngine
{
	class DrawCallStatForm : public GraphicsUI::Form
	{
	private:
		float minMs;
		float maxMs;
		GraphicsUI::Label * lblNumDrawCalls;
		GraphicsUI::Label * lblNumWorldPasses;
		GraphicsUI::Label * lblFps;
		GraphicsUI::Label * lblCpuTime;
		GraphicsUI::Label * lblPipelineLookupTime;

	public:
		DrawCallStatForm(GraphicsUI::UIEntry * parent);
		void SetNumDrawCalls(int val);
		void SetNumWorldPasses(int val);
		void SetCpuTime(float time, float pipelineLookupTime);
		void SetFrameRenderTime(float val);

	};
}

#endif