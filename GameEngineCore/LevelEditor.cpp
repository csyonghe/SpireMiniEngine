#include "LevelEditor.h"
#include "CoreLib/WinForm/WinCommonDlg.h"
#include "Engine.h"
#include "CoreLib/LibUI/LibUI.h"
#include "Level.h"
#include "FreeRoamCameraController.h"

using namespace CoreLib;
using namespace GraphicsUI;
using namespace VectorMath;

namespace GameEngine
{
	class LevelEditorImpl : public LevelEditor
	{
	private:
		RefPtr<CameraActor> editorCam;
		RefPtr<FreeRoamCameraControllerActor> editorCamController;
		Level * level;
		Vec2i mouseDownScreenSpacePos;
		Vec2i lastMouseScreenSpacePos;
		RefPtr<CoreLib::WinForm::FileDialog> dlgOpen, dlgSave;
		void InitUI()
		{
			dlgOpen = new CoreLib::WinForm::FileDialog(Engine::Instance()->GetMainWindow());
			dlgOpen->FileMustExist = true;
			dlgOpen->Filter = "Level file|*.level";
			dlgOpen->DefaultEXT = "level";
			dlgOpen->FileName = Engine::Instance()->GetDirectory(false, ResourceType::Level) + "\\";
			dlgSave = new CoreLib::WinForm::FileDialog(Engine::Instance()->GetMainWindow());
			dlgSave->PathMustExist = true;
			dlgSave->OverwritePrompt = true;
			dlgSave->Filter = "Level file|*.level";
			dlgSave->DefaultEXT = "level";
			dlgSave->FileName = dlgOpen->FileName;

			auto entry = Engine::Instance()->GetMainWindow()->GetUIEntry();
			entry->OnKeyDown.Bind(this, &LevelEditorImpl::UIEntry_KeyDown);
			entry->OnMouseDown.Bind(this, &LevelEditorImpl::UIEntry_MouseDown);
			entry->OnMouseMove.Bind(this, &LevelEditorImpl::UIEntry_MouseMove);
			entry->OnMouseUp.Bind(this, &LevelEditorImpl::UIEntry_MouseUp);

			auto mainMenu = new Menu(entry, Menu::msMainMenu);
			mainMenu->BackColor = Color(50, 50, 50, 255);

			auto mnFile = new MenuItem(mainMenu, "&File");
			auto mnNew = new MenuItem(mnFile, "&New...", "Ctrl+N");
			mnNew->OnClick.Bind(this, &LevelEditorImpl::mnNew_Clicked);
			auto mnOpen = new MenuItem(mnFile, "&Open...", "Ctrl+O");
			mnOpen->OnClick.Bind(this, &LevelEditorImpl::mnOpen_Clicked);

			auto mnSave = new MenuItem(mnFile, "&Save", "Ctrl+S");
			mnSave->OnClick.Bind(this, &LevelEditorImpl::mnSave_Clicked);

			auto mnSaveAs = new MenuItem(mnFile, "S&ave as...", "Ctrl+Shift+S");
			mnSaveAs->OnClick.Bind(this, &LevelEditorImpl::mnSaveAs_Clicked);

			auto mnEdit = new MenuItem(mainMenu, "&Edit");
			auto mnUndo = new MenuItem(mnEdit, "&Undo", "Ctrl+Z");
			mnUndo->OnClick.Bind(this, &LevelEditorImpl::mnUndo_Clicked);

			auto mnRedo = new MenuItem(mnEdit, "&Redo", "Ctrl+Y");
			mnRedo->OnClick.Bind(this, &LevelEditorImpl::mnRedo_Clicked);

		}

		void InitEditorActors()
		{
			editorCam = new CameraActor();
			editorCamController = new FreeRoamCameraControllerActor();
			editorCamController->SetTargetCamera(editorCam.Ptr());
			editorCamController->InputChannel = EditorChannelId;
			editorCam->OnLoad();
			editorCamController->OnLoad();
		}
	public:
		virtual void Tick() override
		{
			level = Engine::Instance()->GetLevel();
			if (!level)
				return;
			level->CurrentCamera = editorCam;

		}
		virtual void OnLoad() override
		{
			InitUI();
			InitEditorActors();
		}

		void mnNew_Clicked(UI_Base *)
		{
			level = Engine::Instance()->NewLevel();
		}
		void mnOpen_Clicked(UI_Base *)
		{
			if (dlgOpen->ShowOpen())
			{
				Engine::Instance()->LoadLevel(dlgOpen->FileName);
			}
		}
		void mnSave_Clicked(UI_Base *)
		{
			if (level->FileName.Length() == 0)
			{
				mnSaveAs_Clicked(nullptr);
			}
			else
				level->SaveToFile(level->FileName);
		}
		void mnSaveAs_Clicked(UI_Base *)
		{
			if (level && dlgSave->ShowSave())
			{
				level->SaveToFile(dlgSave->FileName);
			}
		}
		void mnUndo_Clicked(UI_Base *)
		{
		}
		void mnRedo_Clicked(UI_Base *)
		{
		}
		void UIEntry_MouseDown(UI_Base *, UIMouseEventArgs & e)
		{
			mouseDownScreenSpacePos = Vec2i::Create(e.X, e.Y);
		}
		void UIEntry_MouseMove(UI_Base *, UIMouseEventArgs & e)
		{

			lastMouseScreenSpacePos = Vec2i::Create(e.X, e.Y);
		}
		void UIEntry_MouseUp(UI_Base *, UIMouseEventArgs & e)
		{
			lastMouseScreenSpacePos = Vec2i::Create(e.X, e.Y);
		}
		bool CheckKey(unsigned short key, char val)
		{
			if (key == val)
				return true;
			if (val >= 'A' && val <= 'Z' && key == val - 'A' + 'a')
				return true;
			return false;
		}
		void UIEntry_KeyDown(UI_Base *, UIKeyEventArgs & e)
		{
			if (DispatchShortcuts(e))
				return;
		}

		bool DispatchShortcuts(UIKeyEventArgs & /*e*/)
		{
			return false;
		}
	};
	LevelEditor * GameEngine::CreateLevelEditor()
	{
		return new LevelEditorImpl();
	}
}


