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
		ManipulationMode manipulationMode = ManipulationMode::Translation;
		Matrix4 oldLocalTransform;
		TransformManipulator* manipulator;
		MenuItem* manipulationMenuItems[3];
		Level * level = nullptr;
		Vec2i mouseDownScreenSpacePos;
		Vec2i lastMouseScreenSpacePos;
		Actor * selectedActor = nullptr;
		RefPtr<CoreLib::WinForm::FileDialog> dlgOpen, dlgSave;
		void SelectActor(Actor * actor)
		{
			selectedActor = actor;
			if (actor)
				oldLocalTransform = actor->GetLocalTransform();
		}
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

			new MenuItem(mnEdit);

			manipulationMenuItems[0] = new MenuItem(mnEdit, "&Translate", "Ctrl+1");
			manipulationMenuItems[0]->OnClick.Bind(this, &LevelEditorImpl::mnManipulationTranslation_Clicked);
			manipulationMenuItems[1] = new MenuItem(mnEdit, "&Rotation", "Ctrl+2");
			manipulationMenuItems[1]->OnClick.Bind(this, &LevelEditorImpl::mnManipulationRotation_Clicked);
			manipulationMenuItems[2] = new MenuItem(mnEdit, "&Scale", "Ctrl+3");
			manipulationMenuItems[2]->OnClick.Bind(this, &LevelEditorImpl::mnManipulationScale_Clicked);

			manipulator = new TransformManipulator(entry);
			manipulator->OnPreviewManipulation.Bind(this, &LevelEditorImpl::PreviewManipulation);
			manipulator->OnApplyManipulation.Bind(this, &LevelEditorImpl::ApplyManipulation);

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
		Matrix4 GetManipulationTransform(ManipulationEventArgs e, Matrix4 existingTransform)
		{
			Matrix4 rs;
			Matrix4::CreateIdentityMatrix(rs);
			if (IsTranslationHandle(e.Handle))
			{
				Matrix4::Translation(rs, e.TranslationOffset.x, e.TranslationOffset.y, e.TranslationOffset.z);
				Matrix4::Multiply(rs, rs, existingTransform);
			}
			else if (IsRotationHandle(e.Handle))
			{
				switch (e.Handle)
				{
				case ManipulationHandleType::RotationX:
					Matrix4::RotationX(rs, e.RotationAngle);
					break;
				case ManipulationHandleType::RotationY:
					Matrix4::RotationY(rs, e.RotationAngle);
					break;
				case ManipulationHandleType::RotationZ:
					Matrix4::RotationZ(rs, e.RotationAngle);
					break;
				}
				Vec3 translation = Vec3::Create(existingTransform.values[12], existingTransform.values[13], existingTransform.values[14]);
				existingTransform.values[12] = 0.0f;
				existingTransform.values[13] = 0.0f;
				existingTransform.values[14] = 0.0f;
				Matrix4::Multiply(rs, rs, existingTransform);
				rs.values[12] = translation.x;
				rs.values[13] = translation.y;
				rs.values[14] = translation.z;
			}
			else
			{
				Matrix4::Scale(rs, e.Scale.x, e.Scale.y, e.Scale.z);
				Vec3 translation = Vec3::Create(existingTransform.values[12], existingTransform.values[13], existingTransform.values[14]);
				existingTransform.values[12] = 0.0f;
				existingTransform.values[13] = 0.0f;
				existingTransform.values[14] = 0.0f;
				Matrix4::Multiply(rs, rs, existingTransform);
				rs.values[12] = translation.x;
				rs.values[13] = translation.y;
				rs.values[14] = translation.z;
			}
			return rs;
		}
		void PreviewManipulation(UI_Base *, ManipulationEventArgs e)
		{
			if (!selectedActor)
				return;
			auto transform = GetManipulationTransform(e, oldLocalTransform);
			selectedActor->SetLocalTransform(transform);
		}
		void ApplyManipulation(UI_Base *, ManipulationEventArgs e)
		{
			if (selectedActor)
			{
				PreviewManipulation(nullptr, e);
				oldLocalTransform = selectedActor->GetLocalTransform();
			}
		}
	public:
		virtual void Tick() override
		{
			level = Engine::Instance()->GetLevel();
			if (!level)
				return;
			level->CurrentCamera = editorCam;

			if (selectedActor)
			{
				ManipulatorSceneView view;
				view.FOV = editorCam->FOV;
				auto viewport = Engine::Instance()->GetCurrentViewport();
				view.ViewportX = (float)viewport.x;
				view.ViewportY = (float)viewport.y;
				view.ViewportH = (float)viewport.height;
				view.ViewportW = (float)viewport.width;
				manipulator->SetTarget(manipulationMode, view, editorCam->GetLocalTransform(), editorCam->Position,
					selectedActor->GetPosition());
			}
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
			level = Engine::Instance()->GetLevel();
			if (level)
			{
				Ray ray = Engine::Instance()->GetRayFromMousePosition(e.X, e.Y);
				auto traceRs = level->GetPhysicsScene().RayTraceFirst(ray);
				if (traceRs.Object)
				{
					SelectActor(traceRs.Object->ParentActor);
				}
			}
		}
		void SwitchManipulationMode(ManipulationMode mode)
		{
			for (int i = 0; i < 3; i++)
				manipulationMenuItems[i]->Checked = false;
			manipulationMode = mode;
			manipulationMenuItems[(int)mode]->Checked = true;
		}
		void mnManipulationTranslation_Clicked(UI_Base *)
		{
			SwitchManipulationMode(ManipulationMode::Translation);
		}
		void mnManipulationRotation_Clicked(UI_Base *)
		{
			SwitchManipulationMode(ManipulationMode::Rotation);
		}
		void mnManipulationScale_Clicked(UI_Base *)
		{
			SwitchManipulationMode(ManipulationMode::Scale);
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

		bool DispatchShortcuts(UIKeyEventArgs & e)
		{
			if (e.Shift == SS_CONTROL)
			{
				if (CheckKey(e.Key, '1'))
					mnManipulationTranslation_Clicked(nullptr);
				else if (CheckKey(e.Key, '2'))
					mnManipulationRotation_Clicked(nullptr);
				else if (CheckKey(e.Key, '3'))
					mnManipulationScale_Clicked(nullptr);
				else if (CheckKey(e.Key, 'N'))
					mnNew_Clicked(nullptr);
				else if (CheckKey(e.Key, 'O'))
					mnOpen_Clicked(nullptr);
				else if (CheckKey(e.Key, 'S'))
					mnSave_Clicked(nullptr);
				else if (CheckKey(e.Key, 'Z'))
					mnUndo_Clicked(nullptr);
				else if (CheckKey(e.Key, 'Y'))
					mnRedo_Clicked(nullptr);
			}
			else if (e.Shift == (SS_CONTROL | SS_SHIFT))
			{
				if (CheckKey(e.Key, 'S'))
					mnSaveAs_Clicked(nullptr);
			}
			return false;
		}
	};
	LevelEditor * GameEngine::CreateLevelEditor()
	{
		return new LevelEditorImpl();
	}
}


