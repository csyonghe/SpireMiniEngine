//#include "FBXImport/include/Importer.hpp"
//#include "FBXImport/include/scene.h"
//#include "FBXImport/include/postprocess.h"
#include "CoreLib/Basic.h"
#include "CoreLib/LibIO.h"
#include "Skeleton.h"
#include "Mesh.h"
#include "WinForm/WinButtons.h"
#include "WinForm/WinCommonDlg.h"
#include "WinForm/WinForm.h"
#include "WinForm/WinApp.h"
#include "WinForm/WinTextBox.h"
#include "FBX_SDK/include/fbxsdk.h"

using namespace CoreLib;
using namespace CoreLib::IO;
using namespace GameEngine;
using namespace VectorMath;

Quaternion ToQuaternion(const FbxVector4 & q)
{
	Quaternion rs;
	rs.x = (float)q[0];
	rs.y = (float)q[1];
	rs.z = (float)q[2];
	rs.w = (float)q[3];
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
//
//void ConvertLevelNode(StringBuilder & sb, const aiScene * scene, aiNode * node)
//{
//	for (auto i = 0u; i < node->mNumMeshes; i++)
//	{
//		sb << "StaticMesh\n{\n";
//		sb << "name \"" << String(node->mName.C_Str()) << "\"\n";
//		sb << "transform [";
//		for (int i = 0; i < 16; i++)
//			sb << String(node->mTransformation[i % 4][i / 4]) << " ";
//		sb << "]\n";
//		auto name = String(scene->mMeshes[node->mMeshes[i]]->mName.C_Str());
//		if (name.Length() == 0)
//			name = "mesh_" + String((int)(node->mMeshes[i])) + ".mesh";
//		sb << "mesh \"" << name << "\"\n";
//		for (auto i = 0u; i < node->mNumChildren; i++)
//		{
//			ConvertLevelNode(sb, scene, node->mChildren[i]);
//		}
//		sb << "}\n";
//	}
//}

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
	bool FlipYZ = false;
	bool CreateMeshFromSkeleton = false;
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

void GetSkeletonNodes(List<FbxNode*> & nodes, FbxNode * node)
{
	nodes.Add(node);
	for (auto i = 0; i < node->GetChildCount(); i++)
		GetSkeletonNodes(nodes, node->GetChild(i));
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

void PrintNode(FbxNode* pNode, int numTabs = 0)
{
	for (int i = 0; i < numTabs; i++) printf("  ");
	const char* nodeName = pNode->GetName();
	FbxDouble3 translation = pNode->LclTranslation.Get();
	FbxDouble3 rotation = pNode->LclRotation.Get();
	FbxDouble3 scaling = pNode->LclScaling.Get();
	
	// Print the contents of the node.
	printf("<node type='%s' name='%s' translation='(%f, %f, %f)' rotation='(%f, %f, %f)' scaling='(%f, %f, %f)'>\n",
		pNode->GetTypeName(), nodeName,
		translation[0], translation[1], translation[2],
		rotation[0], rotation[1], rotation[2],
		scaling[0], scaling[1], scaling[2]
	);

	// Recursively print the children.
	for (int j = 0; j < pNode->GetChildCount(); j++)
		PrintNode(pNode->GetChild(j), numTabs + 1);
	for (int i = 0; i < numTabs; i++) printf("  ");
	printf("</node>\n");
}

Matrix4 GetMatrix(FbxAMatrix m)
{
	Matrix4 rs;
	for (int i = 0; i < 16; i++)
		rs.values[i] = (float)m.Get(i >> 2, i & 3);
	return rs;
}

Vec2 GetVec2(FbxVector2 v)
{
	Vec2 rs;
	rs.x = (float)v[0];
	rs.y = (float)v[1];
	return rs;
}

Vec4 GetVec4(FbxColor v)
{
	Vec4 rs;
	rs.x = (float)v.mRed;
	rs.y = (float)v.mGreen;
	rs.z = (float)v.mBlue;
	rs.w = (float)v.mAlpha;
	return rs;
}

Vec3 GetVec3(FbxVector4 v)
{
	Vec3 rs;
	rs.x = (float)v[0];
	rs.y = (float)v[1];
	rs.z = (float)v[2];
	return rs;
}

FbxNode * FindFirstSkeletonNode(FbxNode * node)
{
	if (node->GetSkeleton())
		return node;
	for (int i = 0; i < node->GetChildCount(); i++)
	{
		auto rs = FindFirstSkeletonNode(node->GetChild(i));
		if (rs)
			return rs;
	}
	return nullptr;
}

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
	
	FbxManager * lSdkManager = FbxManager::Create();
	FbxIOSettings *ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);

	// Create an importer using the SDK manager.
	::FbxImporter* lImporter = ::FbxImporter::Create(lSdkManager, "");

	// Use the first argument as the filename for the importer.
	if (!lImporter->Initialize(fileName.Buffer(), -1, lSdkManager->GetIOSettings()))
	{
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		return;
	}
	// Create a new scene so that it can be populated by the imported file.
	FbxScene* lScene = FbxScene::Create(lSdkManager, "myScene");

	// Import the contents of the file into the scene.
	lImporter->Import(lScene);

	// The file is imported, so get rid of the importer.
	lImporter->Destroy();

	// Print the nodes of the scene and their attributes recursively.
	// Note that we are not printing the root node because it should
	// not contain any attributes.
	FbxNode* lRootNode = lScene->GetRootNode();

	Skeleton skeleton;
	List<fbxsdk::FbxNode*> skeletonNodes;
	if (args.ExportSkeleton || args.ExportAnimation)
	{
		// find root bone
		if (args.RootNodeName.Length() == 0)
		{
			bool rootBoneFound = false;
			// find root bone
			auto skeletonRootNode = FindFirstSkeletonNode(lRootNode);
			if (skeletonRootNode)
				args.RootNodeName = skeletonRootNode->GetName();
			else
			{
				printf("error: root bone not specified.\n");
				goto endOfSkeletonExport;
			}
		}
		auto firstBoneNode = lRootNode->FindChild(args.RootNodeName.Buffer());
		if (!firstBoneNode)
		{
			printf("error: root node named '%s' not found.\n", args.RootNodeName.Buffer());
			goto endOfSkeletonExport;
		}
		auto rootBoneNode = firstBoneNode;
		printf("root bone is \'%s\'\n", rootBoneNode->GetName());
		
		GetSkeletonNodes(skeletonNodes, rootBoneNode);
		skeleton.Bones.SetSize(skeletonNodes.Count());
		skeleton.InversePose.SetSize(skeleton.Bones.Count());
		for (auto i = 0; i < skeletonNodes.Count(); i++)
		{
			skeleton.Bones[i].Name = skeletonNodes[i]->GetName();
			skeleton.Bones[i].ParentId = -1;
			skeleton.BoneMapping[skeleton.Bones[i].Name] = i;
			auto trans = GetMatrix(skeletonNodes[i]->EvaluateLocalTransform());
			skeleton.Bones[i].BindPose.FromMatrix(trans);
		}
		for (auto i = 0; i < skeletonNodes.Count(); i++)
		{
			if (skeletonNodes[i])
			{
				for (auto j = 0; j < skeletonNodes[i]->GetChildCount(); j++)
				{
					int boneId = -1;
					if (skeleton.BoneMapping.TryGetValue(skeletonNodes[i]->GetChild(j)->GetName(), boneId))
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
		for (auto i = 0; i < skeletonNodes.Count(); i++)
			skeleton.InversePose[i] = skeleton.Bones[i].BindPose.ToMatrix();
		for (auto i = 0; i < skeletonNodes.Count(); i++)
		{
			if (skeleton.Bones[i].ParentId != -1)
				Matrix4::Multiply(skeleton.InversePose[i], skeleton.InversePose[skeleton.Bones[i].ParentId], skeleton.InversePose[i]);
		}
		for (auto i = 0; i < skeletonNodes.Count(); i++)
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
	if (args.ExportMesh)
	{
		int numColorChannels = 0;
		int numUVChannels = 0;
		int numVerts = 0;
		int numFaces = 0;
		bool hasBones = false;

		for (auto cid = 0; cid < lRootNode->GetChildCount(); cid++)
		{
			if (auto mesh = lRootNode->GetChild(cid)->GetMesh())
			{
				numColorChannels = Math::Max(numColorChannels, (int)mesh->GetElementVertexColorCount());
				numUVChannels = Math::Max(numUVChannels, (int)mesh->GetElementUVCount());
				int deformerCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
				hasBones |= (deformerCount != 0);
				for (auto i = 0; i < mesh->GetPolygonCount(); i++)
				{
					numVerts += mesh->GetPolygonSize(i);
					if (mesh->GetPolygonSize(i) >= 3)
						numFaces += mesh->GetPolygonSize(i) - 2;
				}
			}
		}
		RefPtr<Mesh> meshOut = new Mesh();
		meshOut->Bounds.Init();
		meshOut->SetVertexFormat(MeshVertexFormat(numColorChannels, numUVChannels, true, hasBones));
		meshOut->AllocVertexBuffer(numVerts);
		meshOut->Indices.SetSize(numFaces * 3);
		int vertPtr = 0, idxPtr = 0;
		for (auto cid = 0; cid < lRootNode->GetChildCount(); cid++)
		{
			auto transformMat = GetMatrix(lRootNode->GetChild(cid)->EvaluateGlobalTransform());
			if (auto mesh = lRootNode->GetChild(cid)->GetMesh())
			{
				Matrix4::Multiply(transformMat, args.RootTransform.ToMatrix4(), transformMat);
				Matrix4 normMat;
				transformMat.GetNormalMatrix(normMat);
				printf("Mesh element %d: %s\n", meshOut->ElementRanges.Count(), lRootNode->GetChild(cid)->GetName());
				MeshElementRange elementRange;
				elementRange.StartIndex = idxPtr;
				int meshNumFaces = 0;
				int meshNumVerts = 0;
				for (auto i = 0; i < mesh->GetPolygonCount(); i++)
				{
					if (mesh->GetPolygonSize(i) >= 3)
						meshNumFaces += mesh->GetPolygonSize(i) - 2;
					meshNumVerts += mesh->GetPolygonSize(i);
				}
				elementRange.Count = meshNumFaces * 3;
				meshOut->ElementRanges.Add(elementRange);
				int startVertId = vertPtr;
				vertPtr += meshNumVerts;
				int faceId = 0;
				auto srcIndices = mesh->GetPolygonVertices();
				auto srcVerts = mesh->GetControlPoints();
				mesh->GenerateNormals();
				mesh->GenerateTangentsData(0);
				int mvptr = startVertId;
				int vertexId = 0;
				List<int> vertexIdToControlPointIndex;
				vertexIdToControlPointIndex.SetSize(meshNumVerts);
				for (auto i = 0; i < mesh->GetPolygonCount(); i++)
				{
					if (mesh->GetPolygonSize(i) < 3)
					{
						vertexId += mesh->GetPolygonSize(i);
						continue;
					}
					auto srcPolygonIndices = srcIndices + mesh->GetPolygonVertexIndex(i);
					for (auto j = 0; j < mesh->GetPolygonSize(i); j++)
					{
						int controlPointIndex = srcPolygonIndices[j];
						auto srcVert = srcVerts[srcPolygonIndices[j]];
						auto vertPos = Vec3::Create((float)srcVert[0], (float)srcVert[1], (float)srcVert[2]);
						vertPos = transformMat.TransformHomogeneous(vertPos);
						meshOut->Bounds.Union(vertPos);
						meshOut->SetVertexPosition(mvptr + j, vertPos);
						vertexIdToControlPointIndex[vertexId] = controlPointIndex;
						for (auto k = 0; k < mesh->GetElementUVCount(); k++)
						{
							FbxGeometryElementUV* leUV = mesh->GetElementUV(k);
							Vec2 vertUV;
							switch (leUV->GetMappingMode())
							{
							default:
								break;
							case FbxGeometryElement::eByControlPoint:
								switch (leUV->GetReferenceMode())
								{
								case FbxGeometryElement::eDirect:
									vertUV = GetVec2(leUV->GetDirectArray().GetAt(controlPointIndex));
									break;
								case FbxGeometryElement::eIndexToDirect:
								{
									int id = leUV->GetIndexArray().GetAt(controlPointIndex);
									vertUV = GetVec2(leUV->GetDirectArray().GetAt(id));
								}
								break;
								default:
									break; // other reference modes not shown here!
								}
								break;

							case FbxGeometryElement::eByPolygonVertex:
							{
								int lTextureUVIndex = mesh->GetTextureUVIndex(i, j);
								switch (leUV->GetReferenceMode())
								{
								case FbxGeometryElement::eDirect:
								case FbxGeometryElement::eIndexToDirect:
								{
									vertUV = GetVec2(leUV->GetDirectArray().GetAt(lTextureUVIndex));
								}
								break;
								default:
									break; // other reference modes not shown here!
								}
							}
							break;

							case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
							case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
							case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
								break;
							}
							meshOut->SetVertexUV(mvptr + j, k, vertUV);
						}

						for (auto k = 0; k < mesh->GetElementVertexColorCount(); k++)
						{
							FbxGeometryElementVertexColor* leVtxc = mesh->GetElementVertexColor(k);
							Vec4 vertColor;
							vertColor.SetZero();
							switch (leVtxc->GetMappingMode())
							{
							default:
								break;
							case FbxGeometryElement::eByControlPoint:
								switch (leVtxc->GetReferenceMode())
								{
								case FbxGeometryElement::eDirect:
									vertColor = GetVec4(leVtxc->GetDirectArray().GetAt(controlPointIndex));
									break;
								case FbxGeometryElement::eIndexToDirect:
								{
									int id = leVtxc->GetIndexArray().GetAt(controlPointIndex);
									vertColor = GetVec4(leVtxc->GetDirectArray().GetAt(id));
								}
								break;
								default:
									break; // other reference modes not shown here!
								}
								break;

							case FbxGeometryElement::eByPolygonVertex:
							{
								switch (leVtxc->GetReferenceMode())
								{
								case FbxGeometryElement::eDirect:
									vertColor = GetVec4(leVtxc->GetDirectArray().GetAt(vertexId));
									break;
								case FbxGeometryElement::eIndexToDirect:
								{
									int id = leVtxc->GetIndexArray().GetAt(vertexId);
									vertColor = GetVec4(leVtxc->GetDirectArray().GetAt(id));
								}
								break;
								default:
									break; // other reference modes not shown here!
								}
							}
							break;

							case FbxGeometryElement::eByPolygon: // doesn't make much sense for UVs
							case FbxGeometryElement::eAllSame:   // doesn't make much sense for UVs
							case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
								break;
							}
							meshOut->SetVertexColor(mvptr + j, k, vertColor);
						}

						Vec3 vertNormal, vertTangent, vertBitangent;
						vertNormal.SetZero();
						vertTangent.SetZero();
						vertBitangent.SetZero();
						if (mesh->GetElementNormalCount() > 0)
						{
							FbxGeometryElementNormal* leNormal = mesh->GetElementNormal(0);
							if (leNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
							{
								switch (leNormal->GetReferenceMode())
								{
								case FbxGeometryElement::eDirect:
									vertNormal = GetVec3(leNormal->GetDirectArray().GetAt(vertexId));
									break;
								case FbxGeometryElement::eIndexToDirect:
								{
									int id = leNormal->GetIndexArray().GetAt(vertexId);
									vertNormal = GetVec3(leNormal->GetDirectArray().GetAt(id));
								}
								break;
								default:
									break; // other reference modes not shown here!
								}
							}
						}

						for (int l = 0; l < mesh->GetElementTangentCount(); ++l)
						{
							FbxGeometryElementTangent* leTangent = mesh->GetElementTangent(l);

							if (leTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
							{
								switch (leTangent->GetReferenceMode())
								{
								case FbxGeometryElement::eDirect:
									vertTangent = GetVec3(leTangent->GetDirectArray().GetAt(vertexId));
									break;
								case FbxGeometryElement::eIndexToDirect:
								{
									int id = leTangent->GetIndexArray().GetAt(vertexId);
									vertTangent = GetVec3(leTangent->GetDirectArray().GetAt(id));
								}
								break;
								default:
									break; // other reference modes not shown here!
								}
							}

						}
						for (int l = 0; l < mesh->GetElementBinormalCount(); ++l)
						{
							FbxGeometryElementBinormal* leBinormal = mesh->GetElementBinormal(l);

							if (leBinormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
							{
								switch (leBinormal->GetReferenceMode())
								{
								case FbxGeometryElement::eDirect:
									vertBitangent = GetVec3(leBinormal->GetDirectArray().GetAt(vertexId));
									break;
								case FbxGeometryElement::eIndexToDirect:
								{
									int id = leBinormal->GetIndexArray().GetAt(vertexId);
									vertBitangent = GetVec3(leBinormal->GetDirectArray().GetAt(id));
								}
								break;
								default:
									break; // other reference modes not shown here!
								}
							}
						}
						vertTangent = transformMat.TransformNormal(vertTangent);
						vertBitangent = transformMat.TransformNormal(vertBitangent);
						vertNormal = normMat.TransformNormal(vertNormal);
						Vec3 refBitangent = Vec3::Cross(vertTangent, vertNormal);
						Quaternion q = Quaternion::FromCoordinates(vertTangent, vertNormal, refBitangent);
						auto vq = q.ToVec4().Normalize();
						if (vq.w < 0)
							vq = -vq;
						if (Vec3::Dot(refBitangent, vertTangent) < 0.0f)
							vq = -vq;
						meshOut->SetVertexTangentFrame(mvptr + j, vq);
						vertexId++;
					}
					for (auto j = 1; j < mesh->GetPolygonSize(i) - 1; j++)
					{
						meshOut->Indices[elementRange.StartIndex + faceId * 3] = mvptr;
						meshOut->Indices[elementRange.StartIndex + faceId * 3 + 1] = mvptr + j;
						meshOut->Indices[elementRange.StartIndex + faceId * 3 + 2] = mvptr + j + 1;
						faceId++;
					}
					mvptr += mesh->GetPolygonSize(i);
				}
				idxPtr += faceId * 3;
				int deformCount = mesh->GetDeformerCount(fbxsdk::FbxDeformer::eSkin);
				if (deformCount)
				{
					struct IdWeight
					{
						int boneId;
						float weight;
					};
					List<List<IdWeight>> cpWeights;
					cpWeights.SetSize(mesh->GetControlPointsCount());
					for (int i = 0; i < deformCount; i++)
					{
						auto deformer = mesh->GetDeformer(i, FbxDeformer::eSkin);
						auto skin = (::FbxSkin*)(deformer);
						for (int j = 0; j < skin->GetClusterCount(); j++)
						{
							auto boneNode = skin->GetCluster(j)->GetLink();
							int boneId = -1;
							for (int k = 0; k < skeletonNodes.Count(); k++)
								if (skeletonNodes[k] == boneNode)
								{
									boneId = k;
									break;
								}
							auto cpIndices = skin->GetCluster(j)->GetControlPointIndices();
							auto weights = skin->GetCluster(j)->GetControlPointWeights();
							for (int k = 0; k < skin->GetCluster(j)->GetControlPointIndicesCount(); k++)
							{
								IdWeight p;
								p.boneId = boneId;
								p.weight = (float)weights[k];
								cpWeights[cpIndices[k]].Add(p);
							}
						}
					}
					// trim bone weights
					for (auto & weights : cpWeights)
					{
						weights.Sort([](IdWeight w0, IdWeight w1) {return w0.weight > w1.weight; });

						if (weights.Count() > 4)
						{
							weights.SetSize(4);
						}
						float sumWeight = 0.0f;
						for (int k = 0; k < weights.Count(); k++)
							sumWeight += weights[k].weight;
						sumWeight = 1.0f / sumWeight;
						for (int k = 0; k < weights.Count(); k++)
							weights[k].weight *= sumWeight;
					}
					
					vertexId = 0;
					for (auto i = 0; i < mesh->GetPolygonCount(); i++)
					{
						if (mesh->GetPolygonSize(i) < 3)
						{
							vertexId += mesh->GetPolygonSize(i);
							continue;
						}
						auto srcPolygonIndices = srcIndices + mesh->GetPolygonVertexIndex(i);
						for (auto j = 0; j < mesh->GetPolygonSize(i); j++)
						{
							int cpId = srcPolygonIndices[j];
							Array<int, 8> boneIds;
							Array<float, 8> boneWeights;
							boneIds.SetSize(4);
							boneWeights.SetSize(4);
							boneIds[0] = boneIds[1] = boneIds[2] = boneIds[3] = -1;
							boneWeights[0] = boneWeights[1] = boneWeights[2] = boneWeights[3] = 0.0f;
							for (int k = 0; k < cpWeights[cpId].Count(); k++)
							{
								boneIds[k] = (cpWeights[cpId][k].boneId);
								boneWeights[k] = (cpWeights[cpId][k].weight);
							}
							meshOut->SetVertexSkinningBinding(startVertId + vertexId, 
								boneIds.GetArrayView(), boneWeights.GetArrayView());
							vertexId++;
						}
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
					for (auto i = 0; i < vertexId; i++)
						meshOut->SetVertexSkinningBinding(startVertId + i, boneIds.GetArrayView(), boneWeights.GetArrayView());
				}
				startVertId += vertexId;
			}
		}
		if (meshOut->ElementRanges.Count())
		{
			auto optimizedMesh = meshOut->DeduplicateVertices();
			optimizedMesh.SaveToFile(Path::ReplaceExt(outFileName, "mesh"));
			wprintf(L"mesh converted: elements %d, faces: %d, vertices: %d, skeletal: %s.\n", optimizedMesh.ElementRanges.Count(), optimizedMesh.Indices.Count() / 3, optimizedMesh.GetVertexCount(),
				hasBones ? L"true" : L"false");
		}
	}

	if (args.ExportAnimation && skeletonNodes.Count())
	{
		bool inconsistentKeyFrames = false;
		SkeletalAnimation anim;
		FbxAnimStack* currAnimStack = lScene->GetCurrentAnimationStack();
		FbxString animStackName = currAnimStack->GetName();
		char * mAnimationName = animStackName.Buffer();
		FbxTakeInfo* takeInfo = lScene->GetTakeInfo(animStackName);
		fbxsdk::FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
		fbxsdk::FbxTime end = takeInfo->mLocalTimeSpan.GetStop();
		int mAnimationLength = (int)(end.GetFrameCount(fbxsdk::FbxTime::eNTSCFullFrame) - start.GetFrameCount(fbxsdk::FbxTime::eNTSCFullFrame) + 1);
		anim.Speed = 1.0f;
		anim.Duration = mAnimationLength / 30.0f;
		anim.Name = mAnimationName;
		anim.Channels.SetSize(skeletonNodes.Count());
		memset(anim.Reserved, 0, sizeof(anim.Reserved));
		int channelId = 0;
		for (auto inNode : skeletonNodes)
		{
			anim.Channels[channelId].BoneName = inNode->GetName();
			bool isRoot = anim.Channels[channelId].BoneName == args.RootNodeName;
			unsigned int ptrPos = 0, ptrRot = 0, ptrScale = 0;
			for (FbxLongLong i = start.GetFrameCount(fbxsdk::FbxTime::eNTSCFullFrame); i <= end.GetFrameCount(fbxsdk::FbxTime::eNTSCFullFrame); ++i)
			{
				fbxsdk::FbxTime currTime;
				currTime.SetFrame(i, fbxsdk::FbxTime::eNTSCFullFrame);
				AnimationKeyFrame keyFrame;
				keyFrame.Time = (float)currTime.GetSecondDouble();
				FbxAMatrix currentTransformOffset = inNode->EvaluateLocalTransform(currTime);
				keyFrame.Transform.FromMatrix(GetMatrix(currentTransformOffset));
				// apply root fix transform
				keyFrame.Transform = RootTransformApplier::ApplyFix(rootFixTransform, keyFrame.Transform);
				keyFrame.Transform = rootTransformApplier.Apply(keyFrame.Transform);
				anim.Channels[channelId].KeyFrames.Add(keyFrame);
			}
			channelId++;
		}
		anim.SaveToFile(Path::ReplaceExt(outFileName, "anim"));
		int maxKeyFrames = 0;
		for (auto & c : anim.Channels)
			maxKeyFrames = Math::Max(maxKeyFrames, c.KeyFrames.Count());
		printf("animation converted. keyframes %d, bones %d\n", maxKeyFrames, anim.Channels.Count());
	}
	
	// Destroy the SDK manager and all the other objects it was handling.
	lSdkManager->Destroy();
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