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
				Bounds = Mesh->Bounds;
			}
			if (!Mesh)
			{
				isInvalid = true;
			}
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
		SetLocalTransform(localTransform); // update bbox
	}

	void StaticMeshActor::GetDrawables(const GetDrawablesParameter & params)
	{
		if (!drawable)
			drawable = params.rendererService->CreateStaticDrawable(Mesh, MaterialInstance);
		if (localTransformChanged)
		{
			drawable->UpdateTransformUniform(localTransform);
			localTransformChanged = false;
		}
		drawable->Bounds = Bounds;
		params.sink->AddDrawable(drawable.Ptr());
	}

	void StaticMeshActor::SetLocalTransform(const VectorMath::Matrix4 & val)
	{
		Actor::SetLocalTransform(val);
		localTransformChanged = true;
		CoreLib::Graphics::TransformBBox(Bounds, localTransform, Mesh->Bounds);
	}

}

