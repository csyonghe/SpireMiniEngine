#include "Engine.h"
#include "SkeletalMeshActor.h"
#include "FreeRoamCameraController.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Tokenizer.h"

#ifndef DWORD
typedef unsigned long DWORD;
#endif
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

namespace GameEngine
{
	Engine * Engine::instance = nullptr;

	using namespace CoreLib;
	using namespace CoreLib::IO;
	using namespace CoreLib::Diagnostics;
	using namespace GraphicsUI;

	void RegisterEngineActorClasses(Engine *);

    String RemoveQuote(String str)
    {
        if (str.Length() >= 2 && str.StartsWith("\""))
            return str.SubString(1, str.Length() - 2);
        return str;

    }

	void Engine::InternalInit(const EngineInitArguments & args)
	{
		try
		{
			RegisterEngineActorClasses(this);


			startTime = lastGameLogicTime = lastRenderingTime = Diagnostics::PerformanceCounter::Start();

			GpuId = args.GpuId;

			gameDir = args.GameDirectory;
			engineDir = args.EngineDirectory;
			Path::CreateDir(Path::Combine(gameDir, "Cache/Shaders"));
			Path::CreateDir(Path::Combine(gameDir, "Settings"));
			Path::CreateDir(Path::Combine(gameDir, "Settings"));


			auto graphicsSettingsFile = FindFile("graphics.settings", ResourceType::Settings);
			if (graphicsSettingsFile.Length())
				graphicsSettings.LoadFromFile(graphicsSettingsFile);

			// initialize input dispatcher
			inputDispatcher = new InputDispatcher(CreateHardwareInputInterface(args.Window));
			auto bindingFile = Path::Combine(gameDir, "bindings.config");
			if (File::Exists(bindingFile))
				inputDispatcher->LoadMapping(bindingFile);

			// initialize renderer
			renderer = CreateRenderer(args.Window, args.API);
			renderer->Resize(args.Width, args.Height);

			uiSystemInterface = new UIWindowsSystemInterface(renderer->GetHardwareRenderer());
			Global::Colors = CreateDarkColorTable();
			uiEntry = new UIEntry(args.Width, args.Height, uiSystemInterface.Ptr());
			uiSystemInterface->SetEntry(uiEntry.Ptr());
			uiCommandForm = new CommandForm(uiEntry.Ptr());
			uiCommandForm->OnCommand.Bind(this, &Engine::OnCommand);
            if (!args.NoConsole)
                uiEntry->ShowWindow(uiCommandForm);
            else
                uiEntry->CloseWindow(uiCommandForm);
			auto configFile = Path::Combine(gameDir, "game.config");
			String levelToLoad = RemoveQuote(args.StartupLevelName);
			if (File::Exists(configFile))
			{
				CoreLib::Text::TokenReader parser(File::ReadAllText(configFile));
				if (parser.LookAhead("DefaultLevel"))
				{
					parser.ReadToken();
					parser.Read("=");
                    auto defaultLevelName = parser.ReadStringLiteral();
					if (args.StartupLevelName.Length() == 0)
						levelToLoad = defaultLevelName;
				}
			}
			LoadLevel(levelToLoad);
		}
		catch (const Exception & e)
		{
			MessageBox(NULL, e.Message.ToWString(), L"Error", MB_ICONEXCLAMATION);
			exit(1);
		}
	}

	Engine::~Engine()
	{
		renderer->Wait();
		level = nullptr;
		uiEntry = nullptr;
		uiSystemInterface = nullptr;
		renderer = nullptr;
	}

	void Engine::SaveGraphicsSettings()
	{
		auto graphicsSettingsFile = Path::Combine(gameDir, "Settings/graphics.settings");
		graphicsSettings.SaveToFile(graphicsSettingsFile);
	}

	float Engine::GetTimeDelta(EngineThread thread)
	{
		if (timingMode == TimingMode::Natural)
		{
			if (thread == EngineThread::GameLogic)
				return gameLogicTimeDelta;
			else
				return renderingTimeDelta;
		}
		else
		{
			return fixedFrameDuration;
		}
	}

	static float aggregateTime = 0.0f;
	static int frameCount = 0;

	void Engine::Tick()
	{
		auto thisGameLogicTime = PerformanceCounter::Start();
		gameLogicTimeDelta = PerformanceCounter::EndSeconds(lastGameLogicTime);

		if (enableInput && !uiEntry->KeyInputConsumed)
			inputDispatcher->DispatchInput();

		if (level)
		{
			for (auto & actor : level->Actors)
				actor.Value->Tick();
		}
		lastGameLogicTime = thisGameLogicTime;

		auto thisRenderingTime = PerformanceCounter::Start();
		renderingTimeDelta = PerformanceCounter::EndSeconds(lastRenderingTime);
		renderer->TakeSnapshot();
		renderer->RenderFrame();
		lastRenderingTime = thisRenderingTime;

		frameCount++;
		aggregateTime += renderingTimeDelta;

		if (aggregateTime > 1.0f)
		{
			printf("                                  \r");
			printf("%.2f ms (%u FPS)\r", aggregateTime/frameCount*1000.0f, (int)(frameCount / aggregateTime));
		
			aggregateTime = 0.0f;
			frameCount = 0;
		}

		auto uiCommands = uiEntry->DrawUI();
		uiSystemInterface->ExecuteDrawCommands(renderer->GetRenderedImage(), uiCommands);
		renderer->GetHardwareRenderer()->Present(uiSystemInterface->GetRenderedImage());
		//renderer->GetHardwareRenderer()->Present(renderer->GetRenderedImage());
		frameCounter++;
	}

