#include "LevelEditor.h"
#include "CoreLib/WinForm/WinCommonDlg.h"
#include "Engine.h"
#include "CoreLib/LibUI/LibUI.h"
#include "Level.h"
#include "FreeRoamCameraController.h"
#include "PropertyEditControl.h"

using namespace CoreLib;
using namespace GraphicsUI;
using namespace VectorMath;

namespace GameEngine
{
	enum class MouseMode
	{
		None, ChangeView
	};
	class LevelEditorImpl : public LevelEditor
	{
	private:
		RefPtr<CameraActor> editorCam;
		RefPtr<FreeRoamCameraControllerActor> editorCamController;
		ManipulationMode manipulationMode = ManipulationMode::Translation;
		Matrix4 oldLocalTransform;
		TransformManipulator* manipulator;
		MenuItem* manipulationMenuItems[3];
		MouseMode mouseMode = MouseMode::None;
		Level * level = nullptr;
		Vec2i mouseDownScreenSpacePos;
		Vec2i lastMouseScreenSpacePos;
		Actor * selectedActor = nullptr;
		List<PropertyEdit*> propertyEdits;
		GraphicsUI::VScrollPanel * pnlProperties;
		GraphicsUI::ListBox * lstActors = nullptr;
		GraphicsUI::TextBox * txtName = nullptr;
		RefPtr<CoreLib::WinForm::FileDialog> dlgOpen, dlgSave;
		const Color uiGrayText = Color(180, 180, 180, 255);
		void UpdatePropertyPanel()
		{
			pnlProperties->ClearChildren();
			propertyEdits.Clear();
			if (!selectedActor)
				return;
			auto propertyList = selectedActor->GetPropertyList();
			int height = 0;
			auto lblType = new Label(pnlProperties);
			lblType->SetText(selectedActor->GetTypeName());
			lblType->Posit(0, 0, EM(10.0f), EM(1.0f));
			lblType->FontColor = uiGrayText;
			auto lblName = new Label(pnlProperties);
			lblName->SetText("Name");
			lblName->Posit(0, EM(1.1f), EM(10.0f), EM(1.0f));
			txtName = new TextBox(pnlProperties);
			txtName->SetText(selectedActor->Name.GetValue());
			txtName->Posit(0, EM(2.1f), EM(10.0f), EM(1.1f));
			txtName->OnKeyPress.Bind(this, &LevelEditorImpl::txtName_KeyPressed);
			height = txtName->Top + txtName->GetHeight() + EM(0.5f);
			for (auto & prop : propertyList)
			{
				if (strcmp(prop->GetName(), "Name") == 0)
					continue;
				auto edit = CreatePropertyEdit(pnlProperties, prop, Engine::Instance()->GetMainWindow());
				propertyEdits.Add(edit);
				edit->OnPropertyChanged.Bind(this, &LevelEditorImpl::propertyEdit_Changed);
				edit->Posit(0, height, pnlProperties->GetWidth() - EM(1.2f), edit->GetHeight());
				height += edit->GetHeight() + EM(0.2f);
			}
			if (selectedActor->GetEngineType() == EngineActorType::Camera)
			{
				auto btnCopyFromEditor = new Button(pnlProperties);
				btnCopyFromEditor->SetText("Use Current View");
				btnCopyFromEditor->Posit(0, height, EM(6.5f), EM(1.5f));
				btnCopyFromEditor->OnClick.Bind(this, &LevelEditorImpl::btnCopyCameraFromEditorCamera_Clicked);
				height += btnCopyFromEditor->GetHeight() + EM(0.2f);
				auto btnPreview = new Button(pnlProperties);
				btnPreview->SetText("Preview");
				btnPreview->Posit(0, height, EM(6.5f), EM(1.5f));
				btnPreview->OnClick.Bind(this, &LevelEditorImpl::btnPreviewCamera_Clicked);
			}
			pnlProperties->SizeChanged();
		}
		void UpdatePropertyValues()
		{
			for (auto & edit : propertyEdits)
				edit->Update();
		}
		void propertyEdit_Changed(Property *)
		{
			UpdatePropertyValues();
		}
		void btnCopyCameraFromEditorCamera_Clicked(UI_Base*)
		{
			if (selectedActor)
			{
				if (auto cam = dynamic_cast<CameraActor*>(selectedActor))
				{
					cam->Position = editorCam->Position.GetValue();
					cam->Orientation = editorCam->Orientation.GetValue();
				}
			}
		}
		void btnPreviewCamera_Clicked(UI_Base*)
		{
			if (selectedActor)
			{
				if (auto cam = dynamic_cast<CameraActor*>(selectedActor))
				{
					editorCam->Position = cam->Position.GetValue();
					editorCam->Orientation = cam->Orientation.GetValue();
				}
			}
		}
		void DeleteActor(Actor * actor)
		{
			if (!actor)
				return;
			if (actor == selectedActor)
				SelectActor(nullptr);
			Engine::Instance()->GetRenderer()->Wait();
			level->UnregisterActor(actor);
			UpdateActorList();
		}
		void SelectActor(Actor * actor)
		{
			if (selectedActor != actor)
			{
				selectedActor = actor;
				if (actor)
				{
					oldLocalTransform = actor->GetLocalTransform();
					int index = -1;
					for (int i = 0; i < lstActors->Items.Count(); i++)
						if (lstActors->GetTextItem(i)->GetText() == actor->Name.GetValue())
						{
							index = i;
							break;
						}
					lstActors->SetSelectedIndex(index);
				}
				else
					lstActors->SetSelectedIndex(-1);
				UpdatePropertyPanel();
			}
		}
		void CreateActor(String className)
		{
			if (level)
			{
				Engine::Instance()->GetRenderer()->Wait();
				auto actor = Engine::Instance()->CreateActor(className);
				actor->SetLevel(level);
				int i = 0;
				String name;
				do
				{
					name = className + String(i);
					i++;
				} while (level->FindActor(name));
				actor->Name = name;
				level->RegisterActor(actor);
				lstActors->AddTextItem(name);
				Matrix4 trans;
				auto pos = editorCam->GetPosition();
				auto dir = editorCam->GetView().GetDirection();
				auto offset = pos + dir * (editorCam->Radius.GetValue() + 100.0f);
				Matrix4::Translation(trans, offset.x, offset.y, offset.z);
				actor->LocalTransform = trans;
				SelectActor(actor);

			}
		}
		void InitUI()
		{
			Color uiBackColor = Color(45, 45, 45, 255);
			Global::Colors.EditableAreaBackColor = Color(35, 35, 35, 255);

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

			new MenuItem(mnEdit);
			auto mnDelete = new MenuItem(mnEdit, "&Delete", "Del");
			mnDelete->OnClick.Bind(this, &LevelEditorImpl::mnDelete_Clicked);

			manipulator = new TransformManipulator(entry);
			manipulator->OnPreviewManipulation.Bind(this, &LevelEditorImpl::PreviewManipulation);
			manipulator->OnApplyManipulation.Bind(this, &LevelEditorImpl::ApplyManipulation);

			auto pnlLeft = new Container(entry);
			pnlLeft->Padding = EM(0.5f);
			pnlLeft->Posit(0, 0, EM(15.0f), 100);
			pnlLeft->DockStyle = Control::dsLeft;
			pnlLeft->BackColor = uiBackColor;
			auto pnlCreate = new Container(pnlLeft);
			pnlCreate->Posit(0, 0, EM(10.0f), EM(18.0f));
			pnlCreate->DockStyle = Control::dsTop;
			auto lblCreate = new Label(pnlCreate);
			lblCreate->Posit(0, 0, 100, 25);
			lblCreate->SetText("Create");
			lblCreate->DockStyle = Control::dsTop;
			lblCreate->AutoHeight = false;
			lblCreate->SetHeight(EM(1.5f));
			auto pnlCreateActorButtons = new VScrollPanel(pnlCreate);
			pnlCreateActorButtons->DockStyle = Control::dsFill;
			auto actorClasses = Engine::Instance()->GetRegisteredActorClasses();
			int i = 0;
			for (auto & cls : actorClasses)
			{
				auto btnCreateActor = new Button(pnlCreateActorButtons);
				btnCreateActor->SetText(cls);
				btnCreateActor->Posit(0, i * EM(2.0f), EM(12.0f), EM(1.8f));
				btnCreateActor->OnClick.Bind(this, &LevelEditorImpl::btnCreateActor_Clicked);
				i++;
			}

			auto pnlActorList = new Container(pnlLeft);
			auto lblActors = new Label(pnlActorList);
			lblActors->Posit(0, 0, 100, 25);
			lblActors->SetText("Actors");
			lblActors->DockStyle = Control::dsTop;
			lblActors->AutoHeight = false;
			lblActors->SetHeight(EM(1.5f));
			pnlActorList->DockStyle = Control::dsFill;
			lstActors = new ListBox(pnlActorList);
			lstActors->DockStyle = Control::dsFill;
			lstActors->OnClick.Bind(this, &LevelEditorImpl::lstActors_Clicked);

			pnlProperties = new VScrollPanel(entry);
			pnlProperties->DockStyle = Control::dsRight;
			pnlProperties->SetWidth(EM(12.0f));
			pnlProperties->BackColor = uiBackColor;
			pnlProperties->Padding = EM(0.5f);
			LevelChanged();
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
				UpdatePropertyValues();
			}
		}
		void btnCreateActor_Clicked(UI_Base * ctrl)
		{
			Button* btn = reinterpret_cast<Button*>(ctrl);
			CreateActor(btn->GetText());
		}
		void mnDelete_Clicked(UI_Base *)
		{
			DeleteActor(selectedActor);
		}
		void RenameActor(Actor * actor, String newName)
		{
			if (actor)
			{
				auto oldName = actor->Name.GetValue();
				if (newName != actor->Name.GetValue())
				{
					if (level->FindActor(newName) == nullptr)
					{
						actor->Name = newName;
						List<RefPtr<Actor>> actors;
						for (auto & kv : level->Actors)
							actors.Add(kv.Value);
						level->Actors = EnumerableDictionary<String, RefPtr<Actor>>();
						for (auto & obj : actors)
							level->Actors.Add(obj->Name.GetValue(), obj);
					}
				}
				auto lstItem = lstActors->GetTextItem(lstActors->SelectedIndex);
				if (!lstItem || lstItem->GetText() != oldName)
				{
					lstItem = nullptr;
					for (int i = 0; i < lstActors->Items.Count(); i++)
					{
						if (lstActors->GetTextItem(i)->GetText() == oldName)
						{
							lstItem = lstActors->GetTextItem(i);
							break;
						}
					}
				}
				if (lstItem)
					lstItem->SetText(newName);
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
				manipulator->Visible = true;
				manipulator->SetTarget(manipulationMode, view, editorCam->GetCameraTransform(), editorCam->Position,
					selectedActor->GetPosition());
			}
			else
				manipulator->Visible = false;
		}
		virtual void OnLoad() override
		{
			InitUI();
			InitEditorActors();
		}
		void SelectActor(String name)
		{
			if (level)
				SelectActor(level->FindActor(name));
		}
		void txtName_KeyPressed(UI_Base*, UIKeyEventArgs & e)
		{
			if (e.Key == VK_RETURN)
			{
				RenameActor(selectedActor, txtName->GetText());
				if (selectedActor)
					txtName->SetText(selectedActor->Name.GetValue());
			}
		}
		void lstActors_Clicked(UI_Base *)
		{
			if (lstActors->SelectedIndex != -1)
			{
				auto name = lstActors->GetTextItem(lstActors->SelectedIndex)->GetText();
				if (!selectedActor || name != selectedActor->Name.GetValue())
				{
					SelectActor(name);
				}
			}
		}
		void mnNew_Clicked(UI_Base *)
		{
			SelectActor(nullptr);
			level = Engine::Instance()->NewLevel();
			LevelChanged();
		}
		void mnOpen_Clicked(UI_Base *)
		{
			if (dlgOpen->ShowOpen())
			{
				SelectActor(nullptr);
				Engine::Instance()->LoadLevel(dlgOpen->FileName);
				LevelChanged();
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
		void UpdateActorList()
		{
			lstActors->Clear();
			if (level)
			{
				for (auto & actor : level->Actors)
					lstActors->AddTextItem(actor.Key);
			}
		}
		void LevelChanged()
		{
			level = Engine::Instance()->GetLevel();
			SelectActor(nullptr);
			UpdateActorList();
		}
		void mnUndo_Clicked(UI_Base *)
		{
		}
		void mnRedo_Clicked(UI_Base *)
		{
		}
		void UIEntry_MouseDown(UI_Base *, UIMouseEventArgs & e)
		{
			lastMouseScreenSpacePos = mouseDownScreenSpacePos = Vec2i::Create(e.X, e.Y);
			level = Engine::Instance()->GetLevel();
			if (e.Shift == SS_BUTTONLEFT || e.Shift == SS_BUTTONRIGHT)
			{
				if (level)
				{
					Ray ray = Engine::Instance()->GetRayFromMousePosition(e.X, e.Y);
					auto traceRs = level->GetPhysicsScene().RayTraceFirst(ray);
					if (traceRs.Object)
						SelectActor(traceRs.Object->ParentActor);
					else
						SelectActor(nullptr);
				}
			}
			else if (e.Shift == SS_BUTTONMIDDLE)
			{
				mouseMode = MouseMode::ChangeView;
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
			auto curMouseScreenSpacePos = Vec2i::Create(e.X, e.Y);
			if (mouseMode == MouseMode::ChangeView)
			{
				float dx = (float)(curMouseScreenSpacePos.x - lastMouseScreenSpacePos.x);
				float dy = (float)(curMouseScreenSpacePos.y - lastMouseScreenSpacePos.y);
				editorCam->SetPitch(editorCam->GetPitch() + dy * 0.005f);
				editorCam->SetYaw(editorCam->GetYaw() + dx * 0.005f);
			}
			lastMouseScreenSpacePos = curMouseScreenSpacePos;
		}
		void UIEntry_MouseUp(UI_Base *, UIMouseEventArgs & e)
		{
			mouseMode = MouseMode::None;
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
				return true;
			}
			else if (e.Shift == (SS_CONTROL | SS_SHIFT))
			{
				if (CheckKey(e.Key, 'S'))
					mnSaveAs_Clicked(nullptr);
				return true;
			}
			else if (e.Key == VK_DELETE)
			{
				DeleteActor(selectedActor);
				return true;
			}
			return false;
		}
	};
	LevelEditor * GameEngine::CreateLevelEditor()
	{
		return new LevelEditorImpl();
	}
}


