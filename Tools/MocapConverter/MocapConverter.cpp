#include "CoreLib/Basic.h"
#include "CoreLib/LibIO.h"
#include "Skeleton.h"
#include "Mesh.h"
#include "MeshBuilder.h"
#include "WinForm/WinButtons.h"
#include "WinForm/WinCommonDlg.h"
#include "WinForm/WinForm.h"
#include "WinForm/WinApp.h"
#include "WinForm/WinListBox.h"
#include "WinForm/WinTextBox.h"
#include "RigMapping.h"
#include "BvhFile.h"
#include "Parser.h"
#include <assert.h>

using namespace CoreLib;
using namespace CoreLib::IO;
using namespace CoreLib::Graphics;
using namespace GameEngine;
using namespace VectorMath;
using namespace CoreLib::WinForm;
using namespace GameEngine::Tools;


class ExportArguments
{
public:
	String FileName;
	String SkeletonFileName;
	String RigMappingFileName;
	String EulerOrder = "AUTO";
	bool ExportSkeleton = true;
	bool ExportMesh = false;
	bool ExportAnimation = true;
	bool FlipYZ = true;
};

Vec3 FlipYZ(const Vec3 & v)
{
	return Vec3::Create(v.x, v.z, -v.y);
}

template<typename TFunc>
void TraverseBvhJoints(BvhJoint * joint, const TFunc & f)
{
	f(joint);
	for (auto & child : joint->SubJoints)
		TraverseBvhJoints(joint, f);
}

void TraverseBvhJoints(BvhJoint * parent, BvhJoint * joint, Skeleton & skeleton)
{
	Bone b;
	b.Name = joint->Name;
	b.BindPose.Translation = joint->Offset;
	b.ParentId = parent ? skeleton.BoneMapping[parent->Name]() : -1;
	skeleton.BoneMapping[joint->Name] = skeleton.Bones.Count();
	skeleton.Bones.Add(b);
	for (auto & child : joint->SubJoints)
		TraverseBvhJoints(joint, child.Ptr(), skeleton);
}

void FindBBox(BBox & bbox, BvhJoint * joint)
{
	bbox.Union(joint->Offset);
	for (auto & child : joint->SubJoints)
		FindBBox(bbox, child.Ptr());
}

void GatherNonTrivialBvhJoints(List<BvhJoint*> & list, BvhJoint * joint)
{
	if (joint->Channels.Count())
		list.Add(joint);
	for (auto & child : joint->SubJoints)
		GatherNonTrivialBvhJoints(list, child.Ptr());
}

void RotateKeyFrame(BoneTransformation & kf, Vec3 rotation)
{
	Matrix4 xMat, yMat, zMat, rot, invRot;
	Matrix4::RotationX(xMat, rotation.x * (Math::Pi / 180.0f));
	Matrix4::RotationY(yMat, rotation.y * (Math::Pi / 180.0f));
	Matrix4::RotationZ(zMat, rotation.z * (Math::Pi / 180.0f));
	Matrix4::Multiply(rot, yMat, zMat);
	Matrix4::Multiply(rot, xMat, rot);
	Matrix4::RotationX(xMat, -rotation.x * (Math::Pi / 180.0f));
	Matrix4::RotationY(yMat, -rotation.y * (Math::Pi / 180.0f));
	Matrix4::RotationZ(zMat, -rotation.z * (Math::Pi / 180.0f));
	Matrix4::Multiply(invRot, yMat, zMat);
	Matrix4::Multiply(invRot, xMat, invRot);

	Matrix4 transform = kf.ToMatrix();

	Matrix4::Multiply(transform, transform, rot);
	Matrix4::Multiply(transform, invRot, transform);

	kf.Rotation = Quaternion::FromMatrix(transform.GetMatrix3());
	kf.Rotation *= 1.0f / kf.Rotation.Length();
	kf.Translation = Vec3::Create(transform.values[12], transform.values[13], transform.values[14]);
}

