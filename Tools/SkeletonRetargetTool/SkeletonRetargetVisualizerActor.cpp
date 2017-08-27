#ifndef TEST_USER_ACTOR_H
#define TEST_USER_ACTOR_H

#include "MeshBuilder.h"
#include "CoreLib/WinForm/WinApp.h"
#include "CoreLib/WinForm/WinForm.h"
#include "CoreLib/WinForm/WinCommonDlg.h"
#include "Engine.h"
#include "Level.h"
#include "CoreLib/LibUI/LibUI.h"
#include "SystemWindow.h"
#include "EulerAngle.h"

using namespace GraphicsUI;
using namespace GameEngine;

class SkeletonRetargetVisualizerActor : public Actor
{
private:
	RefPtr<Drawable> sourceSkeletonDrawable, targetSkeletonDrawable;
	RefPtr<Drawable> xAxisDrawable, zAxisDrawable;
	RefPtr<SystemWindow> sysWindow;
	Mesh * mMesh = nullptr;
	Skeleton * mSkeleton = nullptr;
	Mesh sourceSkeletonMesh, targetSkeletonMesh;
	RefPtr<Mesh> sourceMesh;
	RefPtr<Drawable> sourceMeshDrawable;
	RefPtr<Skeleton> sourceSkeleton;
	RefPtr<Skeleton> targetSkeleton;
	Material sourceSkeletonMaterial, targetSkeletonMaterial, sourceMeshMaterial,
		xAxisMaterial, zAxisMaterial;
	GraphicsUI::ListBox* lstBones;
	GraphicsUI::TextBox *txtX, *txtY, *txtZ;
	GraphicsUI::TextBox *txtScaleX, *txtScaleY, *txtScaleZ;
	RetargetFile retargetFile;

public:

	virtual EngineActorType GetEngineType() override
	{
		return EngineActorType::Drawable;
	}

	void LoadSourceSkeleton(GraphicsUI::UI_Base *)
	{
		WinForm::FileDialog dlg(Engine::Instance()->GetMainWindow());
		dlg.Filter = "Skeleton|*.skeleton|All Files|*.*";
		if (dlg.ShowOpen())
		{
			sourceSkeleton = new Skeleton();
			sourceSkeleton->LoadFromFile(dlg.FileName);
			sourceSkeletonMesh.FromSkeleton(sourceSkeleton.Ptr(), 3.0f);
			sourceSkeletonDrawable = nullptr;
			retargetFile.SourceSkeletonName = sourceSkeleton->Name;
			sourceSkeletonMaterial.SetVariable("highlightId", -1);
			sourceMeshMaterial.SetVariable("highlightId", -1);
			lstBones->Clear();
			for (int i = 0; i < sourceSkeleton->Bones.Count(); i++)
				lstBones->AddTextItem(sourceSkeleton->Bones[i].Name);
		}
	}

	void LoadTargetSkeleton(GraphicsUI::UI_Base *)
	{
		WinForm::FileDialog dlg(Engine::Instance()->GetMainWindow());
		dlg.Filter = "Skeleton|*.skeleton|All Files|*.*";
		if (dlg.ShowOpen())
		{
			targetSkeleton = new Skeleton();
			targetSkeleton->LoadFromFile(dlg.FileName);
			targetSkeletonMesh.FromSkeleton(targetSkeleton.Ptr(), 3.0f);
			targetSkeletonDrawable = nullptr;
			targetSkeletonMaterial.SetVariable("highlightId", -1);
			retargetFile.RetargetTransforms.SetSize(targetSkeleton->Bones.Count());
			retargetFile.BoneMapping = targetSkeleton->BoneMapping;
			retargetFile.TargetSkeletonName = targetSkeleton->Name;
		}
	}

	void LoadSourceMesh(GraphicsUI::UI_Base *)
	{
		WinForm::FileDialog dlg(Engine::Instance()->GetMainWindow());
		dlg.Filter = "Mesh|*.mesh|All Files|*.*";
		if (dlg.ShowOpen())
		{
			sourceMesh = new Mesh();
			sourceMesh->LoadFromFile(dlg.FileName);
			sourceMeshDrawable = nullptr;
		}
	}

	void Save(GraphicsUI::UI_Base *)
	{
		WinForm::FileDialog dlg(Engine::Instance()->GetMainWindow());
		dlg.Filter = "Retarget File|*.retarget|All Files|*.*";
		dlg.DefaultEXT = "retarget";
		if (dlg.ShowSave())
		{
			retargetFile.SaveToFile(dlg.FileName);
		}
	}

