#include "FBXImport/include/Importer.hpp"
#include "FBXImport/include/scene.h"
#include "FBXImport/include/postprocess.h"
#include "CoreLib/Basic.h"
#include "CoreLib/LibIO.h"
#include "Skeleton.h"
#include "Mesh.h"
#include "WinForm/WinButtons.h"
#include "WinForm/WinCommonDlg.h"
#include "WinForm/WinForm.h"
#include "WinForm/WinApp.h"


using namespace CoreLib;
using namespace CoreLib::IO;
using namespace GameEngine;
using namespace VectorMath;

Quaternion ToQuaternion(const aiQuaternion & q)
{
	Quaternion rs;
	rs.x = q.x;
	rs.y = q.y;
	rs.z = q.z;
	rs.w = q.w;
	return rs;
}

void IndentString(StringBuilder & sb, String src)
{
	int indent = 0;
	bool beginTrim = true;
	for (int c = 0; c < src.Length(); c++)
	{
		auto ch = src[c];
		if (ch == '\n')
		{
			sb << "\n";

			beginTrim = true;
		}
		else
		{
			if (beginTrim)
			{
				while (c < src.Length() - 1 && (src[c] == '\t' || src[c] == '\n' || src[c] == '\r' || src[c] == ' '))
				{
					c++;
					ch = src[c];
				}
				for (int i = 0; i < indent - 1; i++)
					sb << '\t';
				if (ch != '}' && indent > 0)
					sb << '\t';
				beginTrim = false;
			}

			if (ch == '{')
				indent++;
			else if (ch == '}')
				indent--;
			if (indent < 0)
				indent = 0;

			sb << ch;
		}
	}
}

void ConvertLevelNode(StringBuilder & sb, const aiScene * scene, aiNode * node)
{
	for (auto i = 0u; i < node->mNumMeshes; i++)
	{
		sb << "StaticMesh\n{\n";
		sb << "name \"" << String(node->mName.C_Str()) << "\"\n";
		sb << "transform [";
		for (int i = 0; i < 16; i++)
			sb << String(node->mTransformation[i%4][i/4]) << " ";
		sb << "]\n";
		auto name = String(scene->mMeshes[node->mMeshes[i]]->mName.C_Str());
		if (name.Length() == 0)
			name = "mesh_" + String((int)(node->mMeshes[i])) + ".mesh";
		sb << "mesh \"" << name << "\"\n";
		for (auto i = 0u; i < node->mNumChildren; i++)
		{
			ConvertLevelNode(sb, scene, node->mChildren[i]);
		}
		sb << "}\n";
	}
}

class ExportArguments
{
public:
	String FileName;
	bool ExportSkeleton = true;
	bool ExportMesh = true;
	bool ExportAnimation = true;
	bool FlipUV = false;
	bool FlipWindingOrder = false;
	bool FlipYZ = true;
};

using namespace CoreLib::WinForm;

Vec3 FlipYZ(const Vec3 & v)
{
	return Vec3::Create(v.x, v.z, -v.y);
}

