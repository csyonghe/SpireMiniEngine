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
#include "WinForm/WinTextBox.h"

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
			sb << String(node->mTransformation[i % 4][i / 4]) << " ";
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
	String RootNodeName;
	Quaternion RootTransform = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	Quaternion RootFixTransform = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	bool ExportSkeleton = true;
	bool ExportMesh = true;
	bool ExportAnimation = true;
	bool FlipUV = false;
	bool FlipWindingOrder = false;
	bool FlipYZ = true;
	bool CreateMeshFromSkeleton = true;
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

void GetSkeletonNodes(List<aiNode*> & nodes, aiNode * node)
{
	nodes.Add(node);
	for (auto i = 0u; i < node->mNumChildren; i++)
		GetSkeletonNodes(nodes, node->mChildren[i]);
}

class RootTransformApplier
{
public:
	Quaternion rootTransform, invRootTransform;
	Matrix4 invRootTransformM;
	Matrix4 rootTransformM;

	RootTransformApplier(Quaternion transform)
	{
		rootTransform = transform;
		invRootTransform = rootTransform.Inverse();
		invRootTransformM = invRootTransform.ToMatrix4();
		rootTransformM = rootTransform.ToMatrix4();
	}
	static BoneTransformation ApplyFix(const Matrix4 & transform, const BoneTransformation & bt)
	{
		BoneTransformation rs;
		Matrix4 m;
		Matrix4::Multiply(m, transform, bt.ToMatrix());
		rs.FromMatrix(m);

		float X0, Y0, Z0, X1, Y1, Z1;
		QuaternionToEulerAngle(bt.Rotation, X0, Y0, Z0, EulerAngleOrder::YZX);
		QuaternionToEulerAngle(rs.Rotation, X1, Y1, Z1, EulerAngleOrder::YZX);
		return rs;
	}
	BoneTransformation Apply(const BoneTransformation & bt)
	{
		BoneTransformation rs;
		Matrix4 T;
		Matrix4::Translation(T, bt.Translation.x, bt.Translation.y, bt.Translation.z);
		Matrix4 m1;
		Matrix4::Multiply(m1, T, invRootTransformM);
		Matrix4::Multiply(m1, rootTransformM, m1);
		rs.Translation = Vec3::Create(m1.values[12], m1.values[13], m1.values[14]);
		rs.Rotation = rootTransform * bt.Rotation * invRootTransform;
		Matrix4 S;
		Matrix4::Scale(S, bt.Scale.x, bt.Scale.y, bt.Scale.z);
		Matrix4::Multiply(m1, S, invRootTransformM);
		Matrix4::Multiply(m1, rootTransformM, m1);
		rs.Scale = Vec3::Create(m1.values[0], m1.values[5], m1.values[10]);
		float X0, Y0, Z0, X1, Y1, Z1;
		QuaternionToEulerAngle(bt.Rotation, X0, Y0, Z0, EulerAngleOrder::YZX);
		QuaternionToEulerAngle(rs.Rotation, X1, Y1, Z1, EulerAngleOrder::YZX);
		Vec3 norm = rs.Rotation.Transform(Vec3::Create(1.0f, 0.f, 0.f));

		return rs;
	}
};