	void Open(GraphicsUI::UI_Base *)
	{
		WinForm::FileDialog dlg(Engine::Instance()->GetMainWindow());
		dlg.Filter = "Retarget File|*.retarget|All Files|*.*";
		dlg.DefaultEXT = "retarget";
		if (dlg.ShowOpen())
		{
			retargetFile.LoadFromFile(dlg.FileName);
		}
	}
	
	bool disableTextChange = false;

	void ChangeScaleX(GraphicsUI::UI_Base * ctrl)
	{
		if (disableTextChange)
			return;
		if (lstBones->SelectedIndex != -1)
		{
			try
			{
				auto txt = dynamic_cast<GraphicsUI::TextBox*>(ctrl);
				CoreLib::Text::TokenReader p(txt->GetText());

				float val = p.ReadFloat();
				auto & info = retargetFile.RetargetTransforms[lstBones->SelectedIndex];
				info.ScaleX = val;
			}
			catch (...)
			{
			}
		}
	}

	void ChangeScaleY(GraphicsUI::UI_Base * ctrl)
	{
		if (disableTextChange)
			return;
		if (lstBones->SelectedIndex != -1)
		{
			try
			{
				auto txt = dynamic_cast<GraphicsUI::TextBox*>(ctrl);
				CoreLib::Text::TokenReader p(txt->GetText());

				float val = p.ReadFloat();
				auto & info = retargetFile.RetargetTransforms[lstBones->SelectedIndex];
				info.ScaleY = val;
			}
			catch (...)
			{
			}
		}
	}
	
	void ChangeScaleZ(GraphicsUI::UI_Base * ctrl)
	{
		if (disableTextChange)
			return;
		if (lstBones->SelectedIndex != -1)
		{
			try
			{
				auto txt = dynamic_cast<GraphicsUI::TextBox*>(ctrl);
				CoreLib::Text::TokenReader p(txt->GetText());

				float val = p.ReadFloat();
				auto & info = retargetFile.RetargetTransforms[lstBones->SelectedIndex];
				info.ScaleZ = val;
			}
			catch (...)
			{
			}
		}
	}

	void ChangeRotation(GraphicsUI::UI_Base * ctrl)
	{
		if (disableTextChange)
			return;
		if (lstBones->SelectedIndex != -1)
		{
			try
			{
				auto txt = dynamic_cast<GraphicsUI::TextBox*>(ctrl);
				CoreLib::Text::TokenReader p(txt->GetText());

				float angle = p.ReadFloat();
				if (angle > -360.0f && angle < 360.0f)
				{
					auto & info = retargetFile.RetargetTransforms[lstBones->SelectedIndex];
					EulerAngleToQuaternion(info.Rotation, StringToFloat(txtX->GetText()) / 180.0f * Math::Pi, StringToFloat(txtY->GetText()) / 180.0f * Math::Pi, StringToFloat(txtZ->GetText()) / 180.0f * Math::Pi, "YZX");
				}
			}
			catch (...)
			{
			}
		}
	}

	void SelectedBoneChanged(GraphicsUI::UI_Base *)
	{
		sourceSkeletonMaterial.SetVariable("highlightId", lstBones->SelectedIndex);
		sourceMeshMaterial.SetVariable("highlightId", lstBones->SelectedIndex);
		targetSkeletonMaterial.SetVariable("highlightId", lstBones->SelectedIndex);

		if (lstBones->SelectedIndex != -1 && lstBones->SelectedIndex < retargetFile.RetargetTransforms.Count())
		{
			float x, y, z;
			QuaternionToEulerAngle(retargetFile.RetargetTransforms[lstBones->SelectedIndex].Rotation, x, y, z, "YZX");
			disableTextChange = true;
			txtX->SetText(x * 180.0f / Math::Pi);
			txtY->SetText(y * 180.0f / Math::Pi);
			txtZ->SetText(z * 180.0f / Math::Pi);
			txtScaleX->SetText(String(retargetFile.RetargetTransforms[lstBones->SelectedIndex].ScaleX));
			txtScaleY->SetText(String(retargetFile.RetargetTransforms[lstBones->SelectedIndex].ScaleY));
			txtScaleZ->SetText(String(retargetFile.RetargetTransforms[lstBones->SelectedIndex].ScaleZ));

			disableTextChange = false;
		}
	}

	virtual void RegisterUI(GraphicsUI::UIEntry * uiEntry) override
	{
		auto menu = new GraphicsUI::Menu(uiEntry, GraphicsUI::Menu::msMainMenu);
		menu->BackColor = GraphicsUI::Color(0, 0, 0, 200);
		uiEntry->MainMenu = menu;
		auto mnFile = new GraphicsUI::MenuItem(menu, "File");
		auto mnEdit = new GraphicsUI::MenuItem(menu, "Edit");

		auto mnLoadSkeleton = new GraphicsUI::MenuItem(mnFile, "Load Source Skeleton...");
		mnLoadSkeleton->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::LoadSourceSkeleton);

