#include "Model.h"
#include "Level.h"
#include "Engine.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Tokenizer.h"

using namespace CoreLib;
using namespace CoreLib::IO;
using namespace CoreLib::Text;

namespace GameEngine
{
	Model::Model(Mesh * pMesh, Material * material)
	{
		mesh = *pMesh;
		materials.Add(material);
		InitPhysicsModel();
	}
	Model::Model(Mesh * pMesh, Skeleton * pSkeleton, Material * material)
	{
		mesh = *pMesh;
		materials.Add(material);
		if (pSkeleton)
			skeleton = *pSkeleton;
		InitPhysicsModel();
	}
	void Model::LoadFromFile(Level * level, CoreLib::String fileName)
	{
		LoadFromString(level, File::ReadAllText(fileName));
	}
	void Model::LoadFromString(Level * level, CoreLib::String content)
	{
		materials.Clear();
		materialFileNames.Clear();
		meshFileName = skeletonFileName = "";
		TokenReader parser(content);
		parser.Read("model");
		parser.Read("{");
		while (!parser.IsEnd() && !parser.LookAhead("}"))
		{
			auto word = parser.ReadWord();
			if (word == "mesh")
			{
				meshFileName = parser.ReadStringLiteral();
				auto actualName = Engine::Instance()->FindFile(meshFileName, ResourceType::Mesh);
				if (actualName.Length())
				{
					mesh.LoadFromFile(actualName);
				}
				else
				{
					Print("error: cannot load mesh \'%S\'\n", meshFileName.ToWString());
				}
			}
			else if (word == "skeleton")
			{
				skeletonFileName = parser.ReadStringLiteral();
				auto actualName = Engine::Instance()->FindFile(skeletonFileName, ResourceType::Mesh);
				if (actualName.Length())
				{
					skeleton.LoadFromFile(actualName);
				}
				else
				{
					Print("error: cannot load skeleton \'%S\'\n", skeletonFileName.ToWString());
				}
			}
			else if (word == "material")
			{
				auto materialFileName = parser.ReadStringLiteral();
				materialFileNames.Add(materialFileName);
				materials.Add(level->LoadMaterial(materialFileName));
			}
		}
		parser.Read("}");
		for (int i = materials.Count(); i < mesh.ElementRanges.Count(); i++)
			materials.Add(level->LoadMaterial("Error.material"));
		InitPhysicsModel();
	}
	void Model::SaveToFile(CoreLib::String fileName)
	{
		File::WriteAllText(fileName, ToString());
	}
	String Model::ToString()
	{
		StringBuilder sb;
		sb << "model\n{\n\tmesh \"" << EscapeStringLiteral(meshFileName) << "\"\n";
		if (skeletonFileName.Length())
			sb << "\tskeleton \"" << EscapeStringLiteral(skeletonFileName) << "\"\n";
		for (auto m : materialFileNames)
			sb << "\tmaterial \"" << EscapeStringLiteral(m) << "\"\n";
		sb << "}";
		return sb.ProduceString();
	}
	void Model::InitPhysicsModel()
	{
		List<PhysicsModelBuilder> builders;
		builders.SetSize(Math::Max(1, skeleton.Bones.Count()));
		for (int i = 0; i < mesh.Indices.Count(); i += 3)
		{
			PhysicsModelFace face;
			face.Vertices[0] = mesh.GetVertexPosition(mesh.Indices[i]);
			face.Vertices[1] = mesh.GetVertexPosition(mesh.Indices[i + 1]);
			face.Vertices[2] = mesh.GetVertexPosition(mesh.Indices[i + 2]);
			face.Normal = Vec3::Cross(face.Vertices[1] - face.Vertices[0], face.Vertices[2] - face.Vertices[0]).Normalize();
			if (builders.Count() == 0)
				builders[0].AddFace(face);
			else
			{
				CoreLib::Array<int, 8> boneIds;
				CoreLib::Array<float, 8> boneWeights;
				mesh.GetVertexSkinningBinding(mesh.Indices[i], boneIds, boneWeights);
				builders[boneIds[0]].AddFace(face);
			}
		}
		physModels.SetSize(builders.Count());
		for (int i = 0; i < builders.Count(); i++)
			physModels[i] = builders[i].GetModel();
	}
	ModelDrawableInstance Model::GetDrawableInstance(const GetDrawablesParameter & params)
	{
		ModelDrawableInstance rs;
		Matrix4 identityTransform;
		Matrix4::CreateIdentityMatrix(identityTransform);

		if (skeleton.Bones.Count())
		{
			for (int i = 0; i < mesh.ElementRanges.Count(); i++)
				rs.Drawables.Add(params.rendererService->CreateSkeletalDrawable(&mesh, i, &skeleton, materials[i]));
			rs.isSkeletal = true;
			Pose bindPose;
			for (int j = 0; j < skeleton.Bones.Count(); j++)
				bindPose.Transforms.Add(skeleton.Bones[j].BindPose);
			for (auto & drawable : rs.Drawables)
				drawable->UpdateTransformUniform(identityTransform, bindPose);
		}
		else
		{
			rs.isSkeletal = false;
			for (int i = 0; i < mesh.ElementRanges.Count(); i++)
			{
				auto drawable = params.rendererService->CreateStaticDrawable(&mesh, i, materials[i]);
				rs.Drawables.Add(drawable);
				drawable->UpdateTransformUniform(identityTransform);
			}
		}
		return _Move(rs);
	}

	CoreLib::RefPtr<ModelPhysicsInstance> Model::CreatePhysicsInstance(PhysicsScene & physScene, Actor * actor, void * tag)
	{
		CoreLib::RefPtr<ModelPhysicsInstance> rs = new ModelPhysicsInstance(&physScene);
		rs->isSkeletal = skeleton.Bones.Count() != 0;
		rs->skeleton = &skeleton;
		rs->objects.SetSize(physModels.Count());
		for (int i = 0; i < physModels.Count(); i++)
		{
			auto obj = new PhysicsObject(physModels[i].Ptr());
			obj->ParentActor = actor;
			obj->Tag = tag;
			obj->SkeletalBoneId = i;
			Matrix4 identity;
			Matrix4::CreateIdentityMatrix(identity);
			obj->SetModelTransform(identity);
			rs->objects[i] = obj;
			physScene.AddObject(obj);
		}
		return rs;
	}

	void ModelDrawableInstance::UpdateTransformUniform(VectorMath::Matrix4 localTransform)
	{
		for (auto & drawable : Drawables)
			drawable->UpdateTransformUniform(localTransform);
	}
	void ModelDrawableInstance::UpdateTransformUniform(VectorMath::Matrix4 localTransform, Pose & pose, RetargetFile * retargetFile)
	{
		for (auto & drawable : Drawables)
			drawable->UpdateTransformUniform(localTransform, pose, retargetFile);
	}
	void ModelPhysicsInstance::SetTransform(VectorMath::Matrix4 localTransform)
	{
		for (int i = 0; i < objects.Count(); i++)
			objects[i]->SetModelTransform(localTransform);
	}
	void ModelPhysicsInstance::SetTransform(VectorMath::Matrix4 localTransform, Pose & pose, RetargetFile * retarget)
	{
		List<Matrix4> matrices;
		pose.GetMatrices(skeleton, matrices, true, retarget);
		for (int i = 0; i < matrices.Count(); i++)
		{
			Matrix4::Multiply(matrices[i], localTransform, matrices[i]);
			objects[i]->SetModelTransform(matrices[i]);
		}
	}
	void ModelPhysicsInstance::RemoveFromScene()
	{
		for (auto obj : objects)
			scene->RemoveObject(obj);
	}
}