void Export(ExportArguments args)
{
	if (args.FlipYZ)
	{
		Quaternion flipYZTransform = Quaternion::FromAxisAngle(Vec3::Create(1.0f, 0.0f, 0.0f), Math::Pi * 0.5f);
		args.RootTransform = flipYZTransform * args.RootTransform;
	}

	RootTransformApplier rootTransformApplier(args.RootTransform);
	Matrix4 rootFixTransform = args.RootFixTransform.ToMatrix4();

	auto fileName = args.FileName;
	auto outFileName = Path::ReplaceExt(fileName, "out");
	printf("loading %S...\n", fileName.ToWString());
	Assimp::Importer importer;
	int process = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_SortByPType;
	if (args.FlipUV)
		process |= aiProcess_FlipUVs;
	if (args.FlipWindingOrder)
		process |= aiProcess_FlipWindingOrder;
	auto scene = importer.ReadFile(fileName.Buffer(), process);
	if (scene)
	{
		importer.ApplyPostProcessing(aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality | aiProcess_LimitBoneWeights);
		wprintf(L"input file contains %d meshes, %d animations.\n", scene->mNumMeshes, scene->mNumAnimations);
		if ((args.ExportSkeleton || args.ExportMesh) && scene->mNumMeshes == 0)
		{
			wprintf(L"waning: input file does not contain any meshes.\n");
		}
		Skeleton skeleton;
		if (args.ExportSkeleton || args.ExportAnimation)
		{
			// find root bone
			if (args.RootNodeName.Length() == 0)
			{
				bool rootBoneFound = false;
				// find root bone
				if (scene->HasMeshes())
				{
					auto mesh = scene->mMeshes[0];
					if (mesh->HasBones())
					{
						auto firstBoneNode = scene->mRootNode->FindNode(mesh->mBones[0]->mName);
						auto rootBoneNode = firstBoneNode;
						while (rootBoneNode->mParent && rootBoneNode->mParent != scene->mRootNode)
							rootBoneNode = rootBoneNode->mParent;
						printf("root node name not specified, using bone '%s'.\n", rootBoneNode->mName.C_Str());
						args.RootNodeName = rootBoneNode->mName.C_Str();
						rootBoneFound = true;
					}
				}
				if (!rootBoneFound)
				{
					printf("error: root bone not specified.\n");
					goto endOfSkeletonExport;
				}
			}
			auto firstBoneNode = scene->mRootNode->FindNode(args.RootNodeName.Buffer());
			if (!firstBoneNode)
			{
				printf("error: root node named '%s' not found.\n", args.RootNodeName.Buffer());
				goto endOfSkeletonExport;
			}
			auto rootBoneNode = firstBoneNode;
			while (rootBoneNode->mParent && rootBoneNode->mParent != scene->mRootNode)
				rootBoneNode = rootBoneNode->mParent;
			printf("root bone is \'%s\'\n", rootBoneNode->mName.C_Str());
			List<aiNode*> nodes;
			GetSkeletonNodes(nodes, rootBoneNode);
			skeleton.Bones.SetSize(nodes.Count());
			skeleton.InversePose.SetSize(skeleton.Bones.Count());
			for (auto i = 0; i < nodes.Count(); i++)
			{
				skeleton.Bones[i].Name = nodes[i]->mName.C_Str();
				skeleton.Bones[i].ParentId = -1;
				skeleton.BoneMapping[skeleton.Bones[i].Name] = i;

				aiVector3D scale, pos;
				aiQuaternion rot;
				nodes[i]->mTransformation.Decompose(scale, rot, pos);
				skeleton.Bones[i].BindPose.Rotation = ToQuaternion(rot);
				skeleton.Bones[i].BindPose.Scale = Vec3::Create(scale.x, scale.y, scale.z);
				skeleton.Bones[i].BindPose.Translation = Vec3::Create(pos.x, pos.y, pos.z);
			}
			for (auto i = 0; i < nodes.Count(); i++)
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

			// apply root fix transform
			skeleton.Bones[0].BindPose = RootTransformApplier::ApplyFix(rootFixTransform, skeleton.Bones[0].BindPose);

			// apply root transform
			for (auto & bone : skeleton.Bones)
				bone.BindPose = rootTransformApplier.Apply(bone.BindPose);

			// compute inverse matrices
			for (auto i = 0; i < nodes.Count(); i++)
				skeleton.InversePose[i] = skeleton.Bones[i].BindPose.ToMatrix();
			for (auto i = 0; i < nodes.Count(); i++)
			{
				if (skeleton.Bones[i].ParentId != -1)
					Matrix4::Multiply(skeleton.InversePose[i], skeleton.InversePose[skeleton.Bones[i].ParentId], skeleton.InversePose[i]);
			}
			for (auto i = 0; i < nodes.Count(); i++)
				skeleton.InversePose[i].Inverse(skeleton.InversePose[i]);
			if (args.ExportSkeleton)
			{
				skeleton.SaveToFile(Path::ReplaceExt(outFileName, "skeleton"));
				printf("skeleton converted. total bones: %d.\n", skeleton.Bones.Count());
				if (args.CreateMeshFromSkeleton)
				{
					Mesh m;
					m.FromSkeleton(&skeleton, 5.0f);
					m.SaveToFile(Path::ReplaceExt(outFileName, "skeleton.mesh"));
				}
			}
		}

	endOfSkeletonExport:
		if (args.ExportMesh && scene->mNumMeshes > 0)
		{
			int numColorChannels = 0;
			int numUVChannels = 0;
			int numVerts = 0;
			int numFaces = 0;
			bool hasBones = false;
			
			for (auto cid = 0u; cid < scene->mRootNode->mNumChildren; cid++)
			{
				if (scene->mRootNode->mChildren[cid]->mNumMeshes != 0)
				{
					for (auto mid = 0u; mid < scene->mRootNode->mChildren[cid]->mNumMeshes; mid++)
					{
						auto mesh = scene->mMeshes[scene->mRootNode->mChildren[cid]->mMeshes[mid]];
						numColorChannels = Math::Max(numColorChannels, (int)mesh->GetNumColorChannels());
						numUVChannels = Math::Max(numUVChannels, (int)mesh->GetNumUVChannels());
						hasBones |= mesh->HasBones();
						numVerts += mesh->mNumVertices;
						for (auto i = 0u; i < mesh->mNumFaces; i++)
						{
							if (mesh->mFaces[i].mNumIndices == 3)
								numFaces++;
						}
					}
				}
			}
			RefPtr<Mesh> meshOut = new Mesh();
			meshOut->Bounds.Init();
			meshOut->SetVertexFormat(MeshVertexFormat(numColorChannels, numUVChannels, true, hasBones));
			meshOut->AllocVertexBuffer(numVerts);
			meshOut->Indices.SetSize(numFaces * 3);
			int vertPtr = 0, idxPtr = 0;
			for (auto cid = 0u; cid < scene->mRootNode->mNumChildren; cid++)
			{
				if (scene->mRootNode->mChildren[cid]->mNumMeshes != 0)
				{
					aiMatrix4x4 transform = scene->mRootNode->mChildren[cid]->mTransformation;
					transform.Transpose();
					Matrix4 transformMat;
					memcpy(&transformMat, &transform, sizeof(Matrix4));
					Matrix4::Multiply(transformMat, args.RootTransform.ToMatrix4(), transformMat);
					Matrix4 normMat;
					transformMat.GetNormalMatrix(normMat);
					for (auto mid = 0u; mid < scene->mRootNode->mChildren[cid]->mNumMeshes; mid++)
					{
						auto mesh = scene->mMeshes[scene->mRootNode->mChildren[cid]->mMeshes[mid]];
						MeshElementRange elementRange;
						elementRange.StartIndex = idxPtr;
						elementRange.Count = mesh->mNumFaces * 3;
						meshOut->ElementRanges.Add(elementRange);
						int startVertId = vertPtr;
						vertPtr += mesh->mNumVertices;
						int faceId = 0;
						for (auto i = 0u; i < mesh->mNumFaces; i++)
						{
							if (mesh->mFaces[i].mNumIndices != 3)
								continue;
							meshOut->Indices[elementRange.StartIndex + faceId * 3] = startVertId + mesh->mFaces[i].mIndices[0];
							meshOut->Indices[elementRange.StartIndex + faceId * 3 + 1] = startVertId + mesh->mFaces[i].mIndices[1];
							meshOut->Indices[elementRange.StartIndex + faceId * 3 + 2] = startVertId + mesh->mFaces[i].mIndices[2];
							faceId++;
						}
						idxPtr += faceId * 3;
						for (auto i = 0u; i < mesh->mNumVertices; i++)
						{
							auto vertPos = Vec3::Create(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
							vertPos = transformMat.TransformHomogeneous(vertPos);

							meshOut->Bounds.Union(vertPos);
							meshOut->SetVertexPosition(startVertId + i, vertPos);
							for (auto j = 0u; j < mesh->GetNumUVChannels(); j++)
							{
								auto uv = Vec2::Create(mesh->mTextureCoords[j][i].x, mesh->mTextureCoords[j][i].y);
								meshOut->SetVertexUV(startVertId + i, j, uv);
							}
							for (auto j = 0u; j < mesh->GetNumColorChannels(); j++)
							{
								auto color = Vec4::Create(mesh->mColors[j][i].r, mesh->mColors[j][i].g, mesh->mColors[j][i].b, mesh->mColors[j][i].a);
								meshOut->SetVertexColor(startVertId + i, j, color);
							}
							Vec3 normal = Vec3::Create(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

							normal = normMat.TransformNormal(normal);
							Vec3 tangent;
							if (mesh->HasTangentsAndBitangents())
							{
								tangent = Vec3::Create(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
								tangent = transformMat.TransformNormal(tangent);
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
							meshOut->SetVertexTangentFrame(startVertId + i, vq);
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
								if (!skeleton.BoneMapping.ContainsKey(bone->mName.C_Str()))
									continue;
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
									else
										printf("Error: too many bones affecting vertex %d.\n", vertId);
								}
							}
							for (auto i = 0u; i < mesh->mNumVertices; i++)
							{
								if (boneIds[i][0] == -1 || boneWeights[i][0] < 0.3f)
									printf("vertex %d is not affected by any bones.\n", i);
								// normalize bone weights
								float sumWeight = 0.0f;
								for (int j = 0; j < boneIds[i].Count(); j++)
									if (boneIds[i][j] != -1)
										sumWeight += boneWeights[i][j];
								if (sumWeight > 0.0f)
								{
									sumWeight = 1.0f / sumWeight;
									for (int j = 0; j < boneWeights[i].Count(); j++)
										boneWeights[i][j] *= sumWeight;
								}
								meshOut->SetVertexSkinningBinding(startVertId + i, boneIds[i].GetArrayView(), boneWeights[i].GetArrayView());
							}
						}
						else if (hasBones)
						{
							Array<int, 8> boneIds;
							Array<float, 8> boneWeights;
							boneIds.SetSize(4);
							boneWeights.SetSize(4);
							boneIds[0] = 0;
							for (int i = 1; i < boneIds.GetCapacity(); i++)
								boneIds[i] = 255;
							boneWeights[0] = 1.0f;
							for (int i = 1; i < boneIds.GetCapacity(); i++)
								boneWeights[i] = 0.0f;
							for (auto i = 0u; i < mesh->mNumVertices; i++)
								meshOut->SetVertexSkinningBinding(startVertId + i, boneIds.GetArrayView(), boneWeights.GetArrayView());
						}
					}
				}
			}
			meshOut->SaveToFile(Path::ReplaceExt(outFileName, "mesh"));
			wprintf(L"mesh converted: elements %d, faces: %d, vertices: %d, skeletal: %s.\n", meshOut->ElementRanges.Count(), meshOut->Indices.Count()/3, meshOut->GetVertexCount(),
				hasBones?L"true":L"false");
		}

		if (args.ExportAnimation && scene->mAnimations > 0)
		{
			bool inconsistentKeyFrames = false;
			SkeletalAnimation anim;
			auto animIn = scene->mAnimations[0];
			anim.Speed = (float)animIn->mTicksPerSecond;
			memset(anim.Reserved, 0, sizeof(anim.Reserved));
			anim.Name = animIn->mName.C_Str();
			anim.Duration = (float)(animIn->mDuration);
			anim.Channels.SetSize(animIn->mNumChannels);
			for (auto i = 0u; i < animIn->mNumChannels; i++)
			{
				auto channel = animIn->mChannels[i];
				anim.Channels[i].BoneName = animIn->mChannels[i]->mNodeName.C_Str();
				bool isRoot = anim.Channels[i].BoneName == args.RootNodeName;
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
					// apply root fix transform
					keyFrame.Transform = RootTransformApplier::ApplyFix(rootFixTransform, keyFrame.Transform);
					keyFrame.Transform = rootTransformApplier.Apply(keyFrame.Transform);
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

/*
What is RootFixTransform?

When applied to animation and skeleton, RootFixTransform "cancels" the specified a wrong coordinate system transformation
of the input data, by applying the specified transform to the root node.
*/

class ModelImporterForm : public Form
{
private:
	RefPtr<Button> btnSelectFiles;
	RefPtr<CheckBox> chkFlipYZ, chkFlipUV, chkFlipWinding, chkCreateSkeletonMesh;
	RefPtr<TextBox> txtRootTransform, txtRootFixTransform, txtRootBoneName;
	RefPtr<Label> lblRootTransform, lblRootFixTransform, lblRootBoneName;
	Quaternion ParseRootTransform(String txt)
	{
		if (txt.Trim() == "")
			return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		Quaternion rs(0.0f, 0.0f, 0.0f, 1.0f);
		Text::TokenReader parser(txt);
		try
		{
			while (!parser.IsEnd())
			{
				if (parser.LookAhead("rotx"))
				{
					parser.ReadToken();
					rs = rs * Quaternion::FromAxisAngle(Vec3::Create(1.0f, 0.0f, 0.0f), parser.ReadFloat() / 180.0f * Math::Pi);
				}
				else if (parser.LookAhead("roty"))
				{
					parser.ReadToken();
					rs = rs * Quaternion::FromAxisAngle(Vec3::Create(0.0f, 1.0f, 0.0f), parser.ReadFloat() / 180.0f * Math::Pi);
				}
				else if (parser.LookAhead("rotz"))
				{
					parser.ReadToken();
					rs = rs * Quaternion::FromAxisAngle(Vec3::Create(0.0f, 0.0f, 1.0f), parser.ReadFloat() / 180.0f * Math::Pi);
				}
				else
					throw Text::TextFormatException("");
			}

		}
		catch (const Exception &)
		{
			MessageBox("Invalid root transform.", "Error", MB_ICONEXCLAMATION);
			return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		}
		return rs;
	}
public:
	ModelImporterForm()
	{
		SetText("Convert FBX");
		SetClientWidth(400);
		SetClientHeight(300);
		SetMaximizeBox(false);
		SetBorder(fbFixedDialog);
		chkFlipYZ = new CheckBox(this);
		chkFlipYZ->SetPosition(90, 20, 50, 25);
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
		chkCreateSkeletonMesh = new CheckBox(this);
		chkCreateSkeletonMesh->SetPosition(90, 110, 180, 25);
		chkCreateSkeletonMesh->SetText("Create Mesh From Skeleton");
		chkCreateSkeletonMesh->SetChecked(ExportArguments().CreateMeshFromSkeleton);
		txtRootBoneName = new TextBox(this);
		txtRootBoneName->SetText("");
		txtRootBoneName->SetPosition(200, 140, 100, 25);
		lblRootBoneName = new Label(this);
		lblRootBoneName->SetText("Root Name:");
		lblRootBoneName->SetPosition(90, 145, 100, 25);

		txtRootTransform = new TextBox(this);
		txtRootTransform->SetPosition(200, 170, 100, 25);
		txtRootTransform->SetText("");
		lblRootTransform = new Label(this);
		lblRootTransform->SetText("Root Transform: ");
		lblRootTransform->SetPosition(90, 175, 100, 25);

		txtRootFixTransform = new TextBox(this);
		txtRootFixTransform->SetPosition(200, 200, 100, 25);
		txtRootFixTransform->SetText("");
		lblRootFixTransform = new Label(this);
		lblRootFixTransform->SetText("Root Fix:");
		lblRootFixTransform->SetPosition(90, 205, 100, 25);

		btnSelectFiles = new Button(this);
		btnSelectFiles->SetPosition(90, 240, 120, 30);
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
					args.RootNodeName = txtRootBoneName->GetText();
					args.RootTransform = ParseRootTransform(txtRootTransform->GetText());
					args.RootFixTransform = ParseRootTransform(txtRootFixTransform->GetText());

					args.CreateMeshFromSkeleton = chkCreateSkeletonMesh->GetChecked();
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