void FlipKeyFrame(BoneTransformation & kf)
{
	Matrix4 transform = kf.ToMatrix();
	Matrix4 rotX90, rotXNeg90;
	Matrix4::RotationX(rotX90, Math::Pi * 0.5f);
	Matrix4::RotationX(rotXNeg90, -Math::Pi * 0.5f);

	Matrix4::Multiply(transform, transform, rotX90);
	Matrix4::Multiply(transform, rotXNeg90, transform);
	
	kf.Rotation = Quaternion::FromMatrix(transform.GetMatrix3());
	kf.Rotation *= 1.0f / kf.Rotation.Length();
	kf.Translation = Vec3::Create(transform.values[12], transform.values[13], transform.values[14]);

	Swap(kf.Scale.y, kf.Scale.z);
}

void FlipKeyFrameCoordinateSystem(BoneTransformation & kf)
{
	Matrix4 transform = kf.ToMatrix();
	Matrix4 flipMatrix;
	Matrix4::Scale(flipMatrix, 1.0f, 1.0f, -1.0f);
	Matrix4::Multiply(transform, transform, flipMatrix);
	Matrix4::Multiply(transform, flipMatrix, transform);

	kf.Rotation = Quaternion::FromMatrix(transform.GetMatrix3());
	kf.Rotation *= 1.0f / kf.Rotation.Length();
	kf.Translation = Vec3::Create(transform.values[12], transform.values[13], transform.values[14]);
}

Skeleton Retarget(const Skeleton & modelSkeleton, const Skeleton & motionSkeleton, const Vec3 & rootRotatation)
{
    Skeleton result = modelSkeleton;
    auto getMatrices = [](const Skeleton & skeleton)
    {
        List<Matrix4> mats;
        mats.SetSize(skeleton.Bones.Count());
        for (int i = 0; i < skeleton.Bones.Count(); i++)
        {
            auto& bone = skeleton.Bones[i];
            mats[i] = bone.BindPose.ToMatrix();
            if (bone.ParentId != -1)
                Matrix4::Multiply(mats[i], mats[bone.ParentId], mats[i]);
        }
        return mats;
    };
    auto modelSkeletonMatrices = getMatrices(modelSkeleton);
    auto selectPosition = [](const Matrix4 &mat)
    {
        Vec4 rs;
        mat.Transform(rs, Vec4::Create(0.0f, 0.0f, 0.0f, 1.0f));
        return rs.xyz();
    };
    auto modelSkeletonPositions = From(modelSkeletonMatrices).Select(selectPosition).ToList();
    EnumerableDictionary<String, Vec3> positions;
    for (auto & bone : motionSkeleton.Bones)
        positions[bone.Name] = modelSkeletonPositions[modelSkeleton.BoneMapping[bone.Name]()];
	Matrix4 xMat, yMat, zMat, rot;
	Matrix4::RotationX(xMat, rootRotatation.x * (Math::Pi / 180.0f));
	Matrix4::RotationY(yMat, rootRotatation.y * (Math::Pi / 180.0f));
	Matrix4::RotationZ(zMat, rootRotatation.z * (Math::Pi / 180.0f));
	Matrix4::Multiply(rot, xMat, zMat);
	Matrix4::Multiply(rot, yMat, rot);
    for (int i = 0; i < modelSkeleton.Bones.Count(); i++)
    {
        auto offset = modelSkeleton.Bones[i].ParentId == -1 ? modelSkeletonPositions[i] : modelSkeletonPositions[i] - modelSkeletonPositions[modelSkeleton.Bones[i].ParentId];

        result.Bones[i].BindPose.Translation = rot.TransformNormal(offset);
        result.Bones[i].BindPose.Rotation = Quaternion();
		
		auto absoluteOffset = rot.TransformNormal(modelSkeletonPositions[i]);
        Matrix4::Translation(result.InversePose[i], -absoluteOffset.x, -absoluteOffset.y, -absoluteOffset.z);
        Matrix4::Multiply(result.InversePose[i], result.InversePose[i], rot);
    }
    return result;
}

struct SkeletonMeshVertex
{
	Vec3 pos;
	Quaternion tangentFrame;
	int boneId;
};

