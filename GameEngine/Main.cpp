#include "CoreLib/Basic.h"
#include "CoreLib/PerformanceCounter.h"
#include "CoreLib/WinForm/WinForm.h"
#include "CoreLib/WinForm/WinApp.h"
#include "CoreLib/CommandLineParser.h"
#include "HardwareRenderer.h"
#include "Engine.h"
#include "CoreLib/Imaging/Bitmap.h"

using namespace GameEngine;
using namespace CoreLib::WinForm;

#define COMMAND false
#define WINDOWED !COMMAND

String RemoveQuote(String dir)
{
	if (dir.StartsWith("\""))
		return dir.SubString(1, dir.Length() - 2);
	return dir;
}

void RegisterTestUserActor();

#include "FrustumCulling.h"

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
		auto & appParams = args.LaunchParams;
		try
		{
			int w = 1920;
			int h = 1080;

			args.API = RenderAPI::OpenGL;
			args.GpuId = 0;
			args.RecompileShaders = false;

			CommandLineParser parser(Application::GetCommandLine());
			if (parser.OptionExists("-vk"))
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
				appParams.EnableVideoCapture = true;
				appParams.Directory = RemoveQuote(parser.GetOptionValue("-recdir"));
			}
			if (parser.OptionExists("-reclen"))
			{
				appParams.EnableVideoCapture = true;
				appParams.Length = (float)StringToDouble(parser.GetOptionValue("-reclen"));
			}
			if (parser.OptionExists("-recfps"))
			{
				appParams.EnableVideoCapture = true;
				appParams.FramesPerSecond = (int)StringToInt(parser.GetOptionValue("-recfps"));
			}
			if (parser.OptionExists("-no_console"))
				args.NoConsole = true;
			if (parser.OptionExists("-runforframes"))
				appParams.RunForFrames = (int)StringToInt(parser.GetOptionValue("-runforframes"));
			if (parser.OptionExists("-dumpstat"))
			{
				appParams.DumpRenderStats = true;
				appParams.RenderStatsDumpFileName = RemoveQuote(parser.GetOptionValue("-dumpstat"));
			}
			if (parser.OptionExists("-width"))
			{
				w = StringToInt(parser.GetOptionValue("-width"));
			}
			if (parser.OptionExists("-height"))
			{
				h = StringToInt(parser.GetOptionValue("-height"));
			}

            args.Width = w;
			args.Height = h;

			RegisterTestUserActor();

			Engine::Init(args);

			if (parser.OptionExists("-pipelinecache"))
			{
				Engine::Instance()->GetGraphicsSettings().UsePipelineCache = ((int)StringToInt(parser.GetOptionValue("-pipelinecache")) == 1);
			}

			if (appParams.EnableVideoCapture)
			{
				Engine::Instance()->SetTimingMode(GameEngine::TimingMode::Fixed);
				Engine::Instance()->SetFrameDuration(1.0f / appParams.FramesPerSecond);
			}
			Engine::Run();
		}
		catch (const Exception & e)
		{
			MessageBoxW(NULL, e.Message.ToWString(), NULL, NULL);
		}
		Engine::Destroy();
	}
	Application::Dispose();
}