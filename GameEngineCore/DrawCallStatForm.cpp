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
		lblCpuTime = new Label(this);
		lblPipelineLookupTime = new Label(this);

		lblFps->Posit(emToPixel(0.5f), emToPixel(0.5f), emToPixel(20.0f), emToPixel(1.5f));
		lblNumWorldPasses->Posit(emToPixel(0.5f), emToPixel(1.5f), emToPixel(20.0f), emToPixel(1.5f));
		lblNumDrawCalls->Posit(emToPixel(0.5f), emToPixel(2.5f), emToPixel(20.0f), emToPixel(1.5f));
		lblCpuTime->Posit(emToPixel(0.5f), emToPixel(3.5f), emToPixel(20.0f), emToPixel(1.5f));
		lblPipelineLookupTime->Posit(emToPixel(0.5f), emToPixel(4.5f), emToPixel(20.0f), emToPixel(1.5f));
		SetWidth(emToPixel(14.0f));
		SetHeight(emToPixel(8.2f));
	}

	void DrawCallStatForm::SetNumDrawCalls(int val)
	{
		lblNumDrawCalls->SetText("Draw Calls: " + CoreLib::String(val));
	}

	void DrawCallStatForm::SetNumWorldPasses(int val)
	{
		lblNumWorldPasses->SetText("Passes: " + CoreLib::String(val));
	}

	void DrawCallStatForm::SetCpuTime(float time, float pipelineLookupTime)
	{
		CoreLib::StringBuilder sb(256);
		sb << "Renderer CPU: " << CoreLib::String(time*1000.0f, "%.1f") << "ms";
		lblCpuTime->SetText(sb.ToString());
		sb.Clear();
		sb << "PipelineLookup: " << CoreLib::String(pipelineLookupTime * 1000.0f, "%.1f") << "ms";
		lblPipelineLookupTime->SetText(sb.ToString());
	}

	void DrawCallStatForm::SetFrameRenderTime(float val)
	{
		static int i = 0;
		if (i < 10)
		{
			maxMs = 0.0f;
			minMs = 100.0f;
			++i;
		}
		if (val > maxMs) maxMs = val;
		if (val < minMs) minMs = val;
		lblFps->SetText(CoreLib::String((int)(1.0f / val)) + " fps (" + CoreLib::String(val * 1000.0f, "%.1f") + "ms)" + " [" + CoreLib::String(minMs * 1000.0f, "%.1f") + " - " + CoreLib::String(maxMs * 1000.0f, "%.1f") + "ms]");
	}

}

