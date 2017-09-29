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
#include "AnimationSynthesizer.h"

using namespace GraphicsUI;
using namespace GameEngine;

class SkeletonRetargetVisualizerActor : public Actor
{
private:
	RefPtr<Drawable> targetSkeletonDrawable, sourceSkeletonDrawable;
	RefPtr<Drawable> xAxisDrawable, yAxisDrawable, zAxisDrawable;
	RefPtr<SystemWindow> sysWindow;
	Mesh * mMesh = nullptr;
	Skeleton * mSkeleton = nullptr;
	Mesh sourceSkeletonMesh, targetSkeletonMesh;
	RefPtr<Model> sourceModel;
	ModelDrawableInstance sourceModelInstance;
	RefPtr<Skeleton> targetSkeleton;
	Material sourceSkeletonMaterial, targetSkeletonMaterial, sourceMeshMaterial,
		xAxisMaterial, yAxisMaterial, zAxisMaterial;
	GraphicsUI::Label * lblFrame;
	GraphicsUI::Form * infoForm;
	GraphicsUI::MultiLineTextBox * infoFormTextBox;
	GraphicsUI::ListBox* lstBones;
	GraphicsUI::TextBox *txtX, *txtY, *txtZ;
	GraphicsUI::TextBox *txtScaleX, *txtScaleY, *txtScaleZ;
	GraphicsUI::MultiLineTextBox * txtInfo;
	List<GraphicsUI::ComboBox*> cmbAnimBones;
	GraphicsUI::VScrollPanel * pnlBoneMapping;
	GraphicsUI::ScrollBar * scTimeline;
	RetargetFile retargetFile;
	SkeletalAnimation currentAnim;
	Pose currentPose;
	GraphicsUI::UIEntry * uiEntry = nullptr;
	bool isPlaying = false;
	RefPtr<SimpleAnimationSynthesizer> animSynthesizer = new SimpleAnimationSynthesizer();
public:

	virtual EngineActorType GetEngineType() override
	{
		return EngineActorType::Drawable;
	}

	virtual void Tick() override
	{
		if (isPlaying)
		{
			if (currentAnim.Duration > 0.0f)
			{
				auto time = fmod(Engine::Instance()->GetTime(), currentAnim.Duration);
				int newFrame = (int)(time * 30.0f);
				while (newFrame >= scTimeline->GetMax())
					newFrame -= scTimeline->GetMax();
				if (newFrame < 1)
					newFrame = 1;
				if (newFrame < scTimeline->GetMax())
					scTimeline->SetPosition(newFrame);
			}
		}
	}

	void SetBoneMapping(int sourceBone, int animBone)
	{
		retargetFile.ModelBoneIdToAnimationBoneId[sourceBone] = animBone;
		UpdateCombinedRetargetTransform();
	}

