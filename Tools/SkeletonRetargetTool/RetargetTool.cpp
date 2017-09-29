#include "Engine.h"
#include "CoreLib/WinForm/WinForm.h"
#include "CoreLib/WinForm/WinApp.h"
#include "CoreLib/LibUI/LibUI.h"
#include "SystemWindow.h"

using namespace CoreLib::WinForm;
using namespace GameEngine;

void InitUI(SystemWindow * window)
{
	window->SetText("Skeleton Retargeting Tool");
}

String RemoveQuote(String dir)
{
	if (dir.StartsWith("\""))
		return dir.SubString(1, dir.Length() - 2);
	return dir;
}

const char * levelSrc = R"(
Camera
{
	name "FreeCam"
	orientation[-0.044 0.000 0.000]
	position[18.77 108.36 280.13]
	znear 5.0
	zfar 8000.0
	fov 60.0
}

ArcBallCameraController
{
	name "CamControl"
	TargetCamera "FreeCam"
	Radius 500.00
	Center[0.0 50.0 0.0]
	Alpha 1.57
	Beta 0.439
	NeedAlt false
}

DirectionalLight
{
	name "sunlight"
	transform[1 0 0 0   0 1 0 0    0 0 1 0    10000 10000 0 1]
	Direction[0.8 0.7 0.3]
	Color[3.1 3.1 3.1]
	EnableCascadedShadows true
	NumShadowCascades 8
	ShadowDistance 4500
	TransitionFactor 0.8
}

EnvMap
{
	name "envMap"
	transform[1 0 0 0   0 1 0 0    0 0 1 0    0 5000 0 1]
    Radius 2000000.0
	TintColor [0.7 0.7 0.7]
}

ToneMapping
{
	name "tonemapping"
	Exposure 0.7
}
Atmosphere
{
	name "atmosphere"
	AtmosphericFogScaleFactor 0.01
	SunDir[0.8 0.7 0.3]
}
SkeletonRetargetVisualizer
{}
)";

void RegisterRetargetActor();

int wWinMain(
	HINSTANCE /*hInstance*/,
	HINSTANCE /*hPrevInstance*/,
	LPWSTR     /*lpCmdLine*/,
	int       /*nCmdShow*/
)
{
	Application::Init();
	try
	{
		EngineInitArguments args;
		auto & appParams = args.LaunchParams;
		int w = 1920;
		int h = 1080;

		args.API = RenderAPI::OpenGL;
		args.GpuId = 0;
		args.Width = w;
		args.Height = h;
		args.NoConsole = true;
		CommandLineParser parser(Application::GetCommandLine());
		if (parser.OptionExists("-vk"))
			args.API = RenderAPI::Vulkan;
		if (parser.OptionExists("-dir"))
			args.GameDirectory = RemoveQuote(parser.GetOptionValue("-dir"));
		if (parser.OptionExists("-enginedir"))
			args.EngineDirectory = RemoveQuote(parser.GetOptionValue("-enginedir"));
		args.RecompileShaders = false;
		{
			Engine::Instance()->Init(args);
			RegisterRetargetActor();
			InitUI(Engine::Instance()->GetMainWindow());
			Engine::Instance()->LoadLevelFromText(levelSrc);
			
			Engine::Run();
			Engine::Destroy();
		}
	}
	catch (...)
	{
	}
	Application::Dispose();
}