Mesh CreateStickmanMesh(const BBox& bbox, Skeleton * skeleton, float width)
{
	Mesh rs;
	rs.FromSkeleton(skeleton, (bbox.Max - bbox.Min).Length() * 0.08f);

	//
	// add head and hands to mesh
	//
	List<SkeletonMeshVertex> vertices;
	List<Matrix4> forwardTransforms;
	List<Vec3> positions;
	positions.SetSize(skeleton->Bones.Count());
	forwardTransforms.SetSize(skeleton->Bones.Count());
	for (int i = 0; i < skeleton->Bones.Count(); i++)
	{
		forwardTransforms[i] = skeleton->Bones[i].BindPose.ToMatrix();
		if (skeleton->Bones[i].ParentId != -1)
			Matrix4::Multiply(forwardTransforms[i], forwardTransforms[skeleton->Bones[i].ParentId], forwardTransforms[i]);

		positions[i] = Vec3::Create(forwardTransforms[i].values[12], forwardTransforms[i].values[13], forwardTransforms[i].values[14]);
	}

	auto appendMesh = [&](Mesh & m, Mesh & m1, int boneId)
	{
		int oldVerNum = m.GetVertexCount();
		int boxVerticesNum = m1.GetVertexCount();
		int verticesNum = oldVerNum + boxVerticesNum;
		m.AllocVertexBuffer(verticesNum);

		for (int i = 0; i < boxVerticesNum; i++)
		{
			m.SetVertexPosition(i + oldVerNum, m1.GetVertexPosition(i));
			m.SetVertexTangentFrame(i + oldVerNum, m1.GetVertexTangentFrame(i));
			m.SetVertexSkinningBinding(i + oldVerNum, MakeArrayView(20), MakeArrayView(1.0f));
		}
		for (auto & idx : m1.Indices)
			idx += oldVerNum;
		m.Indices.AddRange(m1.Indices);
		m.Bounds.Union(m1.Bounds);
	};

	// head
	const int headBoneId = 20;
	{
		MeshBuilder mb;
		mb.AddBox(Vec3::Create(-1.5f, 0.0f, -2.2f), Vec3::Create(1.5f, 6.0f, 2.2f));
		//mb.AddBox(Vec3::Create(1.5f, 2.5f, -0.5f), Vec3::Create(25.5f, 3.5f, 0.5f));
		Mesh head = mb.ToMesh();
		
		Vec3 neckDir = positions[headBoneId] - positions[headBoneId - 1];
		neckDir.Normalize();
		Vec3 normX = Vec3::Cross(neckDir, Vec3::Create(0.0f, 1.0f, 0.0f));
		Vec3 normZ = Vec3::Cross(normX, neckDir);
		Matrix4 rot;
		//Matrix4::CreateIdentityMatrix(rot);
		//rot.values[0] = normX.x;
		//rot.values[1] = normX.y;
		//rot.values[2] = normX.z;

		//rot.values[4] = neckDir.x;
		//rot.values[5] = neckDir.y;
		//rot.values[6] = neckDir.z;

		//rot.values[8] = normZ.x;
		//rot.values[9] = normZ.y;
		//rot.values[10] = normZ.z;
		//// test
		//Vec3 test = rot.GetMatrix3().Transform(Vec3::Create(0.0f, 1.0f, 0.0f));


		Matrix4::RotationZ(rot, -0.44f);
		//rot = forwardTransforms[headBoneId];
		Vec3 headPos = positions[headBoneId];
		rot.values[12] = headPos.x;
		rot.values[13] = headPos.y;
		rot.values[14] = headPos.z;
		head = MeshBuilder::TransformMesh(head, rot);
		appendMesh(rs, head, headBoneId);
	}
	return rs;
}

