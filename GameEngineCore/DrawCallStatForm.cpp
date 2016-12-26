#include "DrawCallStatForm.h"

using namespace GraphicsUI;

namespace GameEngine
{
	DrawCallStatForm::DrawCallStatForm(UIEntry * parent)
		: Form(parent)
	{
		SetText("Draw Stats");
		lblFps = new Label(this);
		lblNumDrawCalls = new Label(this);
		lblNumWorldPasses = new Label(this);
		lblFps->Posit(emToPixel(0.5f), emToPixel(0.5f), emToPixel(20.0f), emToPixel(1.5f));
		lblNumWorldPasses->Posit(emToPixel(0.5f), emToPixel(1.5f), emToPixel(20.0f), emToPixel(1.5f));
		lblNumDrawCalls->Posit(emToPixel(0.5f), emToPixel(2.5f), emToPixel(20.0f), emToPixel(1.5f));
		SetWidth(emToPixel(10.0f));
		SetHeight(emToPixel(6.2f));
	}

	void DrawCallStatForm::SetNumDrawCalls(int val)
	{
		lblNumDrawCalls->SetText("Draw Calls: " + CoreLib::String(val));
	}

	void DrawCallStatForm::SetNumWorldPasses(int val)
	{
		lblNumWorldPasses->SetText("Passes: " + CoreLib::String(val));
	}

	void DrawCallStatForm::SetFrameRenderTime(float val)
	{
		lblFps->SetText(CoreLib::String((int)(1.0f / val)) + " fps (" + CoreLib::String(val * 1000.0f, "%.1f") + "ms)");
	}

}

