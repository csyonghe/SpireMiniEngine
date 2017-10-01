#include "StaticMeshActor.h"
#include "Level.h"
#include "Engine.h"
#include "MeshBuilder.h"

namespace GameEngine
{
	bool StaticMeshActor::ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("model"))
		{
			parser.ReadWord();
			Model = level->LoadModel(parser.ReadStringLiteral());
			Bounds = Model->GetBounds();
		}
		if (parser.LookAhead("mesh"))
		{
			parser.ReadToken();
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
				Mesh = level->LoadMesh(MeshName);
			}
			if (!Mesh)
			{
				isInvalid = true;
			}
			else
				Bounds = Mesh->Bounds;
			return true;
		}
		if (parser.LookAhead("material"))
		{
			if (parser.NextToken(1).Content == "{")
			{
				MaterialInstance = level->CreateNewMaterial();
				MaterialInstance->Parse(parser);
				MaterialInstance->Name = Name;
			}
			else
			{
				parser.ReadToken();
				auto materialName = parser.ReadStringLiteral();
				MaterialInstance = level->LoadMaterial(materialName);
				if (!MaterialInstance)
					isInvalid = true;
			}
			return true;
		}
		return false;
	}

	void StaticMeshActor::OnLoad()
	{
		if (!Model)
		{
			Model = new GameEngine::Model(Mesh, MaterialInstance);
		}
		SetLocalTransform(localTransform); // update bbox
		// update physics scene
		physInstance = Model->CreatePhysicsInstance(level->GetPhysicsScene(), this, nullptr);
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
		if (Model)
		{
			if (modelInstance.IsEmpty())
				modelInstance = Model->GetDrawableInstance(params);
			if (localTransformChanged)
			{
				modelInstance.UpdateTransformUniform(localTransform);
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
		CoreLib::Graphics::TransformBBox(Bounds, localTransform, Mesh->Bounds);
		if (physInstance)
			physInstance->SetTransform(val);
	}

}

