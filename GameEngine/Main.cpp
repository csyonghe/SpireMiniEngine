#include "CoreLib/Basic.h"
#include "CoreLib/PerformanceCounter.h"
#include "CoreLib/WinForm/WinForm.h"
#include "CoreLib/WinForm/WinApp.h"
#include "CoreLib/CommandLineParser.h"
#include "HardwareRenderer.h"
#include "Engine.h"
#include "CoreLib/Imaging/Bitmap.h"

namespace GameEngine
{
	using namespace CoreLib;
	using namespace CoreLib::WinForm;

	struct VideoRecordingParameters
	{
		bool Enabled = false;
		String Directory;
		float Length = 10.0f;
		int FramesPerSecond = 30;
	};

	class MainForm : public CoreLib::WinForm::Form
	{
	private:
		VideoRecordingParameters videoRecParams;
	protected:
		int ProcessMessage(WinMessage & msg) override
		{
			auto engine = Engine::Instance();
			int rs = -1;
			if (engine)
			{
				rs = engine->HandleWindowsMessage(msg.hWnd, msg.message, msg.wParam, msg.lParam);
			}
			if (rs == -1)
				return BaseForm::ProcessMessage(msg);
			return rs;
		}
	public:
		MainForm(const VideoRecordingParameters & pVideoRecParams)
		{
			wantChars = true;
			videoRecParams = pVideoRecParams;
			SetText("Game Engine");
			OnResized.Bind(this, &MainForm::FormResized);
			OnFocus.Bind([](auto, auto) {Engine::Instance()->EnableInput(true); });
			OnLostFocus.Bind([](auto, auto) {Engine::Instance()->EnableInput(false); });
			SetClientWidth(1920);
			SetClientHeight(1080);
		}
		
		void FormResized(Object *, EventArgs)
		{
			Engine::Instance()->Resize(GetClientWidth(), GetClientHeight());
		}
		void MainLoop(Object *, EventArgs)
		{
			static int frameId = 0;
			auto instance = Engine::Instance();
			if (instance)
			{
				instance->Tick();
				if (videoRecParams.Enabled)
				{
					auto image = instance->GetRenderResult(true);
					Engine::SaveImage(image, CoreLib::IO::Path::Combine(videoRecParams.Directory, String(frameId) + ".bmp"));
					if (Engine::Instance()->GetTime() >= videoRecParams.Length)
						Close();
				}
				frameId++;
			}
		}
	};

}

using namespace GameEngine;
using namespace CoreLib::WinForm;

#define COMMAND true
#define WINDOWED !COMMAND

String RemoveQuote(String dir)
{
	if (dir.StartsWith("\""))
		return dir.SubString(1, dir.Length() - 2);
	return dir;
}

void RegisterTestUserActor();

#if COMMAND
int wmain(int /*argc*/, const wchar_t ** /*argv*/)
#elif WINDOWED
int wWinMain(
	_In_ HINSTANCE /*hInstance*/,
	_In_ HINSTANCE /*hPrevInstance*/,
	_In_ LPWSTR     /*lpCmdLine*/,
	_In_ int       /*nCmdShow*/
)
#endif
{
	Application::Init();
	{
		EngineInitArguments args;
		VideoRecordingParameters videoRecParams;

		args.API = RenderAPI::OpenGL;
		args.GpuId = 0;
		args.RecompileShaders = false;

		CommandLineParser parser(Application::GetCommandLine());
		if(parser.OptionExists("-vk"))
			args.API = RenderAPI::Vulkan;
		if (parser.OptionExists("-dir"))
			args.GameDirectory = RemoveQuote(parser.GetOptionValue("-dir"));
		if (parser.OptionExists("-enginedir"))
			args.EngineDirectory = RemoveQuote(parser.GetOptionValue("-enginedir"));
		if (parser.OptionExists("-gpu"))
			args.GpuId = StringToInt(parser.GetOptionValue("-gpu"));
		if (parser.OptionExists("-recompileshaders"))
			args.RecompileShaders = true;
		if (parser.OptionExists("-level"))
			args.StartupLevelName = parser.GetOptionValue("-level");
		if (parser.OptionExists("-recdir"))
		{
			videoRecParams.Enabled = true;
			videoRecParams.Directory = RemoveQuote(parser.GetOptionValue("-recdir"));
		}
		if (parser.OptionExists("-reclen"))
		{
			videoRecParams.Enabled = true;
			videoRecParams.Length = (float)StringToDouble(parser.GetOptionValue("-reclen"));
		}
		if (parser.OptionExists("-recfps"))
		{
			videoRecParams.Enabled = true;
			videoRecParams.FramesPerSecond = (int)StringToInt(parser.GetOptionValue("-recfps"));
		}
        if (parser.OptionExists("-no_console"))
            args.NoConsole = true;
		auto form = new MainForm(videoRecParams);
		Application::SetMainLoopEventHandler(new CoreLib::WinForm::NotifyEvent(form, &MainForm::MainLoop));

		args.Width = form->GetClientWidth();
		args.Height = form->GetClientHeight();
		args.Window = (WindowHandle)form->GetHandle();
		
		RegisterTestUserActor();

		Engine::Init(args);
		
		if (videoRecParams.Enabled)
		{
			Engine::Instance()->SetTimingMode(GameEngine::TimingMode::Fixed);
			Engine::Instance()->SetFrameDuration(1.0f / videoRecParams.FramesPerSecond);
		}

		Application::Run(form, true);
		
		Engine::Destroy();
	}
	Application::Dispose();
}