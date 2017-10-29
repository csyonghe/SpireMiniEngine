#include "Engine.h"
#include "SkeletalMeshActor.h"
#include "CameraActor.h"
#include "FreeRoamCameraController.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Tokenizer.h"
#include "EngineLimits.h"
#include "CoreLib/WinForm/WinApp.h"

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

    void Engine::MainLoop(CoreLib::Object *, CoreLib::WinForm::EventArgs)
    {
        static int frameId = 0;

        if (instance)
        {
            instance->Tick();
            if (params.EnableVideoCapture)
            {
                auto image = instance->GetRenderResult(true);
                if (videoEncodingStream)
                {
                    int w, h;
                    image->GetSize(w, h);
                    List<unsigned char> imageBuffer;
                    imageBuffer.SetSize(w * h * 4);
                    image->GetData(0, imageBuffer.Buffer(), imageBuffer.Count());
                    videoEncoder->EncodeFrame(w, h, imageBuffer.Buffer());
                }
                else
                    Engine::SaveImage(image, CoreLib::IO::Path::Combine(params.Directory, String(frameId) + ".bmp"));
                if (Engine::Instance()->GetTime() >= params.Length)
                {
                    mainWindow->Close();
                }
            }
            frameId++;
            if (frameId == params.RunForFrames)
            {
                if (params.DumpRenderStats)
                {
                    StringBuilder sb;
                    for (auto rs : renderStats)
                    {
                        if (rs.Divisor != 0)
                        {
                            sb << String(rs.CpuTime * 1000.0f / rs.Divisor, "%.1f") << "\t" << String(rs.TotalTime * 1000.0f / rs.Divisor, "%.1f")
                                << "\t" << rs.NumDrawCalls / rs.Divisor << "\n";
                        }
                    }
                    CoreLib::IO::File::WriteAllText(params.RenderStatsDumpFileName, sb.ProduceString());
                }
                mainWindow->Close();
            }
        }
    }

    bool Engine::OnToggleConsoleAction(const CoreLib::String & /*actionName*/, ActionInput /*val*/)
	{
		if (uiCommandForm)
			if (uiCommandForm->Visible)
				mainWindow->GetUIEntry()->CloseWindow(uiCommandForm);
			else
                mainWindow->GetUIEntry()->ShowWindow(uiCommandForm);
		return true;
	}

	void Engine::RefreshUI()
	{
		if (!inDataTransfer)
		{
            for (auto sysWindow : uiSystemInterface->windowContexts)
            {
                auto entry = sysWindow.Value->uiEntry.Ptr();
                auto uiCommands = entry->DrawUI();
				Texture2D * backgroundImage = nullptr;
				if (mainWindow == sysWindow.Key)
					backgroundImage = renderer->GetRenderedImage();
                uiSystemInterface->TransferDrawCommands(sysWindow.Value, backgroundImage, currentViewport, uiCommands);
            }
			renderer->GetHardwareRenderer()->TransferBarrier(DynamicBufferLengthMultiplier);
			for (auto sysWindow : uiSystemInterface->windowContexts)
			{
				uiSystemInterface->ExecuteDrawCommands(sysWindow.Value, nullptr);
				renderer->GetHardwareRenderer()->Present(sysWindow.Value->surface.Ptr(), sysWindow.Value->uiOverlayTexture.Ptr());
			}
			renderer->Wait();
		}
	}

	void Engine::InternalInit(const EngineInitArguments & args)
	{
		try
		{
            if (args.Editor)
                engineMode = EngineMode::Editor;
		
            RegisterEngineActorClasses(this);

            if (args.LaunchParams.Directory.ToLower().EndsWith("mp4"))
            {
                videoEncoder = CreateH264VideoEncoder();
                videoEncodingStream = new FileStream(args.LaunchParams.Directory, FileMode::Create);
                videoEncoder->Init(VideoEncodingOptions(args.Width, args.Height), videoEncodingStream.Ptr());
            }

			startTime = lastGameLogicTime = lastRenderingTime = Diagnostics::PerformanceCounter::Start();
			
			GpuId = args.GpuId;
			RecompileShaders = args.RecompileShaders;
            params = args.LaunchParams;

			gameDir = Path::Normalize(args.GameDirectory);
			engineDir = Path::Normalize(args.EngineDirectory);
			Path::CreateDir(Path::Combine(gameDir, "Cache"));
			Path::CreateDir(Path::Combine(gameDir, "Cache/Shaders"));
			Path::CreateDir(Path::Combine(gameDir, "Settings"));

			auto graphicsSettingsFile = FindFile("graphics.settings", ResourceType::Settings);
			if (graphicsSettingsFile.Length())
				graphicsSettings.LoadFromFile(graphicsSettingsFile);
            			
			// initialize renderer
			renderer = CreateRenderer(args.API);
			renderer->Resize(args.Width, args.Height);
			currentViewport.x = currentViewport.y = 0;
			currentViewport.width = args.Width;
			currentViewport.height = args.Height;
			syncFences.SetSize(DynamicBufferLengthMultiplier);
			fencePool.SetSize(DynamicBufferLengthMultiplier);
			uiSystemInterface = new UIWindowsSystemInterface(renderer->GetHardwareRenderer());
			Global::Colors = CreateDarkColorTable();
			
           
            // create main window
            mainWindow = new SystemWindow(uiSystemInterface.Ptr(), 22);
            mainWindow->SetText("Game Engine");
            mainWindow->OnResized.Bind([=](Object*, CoreLib::WinForm::EventArgs) 
            {
                Engine::Instance()->Resize(); 
            });
			mainWindow->SetClientWidth(args.Width);
			mainWindow->SetClientHeight(args.Height);
            mainWindow->CenterScreen();
            CoreLib::WinForm::Application::SetMainLoopEventHandler(new CoreLib::WinForm::NotifyEvent(this, &Engine::MainLoop));

            // initialize input dispatcher
            inputDispatcher = new InputDispatcher(CreateHardwareInputInterface(mainWindow->GetHandle()));
            auto bindingFile = Path::Combine(gameDir, "bindings.config");
            if (File::Exists(bindingFile))
                inputDispatcher->LoadMapping(bindingFile);
            inputDispatcher->BindActionHandler("ToggleConsole", ActionInputHandlerFunc(this, &Engine::OnToggleConsoleAction));

			uiCommandForm = new CommandForm(mainWindow->GetUIEntry());
			uiCommandForm->OnCommand.Bind(this, &Engine::OnCommand);

			drawCallStatForm = new DrawCallStatForm(mainWindow->GetUIEntry());
			drawCallStatForm->Posit(args.Width - drawCallStatForm->GetWidth() - 10, 10, drawCallStatForm->GetWidth(), drawCallStatForm->GetHeight());
			mainWindow->GetUIEntry()->CloseWindow(drawCallStatForm);

			if (args.NoConsole)
			{
                mainWindow->GetUIEntry()->CloseWindow(drawCallStatForm);
                mainWindow->GetUIEntry()->CloseWindow(uiCommandForm);
			}
			renderStats.SetSize(renderStats.GetCapacity());

			switch (args.API)
			{
			case RenderAPI::Vulkan:
				Print("Vulkan: %s\n", renderer->GetHardwareRenderer()->GetRendererName().Buffer());
				break;
			case RenderAPI::OpenGL:
				Print("OpenGL: %s\n", renderer->GetHardwareRenderer()->GetRendererName().Buffer());
				break;
			}

			auto configFile = Path::Combine(gameDir, "game.config");
			levelToLoad = RemoveQuote(args.StartupLevelName);
			if (!args.Editor && File::Exists(configFile))
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
			else
			{
				if (levelToLoad.Length())
				{
					LoadLevel(levelToLoad);
					levelToLoad = "";
				}
				else
					NewLevel();
				UseEditor(args.Editor.Ptr());
			}
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
        if (videoEncoder)
            videoEncoder->Close();
        if (videoEncodingStream)
            videoEncodingStream->Close();
		level = nullptr;
		fencePool = List<List<RefPtr<Fence>>>();
        mainWindow = nullptr;
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

	void Engine::Tick()
	{
		auto thisGameLogicTime = PerformanceCounter::Start();
		gameLogicTimeDelta = PerformanceCounter::EndSeconds(lastGameLogicTime);

		if (enableInput && mainWindow->Focused() && !mainWindow->GetUIEntry()->KeyInputConsumed && frameCounter > 2)
		{
			if (engineMode == EngineMode::Normal)
				inputDispatcher->DispatchInput(0);
			else
				inputDispatcher->DispatchInput(EditorChannelId);
		}
		if (!level)
		{
			if (levelToLoad.Length())
			{
				Print("loading %S\n", levelToLoad.ToWString());
				LoadLevel(levelToLoad);
				levelToLoad = "";
			}
		}
		if (level && engineMode == EngineMode::Normal)
		{
			level->GetPhysicsScene().Tick();
			for (auto & actor : level->Actors)
				actor.Value->Tick();
		}
		if (levelEditor)
		{
			levelEditor->Tick();
		}
		lastGameLogicTime = thisGameLogicTime;
		auto &stats = renderer->GetStats();
		auto thisRenderingTime = PerformanceCounter::Start();
		renderingTimeDelta = PerformanceCounter::EndSeconds(lastRenderingTime);
		lastRenderingTime = thisRenderingTime;

		if (stats.Divisor == 0)
			stats.StartTime = thisRenderingTime;

		for (auto & f : syncFences[frameCounter % DynamicBufferLengthMultiplier])
		{
			f->Wait();
			f->Reset();
		}
		renderer->GetHardwareRenderer()->ResetTempBufferVersion(frameCounter % DynamicBufferLengthMultiplier);

		auto cpuTimePoint = CoreLib::Diagnostics::PerformanceCounter::Start();

		inDataTransfer = true;
		
		renderer->TakeSnapshot();

        for (auto && sysWindow : uiSystemInterface->windowContexts)
        {
            if (!sysWindow.Key->GetVisible())
                continue;
            auto uiEntry = sysWindow.Value->uiEntry.Ptr();
            auto uiCommands = uiEntry->DrawUI();
            Texture2D * backgroundImage = nullptr;
            if (mainWindow == sysWindow.Key)
                backgroundImage = renderer->GetRenderedImage();
            uiSystemInterface->TransferDrawCommands(sysWindow.Value, backgroundImage, currentViewport, uiCommands);
        }

        inDataTransfer = false;
		renderer->GetHardwareRenderer()->TransferBarrier(frameCounter % DynamicBufferLengthMultiplier);
		renderer->RenderFrame();
		
		stats.CpuTime += CoreLib::Diagnostics::PerformanceCounter::EndSeconds(cpuTimePoint);

		int fenceAlloc = 0;
		int version = frameCounter % DynamicBufferLengthMultiplier;
		syncFences[version].Clear();
		for (auto && sysWindow : uiSystemInterface->windowContexts)
		{
			if (!sysWindow.Key->GetVisible())
				continue;
			if (fencePool[version].Count() == fenceAlloc)
				fencePool[version].Add(renderer->GetHardwareRenderer()->CreateFence());
			auto fence = fencePool[version][fenceAlloc].Ptr();
			fenceAlloc++;
			fence->Reset();
			uiSystemInterface->ExecuteDrawCommands(sysWindow.Value, fence);
			syncFences[version].Add(fence);
			aggregateTime += renderingTimeDelta;
			renderer->GetHardwareRenderer()->Present(sysWindow.Value->surface.Ptr(), sysWindow.Value->uiOverlayTexture.Ptr());
		}

		if (aggregateTime > 1.0f)
		{
			drawCallStatForm->SetNumShaders(stats.NumShaders);
			drawCallStatForm->SetNumMaterials(stats.NumMaterials);
		}

		if (stats.Divisor >= 500)
		{
			drawCallStatForm->SetFrameRenderTime(aggregateTime / stats.Divisor);
			drawCallStatForm->SetNumDrawCalls(stats.NumDrawCalls / stats.Divisor);
			drawCallStatForm->SetNumWorldPasses(stats.NumPasses / stats.Divisor);
			drawCallStatForm->SetCpuTime(stats.CpuTime / stats.Divisor, stats.PipelineLookupTime / stats.Divisor);
			static int ptr = 0;
			stats.TotalTime = CoreLib::Diagnostics::PerformanceCounter::EndSeconds(stats.StartTime);
			renderStats[ptr%renderStats.Count()] = stats;
			ptr++;
			stats.Clear();
			aggregateTime = 0.0f;
		}
		frameCounter++;
	}

	void Engine::Resize()
	{
		auto clientRect = mainWindow->GetUIEntry()->ClientRect();
		if (renderer && clientRect.w > 2 && clientRect.h > 2)
		{
			currentViewport.x = clientRect.x;
			currentViewport.y = clientRect.y;
			currentViewport.width = clientRect.w;
			currentViewport.height = clientRect.h;
			renderer->Resize(clientRect.w, clientRect.h);
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
		else if (parser.LookAhead("drawstat"))
		{
			parser.ReadToken();
			mainWindow->GetUIEntry()->ShowWindow(drawCallStatForm);
		}
		else if (parser.LookAhead("saveframe"))
		{
			try
			{
				parser.ReadToken();
				auto fileName = parser.ReadStringLiteral();
				SaveImage(renderer->GetRenderedImage(), fileName);
			}
			catch (const IOException &)
			{
				Print("IO failure.\n");
			}
			catch (Exception & e)
			{
				Print("Error: %s\n", e.Message.Buffer());
			}
		}
		else if (parser.LookAhead("savelevel"))
		{
			try
			{
				parser.ReadToken();
				auto fileName = parser.ReadStringLiteral();
				level->SaveToFile(fileName);
			}
			catch (const IOException &)
			{
				Print("IO failure.\n");
			}
			catch (Exception & e)
			{
				Print("Error: %s\n", e.Message.Buffer());
			}
		}
		else
		{
			try
			{
				auto word = parser.ReadToken();
				List<String> args;
				while (!parser.IsEnd())
					args.Add(parser.ReadToken().Content);
				inputDispatcher->DispatchAction(word.Content, args.GetArrayView(), 1.0f, engineMode==EngineMode::Normal?0:EditorChannelId);
			}
			catch (Exception)
			{
				Print("Invalid command.\n");
			}
		}
	}

	void Engine::UseEditor(LevelEditor * editor)
	{
		levelEditor = editor;
		engineMode = EngineMode::Editor;
		levelEditor->OnLoad();
	}

	void Engine::SetEngineMode(EngineMode newMode)
	{
		engineMode = newMode;
	}

	SystemWindow * Engine::CreateSystemWindow(int log2BufferSize)
    {
        auto rs = new SystemWindow(uiSystemInterface.Ptr(), log2BufferSize);
        rs->GetUIEntry()->BackColor = GraphicsUI::Color(50, 50, 50);
        return rs;
    }

	int Engine::HandleWindowsMessage(SystemWindow * window, UINT message, WPARAM & wparam, LPARAM & lparam)
	{
        if (uiSystemInterface)
        {
			auto ret = uiSystemInterface->HandleSystemMessage(window, message, wparam, lparam);
            return ret;
        }
        if (message == WM_PAINT)
            return 0;
		return -1;
	}

	Texture2D * Engine::GetRenderResult(bool withUI)
	{
        if (withUI)
            return mainWindow->GetUIContext()->uiOverlayTexture.Ptr();
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

	bool Engine::IsRegisteredActorClass(const CoreLib::String & name)
	{
		return actorClassRegistry.ContainsKey(name);
	}

	CoreLib::List<CoreLib::String> Engine::GetRegisteredActorClasses()
	{
		CoreLib::List<CoreLib::String> rs;
		for (auto & kv : this->actorClassRegistry)
			rs.Add(kv.Key);
		return rs;
	}

	void Engine::LoadLevel(const CoreLib::String & fileName)
	{
		renderer->Wait();
		level = nullptr;
		renderer->DestroyContext();
		try
		{
			auto actualFileName = FindFile(fileName, ResourceType::Level);
			level = new GameEngine::Level(actualFileName);
			inDataTransfer = true;
			renderer->InitializeLevel(level.Ptr());
			inDataTransfer = false;
		}
		catch (const Exception & e)
		{
			Print("error loading level '%S': %S\n", fileName.ToWString(), e.Message.ToWString());
		}
	}

	void Engine::LoadLevelFromText(const CoreLib::String & text)
	{
		level = nullptr;
		renderer->DestroyContext();
		level = new GameEngine::Level();
		level->LoadFromText(text);
		inDataTransfer = true;
		renderer->InitializeLevel(level.Ptr());
		inDataTransfer = false;
	}

	Level * Engine::NewLevel()
	{
		renderer->Wait();
		level = nullptr;
		renderer->DestroyContext();
		try
		{
			level = new GameEngine::Level();
			level->LoadFromText("Atmosphere{name \"atmosphere\"} Camera{name \"Camera0\"} FreeRoamCameraController{name \"cameraController\" TargetCameraName \"Camera0\"}");
			renderer->InitializeLevel(level.Ptr());
			inDataTransfer = false;
		}
		catch (const Exception &)
		{
			Print("error creating a new level.\n");
		}
		return level.Ptr();
	}

	void Engine::UpdateLightProbes()
	{
		renderer->UpdateLightProbes();
	}

    ObjPtr<Actor> Engine::ParseActor(GameEngine::Level * pLevel, Text::TokenReader & parser)
	{
        ObjPtr<Actor> actor = CreateActor(parser.NextToken().Content);
		bool isInvalid = false;
		if (actor)
			actor->Parse(pLevel, parser, isInvalid);
		if (!isInvalid)
			return actor;
		else
			return nullptr;
	}

	Ray Engine::GetRayFromMousePosition(int x, int y)
	{
		if (level && level->CurrentCamera)
		{
			int w = currentViewport.width;
			int h = currentViewport.height;
			float invH = 1.0f / h;
			return level->CurrentCamera->GetRayFromViewCoordinates((x - currentViewport.x) / (float)w , (y - currentViewport.y) * invH, w * invH);
		}
		Ray rs;
		rs.Origin.SetZero();
		rs.Dir = Vec3::Create(0.0f, 0.0f, -1.0f);
		return rs;
	}

	CoreLib::String Engine::FindFile(const CoreLib::String & fileName, ResourceType type)
	{
		if (!fileName.Length())
			return fileName;
		if (File::Exists(fileName))
			return fileName;
		auto localFile = Path::Combine(GetDirectory(false, type), fileName);
		if (File::Exists(localFile))
			return localFile;
		auto engineFile = Path::Combine(GetDirectory(true, type), fileName);
		if (File::Exists(engineFile))
			return engineFile;
		if (type == ResourceType::Shader)
		{
			return FindFile(fileName, ResourceType::Material);
		}
		else if (type == ResourceType::Texture || type == ResourceType::Material || type == ResourceType::Animation)
		{
			return FindFile(fileName, ResourceType::Mesh);
		}
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
		case ResourceType::Animation:
			subDirName = "Animations";
			break;
		case ResourceType::Landscape:
			subDirName = "Landscapes";
			break;
		case ResourceType::Settings:
			subDirName = "Settings";
			break;
		case ResourceType::ShaderCache:
			subDirName = "Cache/Shaders";
			break;
		case ResourceType::ExtTools:
			subDirName = "ExtTools";
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
    void Engine::Run()
    {
        CoreLib::WinForm::Application::Run(Engine::Instance()->mainWindow.Ptr(), true);
    }
	void Engine::Destroy()
	{
		delete instance;
		instance = nullptr;
		PropertyContainer::FreeRegistry();
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