	void Engine::Resize(int w, int h)
	{
		if (renderer && w > 2 && h > 2)
		{
			renderer->Resize(w, h);
			uiSystemInterface->SetResolution(w, h);
		}
	}

	void Engine::EnableInput(bool value)
	{
		enableInput = value;
	}

	void Engine::OnCommand(CoreLib::String command)
	{
		CoreLib::Text::TokenReader parser(command);
		if (parser.LookAhead("spawn"))
		{
			parser.ReadToken();
			auto typeName = parser.ReadWord();
			if (level)
			{
				auto actor = CreateActor(typeName);
				if (actor)
				{
					actor->Name = String("TestUser") + String(level->Actors.Count());
					level->RegisterActor(actor);
				}
				else
				{
					Print("Unknown actor class \'%s\'.\n", typeName.Buffer());
				}
			}
		}
		else
		{
			inputDispatcher->DispatchAction(parser.ReadWord());
		}
	}

	int Engine::HandleWindowsMessage(HWND hwnd, UINT message, WPARAM & wparam, LPARAM & lparam)
	{
		if (uiSystemInterface)
			return uiSystemInterface->HandleSystemMessage(hwnd, message, wparam, lparam);
		return -1;
	}

	Texture2D * Engine::GetRenderResult(bool withUI)
	{
		if (withUI)
			return uiSystemInterface->GetRenderedImage();
		else
			return renderer->GetRenderedImage();
	}

	Actor * Engine::CreateActor(const CoreLib::String & name)
	{
		Func<Actor*> createFunc;
		if (actorClassRegistry.TryGetValue(name, createFunc))
			return createFunc();
		return nullptr;
	}

	void Engine::RegisterActorClass(const String &name, const Func<Actor*>& actorCreator)
	{
		actorClassRegistry[name] = actorCreator;
	}

	void Engine::LoadLevel(const CoreLib::String & fileName)
	{
		if (level)
			renderer->DestroyContext();
		level = nullptr;
		try
		{
			auto actualFileName = FindFile(fileName, ResourceType::Level);
			level = new GameEngine::Level(actualFileName);
			renderer->InitializeLevel(level.Ptr());
		}
		catch (const Exception & e)
		{
			Print("error loading level '%S': %S\n", fileName.ToWString(), e.Message.ToWString());
		}
	}

	RefPtr<Actor> Engine::ParseActor(GameEngine::Level * pLevel, Text::TokenReader & parser)
	{
		RefPtr<Actor> actor = CreateActor(parser.NextToken().Content);
		bool isInvalid = false;
		if (actor)
			actor->Parse(pLevel, parser, isInvalid);
		if (!isInvalid)
			return actor;
		else
			return nullptr;
	}

	CoreLib::String Engine::FindFile(const CoreLib::String & fileName, ResourceType type)
	{
		auto localFile = Path::Combine(GetDirectory(false, type), fileName);
		if (File::Exists(localFile))
			return localFile;
		auto engineFile = Path::Combine(GetDirectory(true, type), fileName);
		if (File::Exists(engineFile))
			return engineFile;
		return CoreLib::String();
	}

	CoreLib::String Engine::GetDirectory(bool useEngineDir, ResourceType type)
	{
		String subDirName;
		switch (type)
		{
		case ResourceType::Level:
			subDirName = "Levels";
			break;
		case ResourceType::Mesh:
			subDirName = "Models";
			break;
		case ResourceType::Shader:
			subDirName = "Shaders";
			break;
		case ResourceType::Texture:
		case ResourceType::Material:
			subDirName = "Materials";
			break;
		case ResourceType::Settings:
			subDirName = "Settings";
			break;
		case ResourceType::ShaderCache:
			subDirName = "Cache/Shaders";
			break;
		}
		if (useEngineDir)
			return Path::Combine(engineDir, subDirName);
		else
			return Path::Combine(gameDir, subDirName);
	}

	void Engine::Init(const EngineInitArguments & args)
	{
		instance->InternalInit(args);
	}
	void Engine::Destroy()
	{
		delete instance;
		instance = nullptr;
	}
	void Engine::SaveImage(Texture2D * image, String fileName)
	{
		CoreLib::Imaging::ImageRef imgRef;
		image->GetSize(imgRef.Width, imgRef.Height);
		List<unsigned char> imageBuffer;
		imageBuffer.SetSize(imgRef.Width * imgRef.Height * 4);
		image->GetData(0, imageBuffer.Buffer(), imageBuffer.Count());
		List<Vec4> imageBufferf;
		imageBufferf.SetSize(imgRef.Width * imgRef.Height);
		for (int i = 0; i < imgRef.Width * imgRef.Height; i++)
		{
			imageBufferf[i].x = imageBuffer[i * 4] / 255.0f;
			imageBufferf[i].y = imageBuffer[i * 4 + 1] / 255.0f;
			imageBufferf[i].z = imageBuffer[i * 4 + 2] / 255.0f;
			imageBufferf[i].w = imageBuffer[i * 4 + 3] / 255.0f;
		}
		imgRef.Pixels = imageBufferf.Buffer();
		auto lfileName = fileName.ToLower();
		if (lfileName.EndsWith("bmp"))
			imgRef.SaveAsBmpFile(fileName, true);
		else if (lfileName.EndsWith("png"))
			imgRef.SaveAsPngFile(fileName, true);
		else
			throw InvalidOperationException("Cannot save image as the specified file format.");
	}
}


