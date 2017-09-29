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
}