Matrix4 FlipYZ(const Matrix4 & v)
{
	Matrix4 rs = v;
	for (int i = 0; i < 4; i++)
	{
		rs.m[1][i] = v.m[2][i];
		rs.m[2][i] = -v.m[1][i];
	}
	for (int i = 0; i < 4; i++)
	{
		Swap(rs.m[i][1], rs.m[i][2]);
		rs.m[i][2] = -rs.m[i][2];
	}
	return rs;
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

void Export(ExportArguments args)
{
	auto fileName = args.FileName;
	auto outFileName = Path::ReplaceExt(fileName, "out");
	printf("loading %S...\n", fileName.ToWString());
	Assimp::Importer importer;
	/*int process = aiProcess_CalcTangentSpace;
	if (args.FlipUV)
		process |= aiProcess_FlipUVs;
	if (args.FlipWindingOrder)
		process |= aiProcess_FlipWindingOrder;*/
	int process = 0;
	auto scene = importer.ReadFile(fileName.Buffer(), process);
	if (scene)
	{
		//importer.ApplyPostProcessing(aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes | aiProcess_ImproveCacheLocality | aiProcess_LimitBoneWeights);
		wprintf(L"input file contains %d meshes, %d animations.\n", scene->mNumMeshes, scene->mNumAnimations);
		if ((args.ExportSkeleton || args.ExportMesh) && scene->mNumMeshes == 0)
		{
			wprintf(L"error: input file does not contain any meshes.\n");
		}
        Skeleton skeleton;
		if ((args.ExportSkeleton || args.ExportMesh) && scene->mNumMeshes > 0)
		{
			if (scene->mNumMeshes > 1)
				wprintf(L"warning: input file contains more than 1 mesh, animation is exported for the first mesh only.\n");
			auto mesh = scene->mMeshes[0];
			if (!mesh->HasBones())
			{
				wprintf(L"error: input mesh has no bones.\n");
			}
			else
			{
				skeleton.Bones.SetSize(mesh->mNumBones);
				List<aiNode*> nodes;
				nodes.SetSize(mesh->mNumBones);
				skeleton.InversePose.SetSize(skeleton.Bones.Count());
				for (auto i = 0u; i < mesh->mNumBones; i++)
				{
					skeleton.Bones[i].Name = mesh->mBones[i]->mName.C_Str();
					skeleton.Bones[i].ParentId = -1;
					skeleton.BoneMapping[skeleton.Bones[i].Name] = i;
					memcpy(skeleton.InversePose[i].values, &mesh->mBones[i]->mOffsetMatrix.Transpose(), sizeof(VectorMath::Matrix4));
					nodes[i] = scene->mRootNode->FindNode(mesh->mBones[i]->mName);
					aiVector3D scale, pos;
					aiQuaternion rot;
					nodes[i]->mTransformation.Decompose(scale, rot, pos);
					skeleton.Bones[i].BindPose.Rotation = ToQuaternion(rot);
					skeleton.Bones[i].BindPose.Scale = Vec3::Create(scale.x, scale.y, scale.z);
					skeleton.Bones[i].BindPose.Translation = Vec3::Create(pos.x, pos.y, pos.z);
					
				}
				for (auto i = 0u; i < mesh->mNumBones; i++)
				{
					if (nodes[i])
					{
						for (auto j = 0u; j < nodes[i]->mNumChildren; j++)
						{
							int boneId = -1;
							if (skeleton.BoneMapping.TryGetValue(nodes[i]->mChildren[j]->mName.C_Str(), boneId))
							{
								skeleton.Bones[boneId].ParentId = i;
							}
						}
					}
				}
                skeleton = skeleton.TopologySort();
				auto node = scene->mRootNode->FindNode(skeleton.Bones[0].Name.Buffer());
				aiMatrix4x4 rootTransform = node->mTransformation;
				while (node->mParent)
				{
					rootTransform = rootTransform * node->mParent->mTransformation;
					node = node->mParent;
				}
				aiVector3D rootScale, rootPos;
				aiQuaternion rootRotation;
				rootTransform.Decompose(rootScale, rootRotation, rootPos);
				skeleton.Bones[0].BindPose.Scale = Vec3::Create(rootScale.x, rootScale.y, rootScale.z);
				skeleton.Bones[0].BindPose.Rotation = ToQuaternion(rootRotation);
				skeleton.Bones[0].BindPose.Translation = Vec3::Create(rootPos.x, rootPos.y, rootPos.z);
				if (args.FlipYZ)
				{
					auto invRot = Quaternion::FromAxisAngle(Vec3::Create(1.0f, 0.0f, 0.0f), -Math::Pi * 0.5f);
					skeleton.Bones[0].BindPose.Rotation = invRot * skeleton.Bones[0].BindPose.Rotation;
					auto mat = skeleton.Bones[0].BindPose.Rotation.ToMatrix3();
					Matrix4 rotMat;
					Matrix4::RotationX(rotMat, Math::Pi * 0.5f);
					for (int i = 0; i < skeleton.Bones.Count(); i++)
					{
						Matrix4::Multiply(skeleton.InversePose[i], skeleton.InversePose[i], rotMat);
					}
				}
                if (args.ExportSkeleton)
                {
                    skeleton.SaveToFile(Path::ReplaceExt(outFileName, "skeleton"));
                    printf("skeleton converted. total bones: %d.\n", skeleton.Bones.Count());
                }
			}
		}

		if (args.ExportMesh && scene->mNumMeshes > 0)
		{
			for (auto mi = 0u; mi < scene->mNumMeshes; mi++)
			{
				auto mesh = scene->mMeshes[mi];
				
				RefPtr<Mesh> meshOut = new Mesh();
				meshOut->Bounds.Init();
				meshOut->SetVertexFormat(MeshVertexFormat((int)mesh->GetNumColorChannels(), (int)mesh->GetNumUVChannels(), true, mesh->HasBones()));
				meshOut->AllocVertexBuffer(mesh->mNumVertices);
				meshOut->Indices.SetSize(mesh->mNumFaces * 3);
				for (auto i = 0u; i < mesh->mNumFaces; i++)
				{
					meshOut->Indices[i * 3] = mesh->mFaces[i].mIndices[0];
					meshOut->Indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
					meshOut->Indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
				}
				for (auto i = 0u; i < mesh->mNumVertices; i++)
				{
					auto vertPos = Vec3::Create(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
					if (args.FlipYZ)
						vertPos = FlipYZ(vertPos);
					meshOut->Bounds.Union(vertPos);
					meshOut->SetVertexPosition(i, vertPos);
					for (auto j = 0u; j < mesh->GetNumUVChannels(); j++)
					{
						auto uv = Vec2::Create(mesh->mTextureCoords[j][i].x, mesh->mTextureCoords[j][i].y);
						meshOut->SetVertexUV(i, j, uv);
					}
					for (auto j = 0u; j < mesh->GetNumColorChannels(); j++)
					{
						auto color = Vec4::Create(mesh->mColors[j][i].r, mesh->mColors[j][i].g, mesh->mColors[j][i].b, mesh->mColors[j][i].a);
						meshOut->SetVertexColor(i, j, color);
					}
					Vec3 normal = Vec3::Create(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
					if (args.FlipYZ)
						normal = FlipYZ(normal);
					Vec3 tangent;
					if (mesh->HasTangentsAndBitangents())
					{
						tangent = Vec3::Create(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
						if (args.FlipYZ)
							tangent = FlipYZ(tangent);
						tangent = (tangent - normal * Vec3::Dot(tangent, normal));
						tangent = tangent.Normalize();
					}
					else
					{
						GetOrthoVec(tangent, normal);
					}
					Vec3 binormal = Vec3::Cross(tangent, normal).Normalize();
					Quaternion q = Quaternion::FromCoordinates(tangent, normal, binormal);
					auto vq = q.ToVec4().Normalize();
					if (vq.w < 0)
						vq = -vq;
					meshOut->SetVertexTangentFrame(i, vq);
				}
				if (mesh->HasBones())
				{
					List<Array<int, 8>> boneIds;
					List<Array<float, 8>> boneWeights;
					boneIds.SetSize(mesh->mNumVertices);
					boneWeights.SetSize(mesh->mNumVertices);
					for (auto i = 0u; i < mesh->mNumVertices; i++)
					{
						boneIds[i].SetSize(4);
						boneWeights[i].SetSize(4);
						boneIds[i][0] = boneIds[i][1] = boneIds[i][2] = boneIds[i][3] = -1;
						boneWeights[i][0] = boneWeights[i][1] = boneWeights[i][2] = boneWeights[i][3] = 0.0f;
					}
					for (auto i = 0u; i < mesh->mNumBones; i++)
					{
						auto bone = mesh->mBones[i];
						for (auto j = 0u; j < bone->mNumWeights; j++)
						{
							int vertId = bone->mWeights[j].mVertexId;
							auto & vBoneIds = boneIds[vertId];
							auto & vBoneWeights = boneWeights[vertId];
							int k = 0;
							while (k < 4 && vBoneIds[k] != -1)
								k++;
							if (k < 4)
							{
								vBoneIds[k] = (unsigned char)skeleton.BoneMapping[bone->mName.C_Str()]();
								vBoneWeights[k] = bone->mWeights[j].mWeight;
								if (vBoneWeights[k] > vBoneWeights[0])
								{
									Swap(vBoneWeights[k], vBoneWeights[0]);
									Swap(vBoneIds[k], vBoneIds[0]);
								}
							}
						}
					}
					for (auto i = 0u; i < mesh->mNumVertices; i++)
					{
						meshOut->SetVertexSkinningBinding(i, boneIds[i].GetArrayView(), boneWeights[i].GetArrayView());
					}
				}
				auto outName = outFileName;
				String meshName;
				if (scene->mNumMeshes > 1)
				{
					meshName = Path::GetFileNameWithoutEXT(fileName) + "_";
					String compName = mesh->mName.C_Str();
					meshName = meshName + String((int)mi);
					/*if (compName.Length() != 0)
						meshName = meshName + "_" + compName;*/
					outName = Path::Combine(Path::GetDirectoryName(outFileName), meshName + ".mesh");
				}
				
				meshOut->SaveToFile(Path::ReplaceExt(outName, "mesh"));
				wprintf(L"mesh converted: %s faces: %d, vertices: %d, bones: %d.\n", meshName.ToWString(), mesh->mNumFaces, mesh->mNumVertices, mesh->mNumBones);
			}
		}

		if (args.ExportAnimation && scene->mAnimations > 0)
		{
			bool inconsistentKeyFrames = false;
			SkeletalAnimation anim;
			auto animIn = scene->mAnimations[0];
			anim.Speed = 1.0f / (float)animIn->mTicksPerSecond;
			memset(anim.Reserved, 0, sizeof(anim.Reserved));
			anim.Name = animIn->mName.C_Str();
			anim.Duration = (float)(animIn->mDuration);
			anim.Channels.SetSize(animIn->mNumChannels);
			for (auto i = 0u; i < animIn->mNumChannels; i++)
			{
				auto channel = animIn->mChannels[i];
				anim.Channels[i].BoneName = animIn->mChannels[i]->mNodeName.C_Str();
				unsigned int ptrPos = 0, ptrRot = 0, ptrScale = 0;
				if (channel->mNumPositionKeys != channel->mNumRotationKeys ||
					channel->mNumRotationKeys != channel->mNumScalingKeys)
				{
					if (!inconsistentKeyFrames)
					{
						inconsistentKeyFrames = true;
						wprintf(L"keyframe count is different across different types of transforms.\n");
					}
				}
				while (ptrPos < channel->mNumPositionKeys || ptrRot < channel->mNumRotationKeys || ptrScale < channel->mNumScalingKeys)
				{
					int minType = 0;
					float currentMin = (float)anim.Duration;
					if (ptrPos < channel->mNumPositionKeys)
						currentMin = (float)(channel->mPositionKeys[ptrPos].mTime);
					if (ptrRot < channel->mNumRotationKeys && (float)channel->mRotationKeys[ptrRot].mTime < currentMin)
					{
						minType = 1;
						currentMin = (float)channel->mRotationKeys[ptrRot].mTime;
					}
					if (ptrScale < channel->mNumScalingKeys && (float)channel->mScalingKeys[ptrScale].mTime < currentMin)
					{
						minType = 2;
						currentMin = (float)channel->mScalingKeys[ptrScale].mTime;
					}
					AnimationKeyFrame keyFrame;
					keyFrame.Time = currentMin;
					if (minType == 0)
					{
						auto & val = channel->mPositionKeys[ptrPos].mValue;
						keyFrame.Transform.Translation = Vec3::Create(val.x, val.y, val.z);
					}
					else
					{
						if (ptrPos == 0 && channel->mNumPositionKeys > 0)
						{
							auto & val = channel->mPositionKeys[0].mValue;
							keyFrame.Transform.Translation = Vec3::Create(val.x, val.y, val.z);
						}
						else if (ptrPos < channel->mNumPositionKeys)
						{
							auto & val = channel->mPositionKeys[ptrPos].mValue;
							keyFrame.Transform.Translation = Vec3::Lerp(anim.Channels[i].KeyFrames.Last().Transform.Translation,
								Vec3::Create(val.x, val.y, val.z),
								(currentMin - anim.Channels[i].KeyFrames.Last().Time) /
								(float)(channel->mPositionKeys[ptrPos].mTime - anim.Channels[i].KeyFrames.Last().Time));
						}
					}
					if (minType == 1)
					{
						keyFrame.Transform.Rotation = ToQuaternion(channel->mRotationKeys[ptrRot].mValue);
					}
					else
					{
						if (ptrRot == 0 && channel->mNumRotationKeys > 0)
							keyFrame.Transform.Rotation = ToQuaternion(channel->mRotationKeys[0].mValue);
						else if (ptrRot < channel->mNumRotationKeys)
							keyFrame.Transform.Rotation =
							Quaternion::Slerp(anim.Channels[i].KeyFrames.Last().Transform.Rotation, ToQuaternion(channel->mRotationKeys[ptrRot].mValue),
							(currentMin - anim.Channels[i].KeyFrames.Last().Time) /
								(float)(channel->mRotationKeys[ptrRot].mTime - anim.Channels[i].KeyFrames.Last().Time));
					}
					if (minType == 2)
					{
						auto & val = channel->mScalingKeys[ptrScale].mValue;
						keyFrame.Transform.Scale = Vec3::Create(val.x, val.y, val.z);
					}
					else
					{
						if (ptrScale == 0 && channel->mNumScalingKeys > 0)
						{
							auto & val = channel->mScalingKeys[0].mValue;
							keyFrame.Transform.Scale = Vec3::Create(val.x, val.y, val.z);
						}
						else if (ptrScale < channel->mNumScalingKeys)
						{
							auto & val = channel->mScalingKeys[ptrScale].mValue;
							keyFrame.Transform.Scale =
								Vec3::Lerp(anim.Channels[i].KeyFrames.Last().Transform.Scale,
									Vec3::Create(val.x, val.y, val.z),
									(currentMin - anim.Channels[i].KeyFrames.Last().Time) /
									(float)(channel->mScalingKeys[ptrScale].mTime - anim.Channels[i].KeyFrames.Last().Time));
						}
					}
					if (ptrPos < channel->mNumPositionKeys && (float)(channel->mPositionKeys[ptrPos].mTime) == currentMin)
						ptrPos++;
					if (ptrRot < channel->mNumRotationKeys && (float)(channel->mRotationKeys[ptrRot].mTime) == currentMin)
						ptrRot++;
					if (ptrScale < channel->mNumScalingKeys && (float)(channel->mScalingKeys[ptrScale].mTime) == currentMin)
						ptrScale++;

					if (args.FlipYZ)
					{
						FlipKeyFrame(keyFrame.Transform);
					}
					anim.Channels[i].KeyFrames.Add(keyFrame);
				}
			}
			anim.SaveToFile(Path::ReplaceExt(outFileName, "anim"));
			int maxKeyFrames = 0;
			for (auto & c : anim.Channels)
				maxKeyFrames = Math::Max(maxKeyFrames, c.KeyFrames.Count());
			printf("animation converted. keyframes %d, bones %d\n", maxKeyFrames, anim.Channels.Count());
		}
	}
	else
	{
		printf("cannot load '%S'.\n", fileName.ToWString());
	}
}

class ModelImporterForm : public Form
{
private:
	RefPtr<Button> btnSelectFiles;
	RefPtr<CheckBox> chkFlipYZ, chkFlipUV, chkFlipWinding;
public:
	ModelImporterForm()
	{
		SetText("Convert FBX");
		SetClientWidth(300);
		SetClientHeight(180);
		SetMaximizeBox(false);
		SetBorder(fbFixedDialog);
		chkFlipYZ = new CheckBox(this);
		chkFlipYZ->SetPosition(90, 20, 120, 25);
		chkFlipYZ->SetText("Flip YZ");
		chkFlipYZ->SetChecked(ExportArguments().FlipYZ);
		chkFlipUV = new CheckBox(this);
		chkFlipUV->SetPosition(90, 50, 120, 25);
		chkFlipUV->SetText("Flip UV");
		chkFlipUV->SetChecked(ExportArguments().FlipUV);
		chkFlipWinding = new CheckBox(this);
		chkFlipWinding->SetPosition(90, 80, 120, 25);
		chkFlipWinding->SetText("Flip Winding");
		chkFlipWinding->SetChecked(ExportArguments().FlipWindingOrder);
		btnSelectFiles = new Button(this);
		btnSelectFiles->SetPosition(90, 130, 120, 30);
		btnSelectFiles->SetText("Select Files");
		btnSelectFiles->OnClick.Bind([=](Object*, EventArgs)
		{
			FileDialog dlg(this);
			dlg.MultiSelect = true;
			if (dlg.ShowOpen())
			{
				for (auto file : dlg.FileNames)
				{
					ExportArguments args;
					args.FlipUV = chkFlipUV->GetChecked();
					args.FlipYZ = chkFlipYZ->GetChecked();
					args.FlipWindingOrder = chkFlipWinding->GetChecked();
					args.FileName = file;
					Export(args);
				}
			}
		});
	}
	
};

int wmain(int argc, const wchar_t** argv)
{
	if (argc > 1)
	{
		ExportArguments args;
		args.FileName = String::FromWString(argv[1]);
		Export(args);
	}
	else
	{
		Application::Init();
		Application::Run(new ModelImporterForm());
		Application::Dispose();
	}
	_CrtDumpMemoryLeaks();
	return 0;
}