	void UpdateCombinedRetargetTransform()
	{
		if (!targetSkeleton)
			return;
		List<BoneTransformation> animationBindPose;
		List<Matrix4> matrices, accumSourceBindPose, animationBindPoseM;
		auto sourceSkeleton = sourceModel->GetSkeleton();
		accumSourceBindPose.SetSize(sourceSkeleton->Bones.Count());
		animationBindPoseM.SetSize(sourceSkeleton->Bones.Count());
		for (int i = 0; i < accumSourceBindPose.Count(); i++)
		{
			auto trans = sourceSkeleton->Bones[i].BindPose;
			trans.Rotation = retargetFile.SourceRetargetTransforms[i] * trans.Rotation;
			accumSourceBindPose[i] = trans.ToMatrix();
		}
		for (int i = 1; i < accumSourceBindPose.Count(); i++)
			Matrix4::Multiply(accumSourceBindPose[i], accumSourceBindPose[sourceSkeleton->Bones[i].ParentId], accumSourceBindPose[i]);
		animationBindPose.SetSize(sourceSkeleton->Bones.Count());
		matrices.SetSize(sourceSkeleton->Bones.Count());
		// update translation terms in animationBindPose to match new skeleton
		List<Matrix4> accumAnimBindPose;
		accumAnimBindPose.SetSize(sourceSkeleton->Bones.Count());
		animationBindPose[0].Translation = sourceSkeleton->Bones[0].BindPose.Translation;
		accumAnimBindPose[0] = animationBindPose[0].ToMatrix();
		for (int i = 0; i < matrices.Count(); i++)
		{
			BoneTransformation transform;
			transform.Rotation = retargetFile.SourceRetargetTransforms[i] * sourceSkeleton->Bones[i].BindPose.Rotation;
			transform.Translation = sourceSkeleton->Bones[i].BindPose.Translation;
			
			matrices[i] = transform.ToMatrix();

			animationBindPose[i].Rotation = sourceSkeleton->Bones[i].BindPose.Rotation;
			int animBoneId = retargetFile.ModelBoneIdToAnimationBoneId[i];
			if (animBoneId != -1)
			{
				animationBindPose[i].Rotation = targetSkeleton->Bones[animBoneId].BindPose.Rotation;
			}
			if (i > 0)
			{
				Matrix4 invP;
				accumAnimBindPose[sourceSkeleton->Bones[i].ParentId].Inverse(invP);
				auto P = accumSourceBindPose[sourceSkeleton->Bones[i].ParentId];
				animationBindPose[i].Translation = invP.TransformHomogeneous(P.TransformHomogeneous(sourceSkeleton->Bones[i].BindPose.Translation));
				Matrix4::Multiply(accumAnimBindPose[i], accumAnimBindPose[sourceSkeleton->Bones[i].ParentId], animationBindPose[i].ToMatrix());
			}
			else
			{
				accumAnimBindPose[i] = animationBindPose[i].ToMatrix();
			}
		}
		for (int i = 0; i < animationBindPoseM.Count(); i++)
			animationBindPoseM[i] = animationBindPose[i].ToMatrix();
		for (int i = 1; i < matrices.Count(); i++)
		{
			Matrix4::Multiply(matrices[i], matrices[sourceSkeleton->Bones[i].ParentId], matrices[i]);
			Matrix4::Multiply(animationBindPoseM[i], animationBindPoseM[sourceSkeleton->Bones[i].ParentId], animationBindPoseM[i]);
		}
		for (int i = 0; i < matrices.Count(); i++)
			Matrix4::Multiply(matrices[i], matrices[i], sourceSkeleton->InversePose[i]);

		for (int i = 0; i < matrices.Count(); i++)
		{
			Matrix4 invAnimBindPose;
			animationBindPoseM[i].Inverse(invAnimBindPose);
			Matrix4::Multiply(retargetFile.RetargetedInversePose[i], invAnimBindPose, matrices[i]);
		}
		// derive the retargeted offset
		for (int i = 0; i < matrices.Count(); i++)
		{
			retargetFile.RetargetedBoneOffsets[i] = animationBindPose[i].Translation;
		}
	}

	void LoadTargetSkeleton(GraphicsUI::UI_Base *)
	{
		if (!sourceModel)
		{
			Engine::Instance()->GetMainWindow()->MessageBox("Please load source model first.", "Error", MB_ICONEXCLAMATION);
			return;
		}
		WinForm::FileDialog dlg(Engine::Instance()->GetMainWindow());
		dlg.Filter = "Skeleton|*.skeleton|All Files|*.*";
		if (dlg.ShowOpen())
		{
			targetSkeleton = new Skeleton();
			targetSkeleton->LoadFromFile(dlg.FileName);
			targetSkeletonMesh.FromSkeleton(targetSkeleton.Ptr(), 3.0f);
			targetSkeletonDrawable = nullptr;
			targetSkeletonMaterial.SetVariable("highlightId", -1);
			retargetFile.RetargetedInversePose = sourceModel->GetSkeleton()->InversePose;
			int i = 0;
			for (auto & bone : sourceModel->GetSkeleton()->Bones)
			{
				retargetFile.RetargetedBoneOffsets[i] = bone.BindPose.Translation;
				i++;
			}
			retargetFile.TargetSkeletonName = targetSkeleton->Name;
			for (auto & id : retargetFile.ModelBoneIdToAnimationBoneId)
				id = -1;
			int j = 0;
			i = 0;
			for (auto & b : sourceModel->GetSkeleton()->Bones)
			{
				j = 0;
				for (auto & ab : targetSkeleton->Bones)
				{
					if (b.Name == ab.Name)
						retargetFile.ModelBoneIdToAnimationBoneId[i] = j;
					j++;
				}
				i++;
			}
			UpdateBoneMappingPanel();
		}
	}

