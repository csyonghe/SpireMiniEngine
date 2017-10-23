#include "StaticMeshActor.h"
#include "Level.h"
#include "Engine.h"
#include "MeshBuilder.h"

namespace GameEngine
{
	bool StaticMeshActor::ParseField(CoreLib::String fieldName, CoreLib::Text::TokenReader & parser)
	{
		if (Actor::ParseField(fieldName, parser))
			return true;
		if (fieldName == "mesh")
		{
			if (parser.LookAhead("{"))
			{
				static int meshCounter = 0;
				MeshBuilder mb;
				parser.ReadToken();
				while (!parser.LookAhead("}"))
				{
					parser.Read("box");
					float x0 = parser.ReadFloat();
					float y0 = parser.ReadFloat();
					float z0 = parser.ReadFloat();

					float x1 = parser.ReadFloat();
					float y1 = parser.ReadFloat();
					float z1 = parser.ReadFloat();
					mb.AddBox(Vec3::Create(x0, y0, z0), Vec3::Create(x1, y1, z1));
				}
				parser.Read("}");
				Mesh = level->LoadMesh("immMesh" + String(meshCounter++), mb.ToMesh());
			}
			else
			{
				MeshFile = parser.ReadStringLiteral();
			}
			if (Mesh)
				Bounds = Mesh->Bounds;
			return true;
		}
		else if (fieldName == "material")
		{
			if (parser.NextToken(0).Content == "{")
			{
				parser.Back(1);
				MaterialInstance = level->CreateNewMaterial();
				MaterialInstance->Parse(parser);
				MaterialInstance->Name = *Name;
				useInlineMaterial = true;
			}
			else
			{
				MaterialFile = parser.ReadStringLiteral();
			}
			return true;
		}
		return false;
	}

	void StaticMeshActor::SerializeFields(CoreLib::StringBuilder & sb)
	{
		if (useInlineMaterial)
			MaterialInstance->Serialize(sb);
	}

	void StaticMeshActor::MeshFile_Changing(CoreLib::String & newMeshFile)
	{
		ModelFile.WriteValue("");
		if (newMeshFile.Length())
			Mesh = level->LoadMesh(newMeshFile);
		if (!Mesh)
		{
			Mesh = level->LoadErrorMesh();
			newMeshFile = "";
		}
		model = nullptr;
		ModelChanged();
	}

	void StaticMeshActor::MaterialFile_Changing(CoreLib::String & newMaterialFile)
	{
		if (newMaterialFile.Length())
			MaterialInstance = level->LoadMaterial(newMaterialFile);
		if (!MaterialInstance)
		{
			MaterialInstance = level->LoadMaterial("Error.material");
			newMaterialFile = "Error.material";
		}
		model = nullptr;
		ModelChanged();
	}

	void StaticMeshActor::ModelFile_Changing(CoreLib::String & newModelFile)
	{
		MeshFile.WriteValue("");
		MaterialFile.WriteValue("");
		if (newModelFile.Length())
		{
			model = level->LoadModel(newModelFile);
		}
		if (!model)
			newModelFile = "";
		ModelChanged();
	}

	void StaticMeshActor::LocalTransform_Changing(VectorMath::Matrix4 & value)
	{
		localTransformChanged = true;
		if (model)
			CoreLib::Graphics::TransformBBox(Bounds, value, model->GetBounds());
		if (physInstance)
			physInstance->SetTransform(value);
	}
	
	void StaticMeshActor::ModelChanged()
	{
		if (!model && Mesh && MaterialInstance)
		{
			model = new GameEngine::Model(Mesh, MaterialInstance);
		}
		SetLocalTransform(*LocalTransform); // update bbox
		// update physics scene
		physInstance = model->CreatePhysicsInstance(level->GetPhysicsScene(), this, nullptr);
		physInstance->SetTransform(*LocalTransform);
		modelInstance.Drawables.Clear();
	}

	void StaticMeshActor::OnLoad()
	{
		if (ModelFile.GetValue().Length())
		{
			model = level->LoadModel(ModelFile.GetValue());
		}
		else
		{
			if (MeshFile.GetValue().Length())
				Mesh = level->LoadMesh(MeshFile.GetValue());
			if (!Mesh)
				Mesh = level->LoadErrorMesh();
			if (MaterialFile.GetValue().Length())
				MaterialInstance = level->LoadMaterial(MaterialFile.GetValue());
			if (!MaterialInstance)
				MaterialInstance = level->LoadMaterial("Error.material");
		}
		
		LocalTransform.OnChanging.Bind(this, &StaticMeshActor::LocalTransform_Changing);
		ModelChanged();

		MeshFile.OnChanging.Bind(this, &StaticMeshActor::MeshFile_Changing);
		ModelFile.OnChanging.Bind(this, &StaticMeshActor::ModelFile_Changing);
		MaterialFile.OnChanging.Bind(this, &StaticMeshActor::MaterialFile_Changing);
	}

	void StaticMeshActor::OnUnload()
	{
		physInstance->RemoveFromScene();
	}

	void StaticMeshActor::GetDrawables(const GetDrawablesParameter & params)
	{
		if (model)
		{
			GetDrawablesParameter newParams = params;
			newParams.UseSkeleton = false;
			if (modelInstance.IsEmpty())
				modelInstance = model->GetDrawableInstance(newParams);
			if (localTransformChanged)
			{
				modelInstance.UpdateTransformUniform(*LocalTransform);
				localTransformChanged = false;
			}
			AddDrawable(params, &modelInstance);
		}
	}

}