void Export(ExportArguments args)
{
	BvhFile file = BvhFile::FromFile(args.FileName);
	Skeleton skeleton;

	if (file.Hierarchy)
	{
		TraverseBvhJoints(nullptr, file.Hierarchy.Ptr(), skeleton);
		// compute inverse binding pose
		skeleton.InversePose.SetSize(skeleton.Bones.Count());
		for (int i = 0; i < skeleton.InversePose.Count(); i++)
		{
			auto & bone = skeleton.Bones[i];
			if (args.FlipYZ)
			{
				bone.BindPose.Translation = FlipYZ(bone.BindPose.Translation);
			}
			if (bone.ParentId != -1)
				Matrix4::Multiply(skeleton.InversePose[i], skeleton.InversePose[bone.ParentId], bone.BindPose.ToMatrix());
			else
				skeleton.InversePose[i] = bone.BindPose.ToMatrix();
		}
		for (auto & bone : skeleton.InversePose)
		{
			bone.Inverse(bone);
		}
	}
	float animTranslationScale = 1.0f;
    if (args.SkeletonFileName.Length())
    {
		Skeleton modelSkeleton;
        modelSkeleton.LoadFromFile(args.SkeletonFileName);
		if (args.RigMappingFileName.Length())   // retarget name
		{
			RigMappingFile rig(args.RigMappingFileName);
			animTranslationScale = rig.TranslationScale;
			for (auto & bone : modelSkeleton.Bones)
			{
				String newName;
				if (rig.Mapping.TryGetValue(bone.Name, newName))
				{
					modelSkeleton.BoneMapping[newName] = modelSkeleton.BoneMapping[bone.Name]();
					bone.Name = newName;
				}
			}
			skeleton = Retarget(modelSkeleton, skeleton, rig.RootRotation);
		}
		else
			skeleton = modelSkeleton;
    }

    if (args.ExportSkeleton)
        skeleton.SaveToFile(Path::ReplaceExt(args.FileName, "skeleton"));

	if (args.ExportAnimation)
	{
		List<BvhJoint*> joints;
		GatherNonTrivialBvhJoints(joints, file.Hierarchy.Ptr());
		SkeletalAnimation anim;
		anim.Speed = 1.0f;
        Dictionary<int, int> boneIdToChannelId;
		for (auto & joint : joints)
		{
			AnimationChannel ch;
			int boneId = skeleton.BoneMapping[joint->Name]();
			ch.BoneId = boneId;
			ch.BoneName = joint->Name;
            boneIdToChannelId[boneId] = anim.Channels.Count();
            anim.Channels.Add(ch);

		}
		int frameId = 0;
		int ptr = 0;
		auto readData = [&]() 
		{
			if (ptr < file.FrameData.Count())
			{
				float data = file.FrameData[ptr];
				ptr++;
				return data;
			}
			else
			{
				throw InvalidOperationException("MOTION data size does not match HIERARCHY channels declaration.");
			}
		};
        bool orderDetermined = args.EulerOrder != "AUTO";
        StringBuilder orderSB;
		while (ptr < file.FrameData.Count())
		{
			anim.Duration = file.FrameDuration * frameId;
			for (auto joint : joints)
			{
				int boneId = skeleton.BoneMapping[joint->Name]();
				AnimationKeyFrame keyFrame;
				float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;
				bool hasTranslation = false;
				for (auto & c : joint->Channels)
				{
					switch (c)
					{
					case ChannelType::XPos:
						keyFrame.Transform.Translation.x = readData() * animTranslationScale;
						hasTranslation = true;
						break;
					case ChannelType::YPos:
						keyFrame.Transform.Translation.y = readData() * animTranslationScale;
						hasTranslation = true;
						break;
					case ChannelType::ZPos:
						keyFrame.Transform.Translation.z = readData() * animTranslationScale;
						hasTranslation = true;
						break;
					case ChannelType::XScale:
						keyFrame.Transform.Scale.x = readData();
						break;
					case ChannelType::YScale:
						keyFrame.Transform.Scale.y = readData();
						break;
					case ChannelType::ZScale:
						keyFrame.Transform.Scale.z = readData();
						break;
					case ChannelType::XRot:
						rotX = readData();
                        if (!orderDetermined) 
                            orderSB.Append("X");
						break;
					case ChannelType::YRot:
						rotY = readData();
                        if (!orderDetermined)
                            orderSB.Append("Y");
						break;
					case ChannelType::ZRot:
						rotZ = readData();
                        if (!orderDetermined)
                            orderSB.Append("Z");
						break;
					}
				}
                if (!orderDetermined && orderSB.Length() == 3)
                {
                    args.EulerOrder = orderSB.ProduceString();
                    args.EulerOrder = String(args.EulerOrder[2]) + String(args.EulerOrder[1]) + String(args.EulerOrder[0]);
                    orderDetermined = true;
                }
				Matrix4 xMat, yMat, zMat, rot;
				Matrix4::RotationX(xMat, rotX * (Math::Pi / 180.0f));
				Matrix4::RotationY(yMat, rotY * (Math::Pi / 180.0f));
				Matrix4::RotationZ(zMat, rotZ * (Math::Pi / 180.0f));
				if (args.EulerOrder == "ZXY")
				{
					Matrix4::Multiply(rot, xMat, zMat);
					Matrix4::Multiply(rot, yMat, rot);
				}
				else if (args.EulerOrder == "ZYX")
				{
					Matrix4::Multiply(rot, yMat, zMat);
					Matrix4::Multiply(rot, xMat, rot);
				}
				else if (args.EulerOrder == "YXZ")
				{
					Matrix4::Multiply(rot, xMat, yMat);
					Matrix4::Multiply(rot, zMat, rot);
				}
				else if (args.EulerOrder == "XYZ")
				{
					Matrix4::Multiply(rot, yMat, xMat);
					Matrix4::Multiply(rot, zMat, rot);
				}
				else if (args.EulerOrder == "XZY")
				{
					Matrix4::Multiply(rot, zMat, xMat);
					Matrix4::Multiply(rot, yMat, rot);
				}
				else //if (args.EulerOrder == "YZX")
				{
					Matrix4::Multiply(rot, zMat, yMat);
					Matrix4::Multiply(rot, xMat, rot);
				}
				keyFrame.Transform.Rotation = Quaternion::FromMatrix(rot.GetMatrix3());
				if (args.FlipYZ)
				{
					FlipKeyFrame(keyFrame.Transform);
				}
				if (!hasTranslation)
				{
					keyFrame.Transform.Translation = skeleton.Bones[boneId].BindPose.Translation;
				}
				keyFrame.Time = file.FrameDuration * frameId;
				anim.Channels[boneIdToChannelId[boneId]].KeyFrames.Add(keyFrame);
			}
			frameId++;
		}
		anim.SaveToFile(Path::ReplaceExt(args.FileName, "anim"));
	}
	
    if (args.ExportMesh)
	{
		BBox bbox;
		bbox.Init();
		FindBBox(bbox, file.Hierarchy.Ptr());
		Mesh mesh = CreateStickmanMesh(bbox, &skeleton, (bbox.Max - bbox.Min).Length() * 0.08f);
		mesh.SaveToFile(Path::ReplaceExt(args.FileName, "mesh"));
	}
}