	void ViewBindPose(GraphicsUI::UI_Base *)
	{
		scTimeline->SetPosition(0);
	}

	void LoadAnimation(GraphicsUI::UI_Base *)
	{
		WinForm::FileDialog dlg(Engine::Instance()->GetMainWindow());
		dlg.Filter = "Animation|*.anim|All Files|*.*";
		if (dlg.ShowOpen())
		{
			currentAnim.LoadFromFile(dlg.FileName);
			scTimeline->SetValue(0, (int)(currentAnim.Duration * 30.0f), 0, 1);
		}
	}

	void LoadSourceModel(GraphicsUI::UI_Base *)
	{
		WinForm::FileDialog dlg(Engine::Instance()->GetMainWindow());
		dlg.Filter = "Model|*.model|All Files|*.*";
		if (dlg.ShowOpen())
		{
			sourceModel = new Model();
			sourceModel->LoadFromFile(level, dlg.FileName);
			sourceModelInstance.Drawables.Clear();
			sourceSkeletonMesh.FromSkeleton(sourceModel->GetSkeleton(), 3.0f);
			sourceSkeletonDrawable = nullptr;
			sourceSkeletonMaterial.SetVariable("highlightId", -1);
			sourceMeshMaterial.SetVariable("highlightId", -1);
			retargetFile.SourceSkeletonName = sourceModel->GetSkeleton()->Name;
			retargetFile.SetBoneCount(sourceModel->GetSkeleton()->Bones.Count()); 
			lstBones->Clear();
			auto sourceSkeleton = sourceModel->GetSkeleton();
			for (int i = 0; i < sourceSkeleton->Bones.Count(); i++)
				lstBones->AddTextItem(sourceSkeleton->Bones[i].Name);
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
		if (sourceModel &&  dlg.ShowOpen())
		{
			RetargetFile file;
			file.LoadFromFile(dlg.FileName);
			if (file.ModelBoneIdToAnimationBoneId.Count() != sourceModel->GetSkeleton()->Bones.Count())
				Engine::Instance()->GetMainWindow()->MessageBox("This retarget file does not match the current model.", "Error", MB_ICONEXCLAMATION);
			else
			{
				retargetFile = file;
				txtScaleX->SetText(String(file.RootTranslationScale.x));
				txtScaleY->SetText(String(file.RootTranslationScale.y));
				txtScaleZ->SetText(String(file.RootTranslationScale.z));
				UpdateMappingSelection();
				UpdateCombinedRetargetTransform();
			}
		}
	}
	
	bool disableTextChange = false;

	int GetAnimationBoneIndex(int modelBoneIndex)
	{
		return retargetFile.ModelBoneIdToAnimationBoneId[modelBoneIndex];
	}

	void ChangeScaleX(GraphicsUI::UI_Base * ctrl)
	{
		if (disableTextChange)
			return;
		try
		{
			auto txt = dynamic_cast<GraphicsUI::TextBox*>(ctrl);
			CoreLib::Text::TokenReader p(txt->GetText());

			float val = p.ReadFloat();
			retargetFile.RootTranslationScale.x = val;
		}
		catch (...)
		{
		}
	}

	void ChangeScaleY(GraphicsUI::UI_Base * ctrl)
	{
		if (disableTextChange)
			return;
		try
		{
			auto txt = dynamic_cast<GraphicsUI::TextBox*>(ctrl);
			CoreLib::Text::TokenReader p(txt->GetText());

			float val = p.ReadFloat();
			retargetFile.RootTranslationScale.y = val;

		}
		catch (...)
		{
		}
	}
	
	void ChangeScaleZ(GraphicsUI::UI_Base * ctrl)
	{
		if (disableTextChange)
			return;
		try
		{
			auto txt = dynamic_cast<GraphicsUI::TextBox*>(ctrl);
			CoreLib::Text::TokenReader p(txt->GetText());

			float val = p.ReadFloat();
			retargetFile.RootTranslationScale.z = val;

		}
		catch (...)
		{
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
					auto & info = retargetFile.SourceRetargetTransforms[lstBones->SelectedIndex];
					EulerAngleToQuaternion(info, StringToFloat(txtX->GetText()) / 180.0f * Math::Pi, StringToFloat(txtY->GetText()) / 180.0f * Math::Pi, StringToFloat(txtZ->GetText()) / 180.0f * Math::Pi, EulerAngleOrder::ZXY);
				}
				UpdateCombinedRetargetTransform();

			}
			catch (...)
			{
			}
		}
	}

	void UpdateMappingSelection()
	{
		if (cmbAnimBones.Count() == retargetFile.ModelBoneIdToAnimationBoneId.Count())
		{
			for (int i = 0; i < retargetFile.ModelBoneIdToAnimationBoneId.Count(); i++)
				cmbAnimBones[i]->SetSelectedIndex(Math::Clamp(retargetFile.ModelBoneIdToAnimationBoneId[i] + 1, -1, cmbAnimBones[i]->Items.Count() - 1));
		}
	}

	void SelectedBoneChanged(GraphicsUI::UI_Base *)
	{
		if (lstBones->SelectedIndex != -1)
		{
			sourceSkeletonMaterial.SetVariable("highlightId", lstBones->SelectedIndex);
			sourceMeshMaterial.SetVariable("highlightId", lstBones->SelectedIndex);
			targetSkeletonMaterial.SetVariable("highlightId", GetAnimationBoneIndex(lstBones->SelectedIndex));
			float x, y, z;
			QuaternionToEulerAngle(retargetFile.SourceRetargetTransforms[lstBones->SelectedIndex], x, y, z, EulerAngleOrder::ZXY);
			disableTextChange = true;
			txtX->SetText(String(x * 180.0f / Math::Pi, "%.1f"));
			txtY->SetText(String(y * 180.0f / Math::Pi, "%.1f"));
			txtZ->SetText(String(z * 180.0f / Math::Pi, "%.1f"));
		
			StringBuilder sbInfo;
			auto sourceSkeleton = sourceModel->GetSkeleton();
			sbInfo << "Parent: ";
			if (sourceSkeleton->Bones[lstBones->SelectedIndex].ParentId == -1)
				sbInfo << "null\n";
			else
				sbInfo << sourceSkeleton->Bones[sourceSkeleton->Bones[lstBones->SelectedIndex].ParentId].Name << "\n";

			QuaternionToEulerAngle(sourceSkeleton->Bones[lstBones->SelectedIndex].BindPose.Rotation, x, y, z, EulerAngleOrder::ZXY);
			sbInfo << "BindPose:\n  Rotation " << String(x* 180.0f / Math::Pi, "%.1f") << ", " << String(y* 180.0f / Math::Pi, "%.1f") << ", " << String(z* 180.0f / Math::Pi, "%.1f") << "\n";
			auto trans = sourceSkeleton->Bones[lstBones->SelectedIndex].BindPose.Translation;
			sbInfo << "  Offset " << String(trans.x, "%.1f") << ", " << String(trans.y, "%.1f") << ", " << String(trans.z, "%.1f") << "\n";
			int animBoneId = GetAnimationBoneIndex(lstBones->SelectedIndex);
			if (animBoneId != -1 && targetSkeleton)
			{
				sbInfo << "AnimPose: " << targetSkeleton->Bones[animBoneId].Name << "\n";
				QuaternionToEulerAngle(targetSkeleton->Bones[animBoneId].BindPose.Rotation, x, y, z, EulerAngleOrder::ZXY);
				sbInfo << "  Rotation " << String(x* 180.0f / Math::Pi, "%.1f") << ", " << String(y* 180.0f / Math::Pi, "%.1f") << ", " << String(z* 180.0f / Math::Pi, "%.1f") << "\n";
				auto trans = targetSkeleton->Bones[animBoneId].BindPose.Translation;
				sbInfo << "  Offset " << String(trans.x, "%.1f") << ", " << String(trans.y, "%.1f") << ", " << String(trans.z, "%.1f") << "\n";
			}
			txtInfo->SetText(sbInfo.ProduceString());
			disableTextChange = false;
		}
	}

	void cmbBoneMappingChanged(UI_Base * sender)
	{
		int idx = -1;
		for (int i = 0; i < cmbAnimBones.Count(); i++)
		{
			if (cmbAnimBones[i] == sender)
			{
				idx = i;
				break;
			}
		}
		if (idx != -1 && targetSkeleton)
		{
			SetBoneMapping(idx, Math::Clamp(cmbAnimBones[idx]->SelectedIndex - 1, -1, targetSkeleton->Bones.Count() - 1));
			targetSkeletonMaterial.SetVariable("highlightId", retargetFile.ModelBoneIdToAnimationBoneId[idx]);
		}
	}

	Pose GetAnimSkeletonBindPose()
	{
		Pose rs;
		if (targetSkeleton)
		{
			rs.Transforms.SetSize(targetSkeleton->Bones.Count());
			for (int i = 0; i < targetSkeleton->Bones.Count(); i++)
				rs.Transforms[i] = targetSkeleton->Bones[i].BindPose;
		}
		return rs;
	}

	Pose GetCurrentPose()
	{
		if (targetSkeleton)
		{
			if (scTimeline->GetPosition() == 0)
				return GetAnimSkeletonBindPose();
			else
			{
				animSynthesizer->SetSource(targetSkeleton.Ptr(), &currentAnim);
				float time = scTimeline->GetPosition() / 30.0f;
				Pose p;
				animSynthesizer->GetPose(p, time);
				return p;
			}
		}
		return Pose();
	}

	void GetSkeletonInfo(StringBuilder & sb, Skeleton * skeleton, int id, int indent)
	{
		for (int i = 0; i < indent; i++)
			sb << "  ";
		sb << "\"" << skeleton->Bones[id].Name << "\"\n"; 
		for (int i = 0; i < indent; i++)
			sb << "  ";
		sb << "{\n";
		for (int i = 0; i < skeleton->Bones.Count(); i++)
		{
			if (skeleton->Bones[i].ParentId == id)
				GetSkeletonInfo(sb, skeleton, i, indent + 1);
		}
		for (int i = 0; i < indent; i++)
			sb << "  ";
		sb << "}\n";
		
	}

	void mnPlayAnim_Clicked(UI_Base * sender)
	{
		isPlaying = !isPlaying;
	}

	void mnShowTargetSkeletonShape_Clicked(UI_Base * sender)
	{
		if (targetSkeleton)
		{
			StringBuilder sb;
			GetSkeletonInfo(sb, targetSkeleton.Ptr(), 0, 0);
			infoFormTextBox->SetText(sb.ProduceString());
			uiEntry->ShowWindow(infoForm);
		}
	}

	void mnShowSourceSkeletonShape_Clicked(UI_Base * sender)
	{
		if (sourceModel)
		{
			StringBuilder sb;
			GetSkeletonInfo(sb, sourceModel->GetSkeleton(), 0, 0);
			infoFormTextBox->SetText(sb.ProduceString());
			uiEntry->ShowWindow(infoForm);
		}
	}

	void scTimeline_Changed(UI_Base * sender)
	{
		if (scTimeline->GetPosition() == 0)
			lblFrame->SetText("BindPose");
		else
			lblFrame->SetText(String("Animation: ") + String(scTimeline->GetPosition() / 30.0f, "%.2f") + "s");
	}

	void UpdateBoneMappingPanel()
	{
		cmbAnimBones.Clear();
		pnlBoneMapping->ClearChildren();
		for (int i = 0; i < sourceModel->GetSkeleton()->Bones.Count(); i++)
		{
			auto name = sourceModel->GetSkeleton()->Bones[i].Name;
			auto lbl = new GraphicsUI::Label(pnlBoneMapping);
			lbl->SetText(name);
			lbl->Posit(EM(0.2f), EM(i * 2.5f), EM(12.5f), EM(1.0f));
			int selIdx = -1;
			auto cmb = new GraphicsUI::ComboBox(pnlBoneMapping);
			selIdx = retargetFile.ModelBoneIdToAnimationBoneId[i];
			cmb->AddTextItem("(None)");
			for (auto & b : targetSkeleton->Bones)
				cmb->AddTextItem(b.Name);
			cmb->SetSelectedIndex(selIdx + 1);
			cmbAnimBones.Add(cmb);
			cmb->Posit(EM(0.2f), EM(i * 2.5f + 1.1f), EM(12.5f), EM(1.2f));
			cmb->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::cmbBoneMappingChanged);
		}
		pnlBoneMapping->SizeChanged();
		UpdateCombinedRetargetTransform();
	}

	virtual void RegisterUI(GraphicsUI::UIEntry * pUiEntry) override
	{
		this->uiEntry = pUiEntry;
		auto menu = new GraphicsUI::Menu(uiEntry, GraphicsUI::Menu::msMainMenu);
		menu->BackColor = GraphicsUI::Color(0, 0, 0, 200);
		uiEntry->MainMenu = menu;
		auto mnFile = new GraphicsUI::MenuItem(menu, "File");
		auto mnView = new GraphicsUI::MenuItem(menu, "View");

		auto mnViewBindPose = new GraphicsUI::MenuItem(mnView, "Bind Pose");
		mnViewBindPose->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::ViewBindPose);

		auto mnLoadSourceMesh = new GraphicsUI::MenuItem(mnFile, "Load Model...");
		mnLoadSourceMesh->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::LoadSourceModel);


		auto mnLoadTargetSkeleton = new GraphicsUI::MenuItem(mnFile, "Load Animation Skeleton...");
		mnLoadTargetSkeleton->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::LoadTargetSkeleton);

		auto mnLoadAnim = new GraphicsUI::MenuItem(mnFile, "Load Animation...");
		mnLoadAnim->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::LoadAnimation);

		new GraphicsUI::MenuItem(mnFile);
		auto mnOpenSkeleton = new GraphicsUI::MenuItem(mnFile, "Open Retarget File...");
		mnOpenSkeleton->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::Open);
		
		auto mnSaveSkeleton = new GraphicsUI::MenuItem(mnFile, "Save Retarget File...");
		mnSaveSkeleton->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::Save);

		auto mnShowSourceSkeletonShape = new GraphicsUI::MenuItem(mnView, "Source Skeleton Shape");
		mnShowSourceSkeletonShape->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::mnShowSourceSkeletonShape_Clicked);
		auto mnShowTargetSkeletonShape = new GraphicsUI::MenuItem(mnView, "Target Skeleton Shape");
		mnShowTargetSkeletonShape->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::mnShowTargetSkeletonShape_Clicked);
		auto mnPlayAnimation = new GraphicsUI::MenuItem(mnView, "Play/Stop Animation");
		mnPlayAnimation->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::mnPlayAnim_Clicked);

		auto pnl = new GraphicsUI::Container(uiEntry);
		pnl->BackColor = GraphicsUI::Color(0, 0, 0, 200);
		pnl->DockStyle = GraphicsUI::Control::dsLeft;
		pnl->SetWidth(EM(12.0f));
		pnl->Padding = EM(0.2f);

		auto pnl2 = new GraphicsUI::Container(pnl);
		pnl2->SetHeight(EM(19.0f));
		pnl2->DockStyle = GraphicsUI::Control::dsTop;
		
		auto addLabel = [&](Container* parent, String label, float left, float top, float width = 0.f, float height = 0.f)
		{
			auto lbl = new GraphicsUI::Label(parent);
			lbl->Posit(EM(left), EM(top), 0, 0);
			lbl->SetText(label);
		};

		addLabel(pnl2, "Root Translation Scale", 0, 0.0f, 4.0f, 1.0f);

		addLabel(pnl2, "X", 1.8f, 1.0f);
		txtScaleX = new GraphicsUI::TextBox(pnl2);
		txtScaleX->Posit(EM(0.5f), EM(2.0f), EM(2.8f), EM(1.0f));
		txtScaleX->SetText("1.0");
		txtScaleX->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeScaleX);

		addLabel(pnl2, "Y", 5.8f, 1.0f);
		txtScaleY = new GraphicsUI::TextBox(pnl2);
		txtScaleY->Posit(EM(4.5f), EM(2.0f), EM(2.8f), EM(1.0f));
		txtScaleY->SetText("1.0");
		txtScaleY->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeScaleY);

		addLabel(pnl2, "Z", 9.8f, 1.0f);
		txtScaleZ = new GraphicsUI::TextBox(pnl2);
		txtScaleZ->Posit(EM(8.5f), EM(2.0f), EM(2.8f), EM(1.0f));
		txtScaleZ->SetText("1.0");
		txtScaleZ->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeScaleZ);

		addLabel(pnl2, "Rotation", 0, 4.0f, 4.0f, 1.0f);

		addLabel(pnl2, "X", 1.8f, 5.0f);
		txtX = new GraphicsUI::TextBox(pnl2);
		txtX->Posit(EM(0.5f), EM(6.0f), EM(2.8f), EM(1.0f));
		txtX->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeRotation);

		addLabel(pnl2, "Y", 5.8f, 5.0f);
		txtY = new GraphicsUI::TextBox(pnl2);
		txtY->Posit(EM(4.5f), EM(6.0f), EM(2.8f), EM(1.0f));
		txtY->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeRotation);

		addLabel(pnl2, "Z", 9.8f, 5.0f);
		txtZ = new GraphicsUI::TextBox(pnl2);
		txtZ->Posit(EM(8.5f), EM(6.0f), EM(2.8f), EM(1.0f));
		txtZ->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::ChangeRotation);
		
		auto lblBones = new GraphicsUI::Label(pnl2);
		lblBones->SetText("Bones");
		lblBones->DockStyle = GraphicsUI::Control::dsBottom;
		
		lstBones = new GraphicsUI::ListBox(pnl);
		lstBones->DockStyle = GraphicsUI::Control::dsFill;
		lstBones->OnClick.Bind(this, &SkeletonRetargetVisualizerActor::SelectedBoneChanged);

		txtInfo = GraphicsUI::CreateMultiLineTextBox(pnl2);
		txtInfo->Posit(EM(0.2f), EM(9.0f), EM(11.6f), EM(8.5f));

		pnlBoneMapping = new GraphicsUI::VScrollPanel(uiEntry);
		pnlBoneMapping->BackColor = pnl->BackColor;
		pnlBoneMapping->SetWidth(EM(14.0f));
		pnlBoneMapping->DockStyle = GraphicsUI::Control::dsRight;

		auto pnlBottom = new GraphicsUI::Container(uiEntry);
		pnlBottom->BackColor = pnl->BackColor;
		pnlBottom->Padding = EM(0.5f);
		pnlBottom->Padding.Top = EM(0.3f);
		pnlBottom->SetHeight(EM(3.0f));
		pnlBottom->DockStyle = GraphicsUI::Control::dsBottom;
		lblFrame = new GraphicsUI::Label(pnlBottom);
		lblFrame->Posit(0, EM(0.0f), EM(10.0f), EM(1.0f));
		lblFrame->SetText("BindPose");
		scTimeline = new GraphicsUI::ScrollBar(pnlBottom);
		scTimeline->SetValue(0, 10, 0, 1);
		scTimeline->DockStyle = GraphicsUI::Control::dsBottom;
		scTimeline->OnChanged.Bind(this, &SkeletonRetargetVisualizerActor::scTimeline_Changed);

		infoForm = new GraphicsUI::Form(uiEntry);
		infoForm->Posit(EM(10.0f), EM(10.0f), EM(20.0f), EM(15.0f));
		infoForm->SetText("Info");
		infoFormTextBox = CreateMultiLineTextBox(infoForm);
		infoFormTextBox->DockStyle = GraphicsUI::Control::dsFill;
		infoForm->SizeChanged();
		uiEntry->CloseWindow(infoForm);
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
		yAxisMaterial.LoadFromFile(solidMaterialFileName);
		yAxisMaterial.SetVariable("solidColor", Vec3::Create(0.0f, 1.0f, 0.0f));
		zAxisMaterial.LoadFromFile(solidMaterialFileName);
		zAxisMaterial.SetVariable("solidColor", Vec3::Create(0.0f, 0.0f, 1.0f));

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
			mb.AddBox(Vec3::Create(1.5f, -1.5f, -1.5f), Vec3::Create(100.0f, 1.5f, 1.5f));
			auto mesh = mb.ToMesh();
			xAxisDrawable = param.rendererService->CreateStaticDrawable(&mesh, 0, &xAxisMaterial, false);
			xAxisDrawable->UpdateTransformUniform(identity);
		}
		param.sink->AddDrawable(xAxisDrawable.Ptr());
		if (!yAxisDrawable)
		{
			MeshBuilder mb;
			mb.AddBox(Vec3::Create(-1.5f, 0.0f, -1.5f), Vec3::Create(1.5f, 100.0f, 1.5f));
			auto mesh = mb.ToMesh();
			yAxisDrawable = param.rendererService->CreateStaticDrawable(&mesh, 0, &yAxisMaterial, false);
			yAxisDrawable->UpdateTransformUniform(identity);
			
		}
		param.sink->AddDrawable(yAxisDrawable.Ptr());
		if (!zAxisDrawable)
		{
			MeshBuilder mb;
			mb.AddBox(Vec3::Create(-1.5f, -1.5f, 1.5f), Vec3::Create(1.5f, 1.5f, 100.0f));
			auto mesh = mb.ToMesh();
			zAxisDrawable = param.rendererService->CreateStaticDrawable(&mesh, 0, &zAxisMaterial, false);
			zAxisDrawable->UpdateTransformUniform(identity);
		}
		param.sink->AddDrawable(zAxisDrawable.Ptr());
		if (sourceModel)
		{
			auto sourceSkeleton = sourceModel->GetSkeleton();
			if (sourceModelInstance.IsEmpty())
			{
				sourceModelInstance = sourceModel->GetDrawableInstance(param);
			}
			Pose p;
			p.Transforms.SetSize(sourceModel->GetSkeleton()->Bones.Count());
			for (int i = 0; i < p.Transforms.Count(); i++)
				p.Transforms[i] = sourceSkeleton->Bones[i].BindPose;
			for (auto & d : sourceModelInstance.Drawables)
			{
				d->Bounds.Min = Vec3::Create(-1e9f);
				d->Bounds.Max = Vec3::Create(1e9f);
				param.sink->AddDrawable(d.Ptr());
			}
			if (!sourceSkeletonDrawable)
				sourceSkeletonDrawable = param.rendererService->CreateSkeletalDrawable(&sourceSkeletonMesh, 0, sourceModel->GetSkeleton(), &sourceSkeletonMaterial, false);
			Matrix4 offset1;
			Matrix4::Translation(offset1, 300.0f, 0.0f, 0.0f);
			if (targetSkeleton)
			{
				Pose p = GetCurrentPose();
				sourceSkeletonDrawable->UpdateTransformUniform(offset1, p, &retargetFile);
				sourceModelInstance.UpdateTransformUniform(identity, p, &retargetFile);
			}
			else
			{
				Pose p;
				p.Transforms.SetSize(sourceSkeleton->Bones.Count());
				for (int i = 0; i < p.Transforms.Count(); i++)
					p.Transforms[i] = sourceSkeleton->Bones[i].BindPose;
				sourceSkeletonDrawable->UpdateTransformUniform(offset1, p, nullptr);
				sourceModelInstance.UpdateTransformUniform(identity, p, nullptr);
			}
			param.sink->AddDrawable(sourceSkeletonDrawable.Ptr());
		}
		if (targetSkeleton)
		{
			if (!targetSkeletonDrawable)
			{
				targetSkeletonDrawable = param.rendererService->CreateSkeletalDrawable(&targetSkeletonMesh, 0, targetSkeleton.Ptr(), &targetSkeletonMaterial, false);
			}
			Matrix4 offset2;
			Matrix4::Translation(offset2, 300.0f, 0.0f, 200.0f);
			Pose p = GetCurrentPose();
			targetSkeletonDrawable->UpdateTransformUniform(offset2, p, nullptr);
			param.sink->AddDrawable(targetSkeletonDrawable.Ptr());
		}
	}
};

void RegisterRetargetActor()
{
	Engine::Instance()->RegisterActorClass("SkeletonRetargetVisualizer", []() {return new SkeletonRetargetVisualizerActor(); });
}

#endif