		auto mnLoadSourceMesh = new GraphicsUI::MenuItem(mnFile, "Load Source Mesh...");
		mnLoadSourceMesh->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::LoadSourceMesh);


		auto mnLoadTargetSkeleton = new GraphicsUI::MenuItem(mnFile, "Load Target Skeleton...");
		mnLoadTargetSkeleton->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::LoadTargetSkeleton);

		new GraphicsUI::MenuItem(mnFile);
		auto mnSaveSkeleton = new GraphicsUI::MenuItem(mnFile, "Save Retarget File...");
		mnSaveSkeleton->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::Save);

		auto mnOpenSkeleton = new GraphicsUI::MenuItem(mnFile, "Open Retarget File...");
		mnOpenSkeleton->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::Open);


		auto pnl = new GraphicsUI::Container(uiEntry);
		pnl->BackColor = GraphicsUI::Color(0, 0, 0, 200);
		pnl->DockStyle = GraphicsUI::Control::dsLeft;
		pnl->SetWidth(EM(12.0f));
		pnl->Padding = EM(0.2f);

		auto pnl2 = new GraphicsUI::Container(pnl);
		pnl2->SetHeight(EM(9.0f));
		pnl2->DockStyle = GraphicsUI::Control::dsTop;
		
		auto addLabel = [&](Container* parent, String label, float left, float top, float width = 0.f, float height = 0.f)
		{
			auto lbl = new GraphicsUI::Label(parent);
			lbl->Posit(EM(left), EM(top), 0, 0);
			lbl->SetText(label);
		};

		addLabel(pnl2, "Rotation", 0, 0, 4.0f, 1.0f);

		addLabel(pnl2, "X", 1.8f, 1.0f);
		txtX = new GraphicsUI::TextBox(pnl2);
		txtX->Posit(EM(0.5f), EM(2.0f), EM(2.8f), EM(1.0f));
		txtX->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeRotation);

		addLabel(pnl2, "Y", 5.8f, 1.0f);
		txtY = new GraphicsUI::TextBox(pnl2);
		txtY->Posit(EM(4.5f), EM(2.0f), EM(2.8f), EM(1.0f));
		txtY->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeRotation);

		addLabel(pnl2, "Z", 9.8f, 1.0f);
		txtZ = new GraphicsUI::TextBox(pnl2);
		txtZ->Posit(EM(8.5f), EM(2.0f), EM(2.8f), EM(1.0f));
		txtZ->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeRotation);
		
		addLabel(pnl2, "Scale", 0, 4.0f, 4.0f, 1.0f);

		addLabel(pnl2, "X", 1.8f, 5.0f);
		txtScaleX = new GraphicsUI::TextBox(pnl2);
		txtScaleX->Posit(EM(0.5f), EM(6.0f), EM(2.8f), EM(1.0f));
		txtScaleX->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeScaleX);

		addLabel(pnl2, "Y", 5.8f, 5.0f);
		txtScaleY = new GraphicsUI::TextBox(pnl2);
		txtScaleY->Posit(EM(4.5f), EM(6.0f), EM(2.8f), EM(1.0f));
		txtScaleY->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeScaleY);

		addLabel(pnl2, "Z", 9.8f, 5.0f);
		txtScaleZ = new GraphicsUI::TextBox(pnl2);
		txtScaleZ->Posit(EM(8.5f), EM(6.0f), EM(2.8f), EM(1.0f));
		txtScaleZ->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeScaleZ);

		auto lblBones = new GraphicsUI::Label(pnl2);
		lblBones->SetText("Bones");
		lblBones->DockStyle = GraphicsUI::Control::dsBottom;
		
		lstBones = new GraphicsUI::ListBox(pnl);
		lstBones->DockStyle = GraphicsUI::Control::dsFill;
		lstBones->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::SelectedBoneChanged);
	}
	
	virtual void OnLoad() override
	{
		Actor::OnLoad();
		auto actualName = Engine::Instance()->FindFile("SkeletonVisualize.material", ResourceType::Material);

		sourceSkeletonMaterial.LoadFromFile(actualName);
		sourceMeshMaterial.LoadFromFile(actualName);
		targetSkeletonMaterial.LoadFromFile(actualName);

		targetSkeletonMaterial.SetVariable("solidColor", Vec3::Create(0.2f, 0.3f, 0.9f));
		targetSkeletonMaterial.SetVariable("alpha", 0.8f);
		targetSkeletonMaterial.SetVariable("highlightColor", Vec3::Create(0.7f, 0.7f, 0.1f));

		sourceMeshMaterial.SetVariable("alpha", 1.0f);
		sourceMeshMaterial.SetVariable("solidColor", Vec3::Create(0.2f, 0.2f, 0.2f));
		sourceMeshMaterial.SetVariable("highlightColor", Vec3::Create(1.0f, 0.2f, 0.2f));

		auto solidMaterialFileName = Engine::Instance()->FindFile("SolidColor.material", ResourceType::Material);
		xAxisMaterial.LoadFromFile(solidMaterialFileName);
		xAxisMaterial.SetVariable("solidColor", Vec3::Create(1.0f, 0.0f, 0.0f));

		zAxisMaterial.LoadFromFile(solidMaterialFileName);
		zAxisMaterial.SetVariable("solidColor", Vec3::Create(0.0f, 1.0f, 0.0f));

	}
	
	virtual void OnUnload() override
	{
		Actor::OnUnload();
	}
	
	virtual void GetDrawables(const GetDrawablesParameter & param) override
	{
		Matrix4 identity;
		Matrix4::CreateIdentityMatrix(identity);
		if (!xAxisDrawable)
		{
			MeshBuilder mb;
			mb.AddLine(Vec3::Create(0.0f), Vec3::Create(100.0f, 0.0f, 0.0f), Vec3::Create(0.0f, 1.0f, 0.0f), 3.0f);
			auto mesh = mb.ToMesh();
			xAxisDrawable = param.rendererService->CreateStaticDrawable(&mesh, &xAxisMaterial);
			xAxisDrawable->UpdateTransformUniform(identity);
		}
		param.sink->AddDrawable(xAxisDrawable.Ptr());
		if (!zAxisDrawable)
		{
			MeshBuilder mb;
			mb.AddLine(Vec3::Create(0.0f, 0.0f, 1.5f), Vec3::Create(0.0f, 0.0f, 100.0f), Vec3::Create(0.0f, 1.0f, 0.0f), 3.0f);
			auto mesh = mb.ToMesh();
			zAxisDrawable = param.rendererService->CreateStaticDrawable(&mesh, &zAxisMaterial);
			zAxisDrawable->UpdateTransformUniform(identity);
		}
		param.sink->AddDrawable(zAxisDrawable.Ptr());
		if (sourceSkeleton)
		{
			if (!sourceSkeletonDrawable)
			{
				sourceSkeletonDrawable = param.rendererService->CreateSkeletalDrawable(&sourceSkeletonMesh, sourceSkeleton.Ptr(), &sourceSkeletonMaterial, false);
			}
			
			Pose p;
			p.Transforms.SetSize(sourceSkeleton->Bones.Count());
			for (int i = 0; i < p.Transforms.Count(); i++)
				p.Transforms[i] = sourceSkeleton->Bones[i].BindPose;
			sourceSkeletonDrawable->UpdateTransformUniform(identity, p);
			param.sink->AddDrawable(sourceSkeletonDrawable.Ptr());
			
		}
		if (sourceSkeleton && targetSkeleton && sourceMesh)
		{

			Matrix4 translation;
			Matrix4::Translation(translation, 0.0f, 0.0f, 100.0f);
			if (!sourceMeshDrawable)
				sourceMeshDrawable = param.rendererService->CreateSkeletalDrawable(sourceMesh.Ptr(), sourceSkeleton.Ptr(), &sourceMeshMaterial, false);
			Pose p;
			p.Transforms.SetSize(targetSkeleton->Bones.Count());
			for (int i = 0; i < p.Transforms.Count(); i++)
				p.Transforms[i] = targetSkeleton->Bones[i].BindPose;
			sourceMeshDrawable->UpdateTransformUniform(translation, p, &retargetFile);
			param.sink->AddDrawable(sourceMeshDrawable.Ptr());
		}
		if (targetSkeleton)
		{
			if (!targetSkeletonDrawable)
			{
				targetSkeletonDrawable = param.rendererService->CreateSkeletalDrawable(&targetSkeletonMesh, targetSkeleton.Ptr(), &targetSkeletonMaterial, false);
			}
			Matrix4 identity;
			Matrix4::CreateIdentityMatrix(identity);
			Pose p;
			p.Transforms.SetSize(targetSkeleton->Bones.Count());
			for (int i = 0; i < p.Transforms.Count(); i++)
				p.Transforms[i] = targetSkeleton->Bones[i].BindPose;
			targetSkeletonDrawable->UpdateTransformUniform(identity, p, &retargetFile);
			param.sink->AddDrawable(targetSkeletonDrawable.Ptr());
		}
	}
};

void RegisterRetargetActor()
{
	Engine::Instance()->RegisterActorClass("SkeletonRetargetVisualizer", []() {return new SkeletonRetargetVisualizerActor(); });
}

#endif