class MocapConverterForm : public Form
{
private:
	RefPtr<Button> btnSelectFiles;
	RefPtr<CheckBox> chkFlipYZ, chkCreateMesh, chkConvertLHC;
	RefPtr<ComboBox> cmbEuler;
	RefPtr<Label> lblSkeleton, lblRig;
	RefPtr<TextBox> txtSkeletonFile, txtRigFile;
	RefPtr<Button> btnSelectSkeletonFile, btnSelectRigFile;
public:
	MocapConverterForm()
	{
		SetText("Convert Mocap Data");
		SetClientWidth(420);
		SetClientHeight(240);
		CenterScreen();
		SetMaximizeBox(false);
		SetBorder(fbFixedDialog);
		chkFlipYZ = new CheckBox(this);
		chkFlipYZ->SetPosition(30, 20, 120, 25);
		chkFlipYZ->SetText("Flip YZ");
		chkFlipYZ->SetChecked(ExportArguments().FlipYZ);
		chkCreateMesh = new CheckBox(this);
		chkCreateMesh->SetPosition(30, 50, 120, 25);
		chkCreateMesh->SetText("Create Mesh");
		chkCreateMesh->SetChecked(ExportArguments().ExportMesh);
		cmbEuler = new ComboBox(this);
		cmbEuler->AddItem("Euler Angle Order");
		cmbEuler->AddItem("ZXY");
		cmbEuler->AddItem("ZYX");
		cmbEuler->AddItem("YXZ");
		cmbEuler->AddItem("XYZ");
		cmbEuler->AddItem("XZY");
		cmbEuler->AddItem("YZX");

		cmbEuler->SetPosition(30, 80, 120, 30);
		cmbEuler->SetSelectionIndex(0);

		lblSkeleton = new Label(this);
		lblSkeleton->SetText("Retarget Skeleton:");
		lblSkeleton->SetPosition(30, 113, 120, 30);
		txtSkeletonFile = new TextBox(this);
		txtSkeletonFile->SetText("");
		txtSkeletonFile->SetPosition(150, 110, 140, 25);
		btnSelectSkeletonFile = new Button(this);
		btnSelectSkeletonFile->SetText("Browse");
		btnSelectSkeletonFile->SetPosition(300, 110, 80, 25);

		btnSelectSkeletonFile->OnClick.Bind([this](Object*, EventArgs)
		{
			FileDialog dlg(this);
			dlg.Filter = "Skeleton|*.skeleton";
			if (dlg.ShowOpen())
				txtSkeletonFile->SetText(dlg.FileName);
		});


		lblRig = new Label(this);
		lblRig->SetText("Rig Mapping:");
		lblRig->SetPosition(30, 143, 120, 30);
		txtRigFile = new TextBox(this);
		txtRigFile->SetText("");
		txtRigFile->SetPosition(150, 140, 140, 25);
		btnSelectRigFile = new Button(this);
		btnSelectRigFile->SetText("Browse");
		btnSelectRigFile->SetPosition(300, 140, 80, 25);

		btnSelectRigFile->OnClick.Bind([this](Object*, EventArgs)
		{
			FileDialog dlg(this);
			dlg.Filter = "Rig Mapping|*.rig";
			if (dlg.ShowOpen())
				txtRigFile->SetText(dlg.FileName);
		});


		btnSelectFiles = new Button(this);
		btnSelectFiles->SetPosition(90, 170, 120, 30);
		btnSelectFiles->SetText("Select Files");
		btnSelectFiles->OnClick.Bind([=](Object*, EventArgs)
		{
			try
			{
				FileDialog dlg(this);
				dlg.MultiSelect = true;
				if (dlg.ShowOpen())
				{
					for (auto file : dlg.FileNames)
					{
						ExportArguments args;
						args.FlipYZ = chkFlipYZ->GetChecked();
						args.ExportMesh = chkCreateMesh->GetChecked();
						args.FileName = file;
						args.SkeletonFileName = txtSkeletonFile->GetText();
						args.RigMappingFileName = txtRigFile->GetText();
						if (cmbEuler->GetSelectionIndex() > 0)
							args.EulerOrder = cmbEuler->GetItem(cmbEuler->GetSelectionIndex());
						SetCursor(LoadCursor(NULL, IDC_WAIT));
						Export(args);
						SetCursor(LoadCursor(NULL, IDC_ARROW));
					}
				}
			}
			catch (const Exception & e)
			{
				MessageBox(String("Error: ") + e.Message, "Error", MB_ICONEXCLAMATION);
			}
		});
	}
};

int wmain(int argc, const wchar_t** argv)
{
	if (argc > 1)
	{
		try
		{
			ExportArguments args;
			args.FlipYZ = false;
			args.ExportMesh = false;
			args.FileName = String::FromWString(argv[1]);
			for (int i = 2; i < argc; i++)
				if (String::FromWString(argv[i]) == "-flipyz")
					args.FlipYZ = true;
				else if (String::FromWString(argv[i]) == "-mesh")
					args.ExportMesh = true;
				else if (String::FromWString(argv[i]) == "-euler" && i < argc - 1)
					args.EulerOrder = String::FromWString(argv[i + 1]);
				else if (String::FromWString(argv[i]) == "-skeleton" && i < argc - 1)
					args.SkeletonFileName = String::FromWString(argv[i + 1]);
				else if (String::FromWString(argv[i]) == "-rig" && i < argc - 1)
					args.RigMappingFileName = String::FromWString(argv[i + 1]);
			Export(args);
		}
		catch (const Exception & e)
		{
			printf("Error: %S\n", e.Message.ToWString());
			return 1;
		}
	}
	else
	{
		Application::Init();
		Application::Run(new MocapConverterForm());
		Application::Dispose();
	}
	_CrtDumpMemoryLeaks();
	return 0;
}