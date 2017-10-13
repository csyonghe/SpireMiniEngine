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
		if (fieldName == "model")
		{
			modelFileName = parser.ReadStringLiteral();
			model = level->LoadModel(modelFileName);
			Bounds = model->GetBounds();
			return true;
		}
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
				MeshName = parser.ReadStringLiteral();
				if (MeshName.Length())
					Mesh = level->LoadMesh(MeshName);
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
				materialFileName = parser.ReadStringLiteral();
				if (materialFileName.Length())
					MaterialInstance = level->LoadMaterial(materialFileName);
			}
			return true;
		}
		return false;
	}

	void StaticMeshActor::SerializeFields(CoreLib::StringBuilder & sb)
	{
		if (modelFileName.Length())
		{
			sb << "model " << CoreLib::Text::EscapeStringLiteral(modelFileName) << "\n";
		}
		else
		{
			if (MeshName.Length())
				sb << "mesh " << CoreLib::Text::EscapeStringLiteral(MeshName) << "\n";
			if (useInlineMaterial)
				MaterialInstance->Serialize(sb);
			else
				sb << "material " << CoreLib::Text::EscapeStringLiteral(materialFileName) << "\n";
		}
	}

	void StaticMeshActor::OnLoad()
	{
		if (!model && Mesh && MaterialInstance)
		{
			model = new GameEngine::Model(Mesh, MaterialInstance);
		}
		SetLocalTransform(*LocalTransform); // update bbox
		// update physics scene
		physInstance = model->CreatePhysicsInstance(level->GetPhysicsScene(), this, nullptr);
		physInstance->SetTransform(*LocalTransform);
	}

	void StaticMeshActor::OnUnload()
	{
		physInstance->RemoveFromScene();
	}

	void StaticMeshActor::GetDrawables(const GetDrawablesParameter & params)
	{
		auto insertDrawable = [&](Drawable * d)
		{
			d->CastShadow = CastShadow;
			d->Bounds = Bounds;
			params.sink->AddDrawable(d);
		};
		if (model)
		{
			if (modelInstance.IsEmpty())
				modelInstance = model->GetDrawableInstance(params);
			if (localTransformChanged)
			{
				modelInstance.UpdateTransformUniform(*LocalTransform);
				localTransformChanged = false;
			}
			for (auto &d : modelInstance.Drawables)
				insertDrawable(d.Ptr());
		}
	}

	void StaticMeshActor::SetLocalTransform(const VectorMath::Matrix4 & val)
	{
		Actor::SetLocalTransform(val);
		localTransformChanged = true;
		CoreLib::Graphics::TransformBBox(Bounds, *LocalTransform, Mesh->Bounds);
		if (physInstance)
			physInstance->SetTransform(val);
	}

}

