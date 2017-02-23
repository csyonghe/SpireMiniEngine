#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "CoreLib/Basic.h"
#include "CoreLib/PerformanceCounter.h"
#include "Level.h"
#include "CoreLib/Tokenizer.h"
#include "InputDispatcher.h"
#include "CoreLib/LibUI/LibUI.h"
#include "UISystem_Windows.h"
#include "CoreLib/WinForm/Debug.h"
#include "GraphicsSettings.h"
#include "DrawCallStatForm.h"
#include "SystemWindow.h"
#include "OS.h"

namespace GameEngine
{
    struct AppLaunchParameters
    {
        bool EnableVideoCapture = false;
        bool DumpRenderStats = false;
        String RenderStatsDumpFileName;
        String Directory;
        float Length = 10.0f;
        int FramesPerSecond = 30;
        int RunForFrames = 0; // run for this many frames and then terminate
    };
	class EngineInitArguments
	{
	public:
		RenderAPI API;
        bool NoConsole = false;
		int Width = 400, Height = 400;
		int GpuId = 0;
		bool RecompileShaders = false;
		CoreLib::String GameDirectory, EngineDirectory, StartupLevelName;
        AppLaunchParameters LaunchParams;
	};
	enum class EngineThread
	{
		GameLogic, Rendering
	};
	enum class ResourceType
	{
		Mesh, Shader, Level, Texture, Material, Landscape, Settings, ShaderCache, ExtTools
	};
	enum class TimingMode
	{
		Natural, Fixed
	};

	class Engine
	{
	private:
		static Engine * instance;
        AppLaunchParameters params;
		TimingMode timingMode = TimingMode::Natural;
		float fixedFrameDuration = 1.0f / 30.0f;
		unsigned int frameCounter = 0;
		bool inDataTransfer = false;
		GraphicsSettings graphicsSettings;
		CoreLib::String levelToLoad;
		CoreLib::List<RefPtr<Fence>> syncFences;
	private:
		bool enableInput = true;
		CoreLib::Diagnostics::TimePoint startTime, lastGameLogicTime, lastRenderingTime;
		float gameLogicTimeDelta = 0.0f, renderingTimeDelta = 0.0f;
		CoreLib::String gameDir, engineDir;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::Func<Actor*>> actorClassRegistry;
		CoreLib::RefPtr<Level> level;
		CoreLib::RefPtr<Renderer> renderer;
		CoreLib::RefPtr<InputDispatcher> inputDispatcher;
		
        CoreLib::RefPtr<SystemWindow> mainWindow;

		CoreLib::Array<RenderStat, 16> renderStats;
		GraphicsUI::CommandForm * uiCommandForm = nullptr;
		DrawCallStatForm * drawCallStatForm = nullptr;
		CoreLib::RefPtr<GraphicsUI::UIWindowsSystemInterface> uiSystemInterface;
        void MainLoop(CoreLib::Object *, CoreLib::WinForm::EventArgs);
		bool OnToggleConsoleAction(const CoreLib::String & actionName, ActionInput input);
		Engine() {};
		void InternalInit(const EngineInitArguments & args);
		~Engine();
	public:
		void RefreshUI();
		GraphicsSettings & GetGraphicsSettings()
		{
			return graphicsSettings;
		}
		void SaveGraphicsSettings();
		void SetTimingMode(TimingMode mode)
		{
			timingMode = mode;
		}
		// set fixed frame duration when TimingMode is Fixed
		void SetFrameDuration(float duration)
		{
			fixedFrameDuration = duration;
		}
		float GetTimeDelta(EngineThread thread);
		float GetTime()
		{
			if (timingMode == TimingMode::Natural)
				return CoreLib::Diagnostics::PerformanceCounter::EndSeconds(startTime);
			else
				return frameCounter * fixedFrameDuration;
		}
		Level * GetLevel()
		{
			return level.Ptr();
		}
		Renderer * GetRenderer()
		{
			return renderer.Ptr();
		}
		InputDispatcher * GetInputDispatcher()
		{
			return inputDispatcher.Ptr();
		}
        SystemWindow * GetMainWindow()
        {
            return mainWindow.Ptr();
        }
		GraphicsUI::UIEntry * GetUiEntry()
		{
			return mainWindow->GetUIEntry();
		}
		Texture2D * GetRenderResult(bool withUI);
		CoreLib::ArrayView<RenderStat> GetRenderStats()
		{
			return renderStats.GetArrayView();
		}
	public:
		Actor * CreateActor(const CoreLib::String & name);
		void RegisterActorClass(const CoreLib::String &name, const CoreLib::Func<Actor*> & actorCreator);
		void LoadLevel(const CoreLib::String & fileName);
		CoreLib::RefPtr<Actor> ParseActor(GameEngine::Level * level, CoreLib::Text::TokenReader & parser);
	public:
		CoreLib::String FindFile(const CoreLib::String & fileName, ResourceType type);
		CoreLib::String GetDirectory(bool useEngineDir, ResourceType type);
	public:
		void Tick();
		void Resize(int w, int h);
		void EnableInput(bool value);
		void OnCommand(CoreLib::String command);
        SystemWindow * CreateSystemWindow(int log2BufferSize = 21);
		int HandleWindowsMessage(HWND hwnd, UINT message, WPARAM &wparam, LPARAM &lparam);
	public:
		int GpuId = 0;
		bool RecompileShaders = false;
		static Engine * Instance()
		{
			if (!instance)
				instance = new Engine();
			return instance;
		}
		static void Init(const EngineInitArguments & args);
        static void Run();
		static void Destroy();
		template<typename ...Args>
		static void Print(const char * message, Args... args)
		{
			static char printBuffer[32768];
			static CoreLib::Diagnostics::TimePoint lastUIUpdate;
			snprintf(printBuffer, 32768, message, args...);
			if (instance && instance->uiCommandForm)
			{
				instance->uiCommandForm->Write(printBuffer);
				float timeElapsed = CoreLib::Diagnostics::PerformanceCounter::EndSeconds(lastUIUpdate);
				if (timeElapsed > 0.2f)
				{
					instance->RefreshUI();
					lastUIUpdate = CoreLib::Diagnostics::PerformanceCounter::Start();
				}
			}
			else
			{
				printf(message, args...);
			}
			CoreLib::Diagnostics::Debug::Write(printBuffer);
		}
		static void SaveImage(Texture2D * texture, String fileName);
	};

	template<typename ...Args>
	void Print(const char * message, Args... args)
	{
		Engine::Print(message, args...);
	}


}

#define EM(x) GraphicsUI::emToPixel(x)

#endif