#include "TerrainActor.h"
#include "Level.h"
#include "RenderContext.h"
#include "LibIO.h"
#include "Engine.h"
#include "RendererService.h"

using namespace CoreLib;
using namespace CoreLib::IO;
using namespace VectorMath;

namespace GameEngine
{
	struct VertexData
	{
		Vec3 Position;
		Vec3 Normal;
		Vec3 Tangent;
		Vec2 UV;
	};
	void TerrainActor::BuildMesh(int w, int h, float pCellSpace, float pHeightScale, CoreLib::ArrayView<unsigned short> heightField)
	{
		terrainMesh.Bounds.Init();

		List<VertexData> vertices;

		for (int i = 0; i < h; i++)
		{
			float z = (i - (h >> 1)) * pCellSpace;
			for (int j = 0; j < w; j++)
			{
				float x = (j - (w >> 1)) * pCellSpace;
				float y = (float)(heightField[i*w + j] - 32768) * pHeightScale;
				VertexData vert;
				vert.Position = Vec3::Create(x, y, z);
				vert.UV = Vec2::Create((float)(j) / (float)(w-1), (float)i / (float)(h-1));
				vert.Normal = Vec3::Create(0.0f, 1.0f, 0.0f);
				vertices.Add(vert);
				terrainMesh.Bounds.Union(vert.Position);
			}
		}
		for (int i = 0; i < h - 1; i++)
		{
			for (int j = 0; j < w - 1; j++)
			{
				Vec3 a = vertices[i*w + j + 1].Position
					- vertices[i*w + j].Position;
				Vec3 b = vertices[(i + 1)*w + j].Position
					- vertices[i*w + j].Position;
				Vec3 norm;
				Vec3::Cross(norm, b, a);
				Vec3::Normalize(norm, norm);
				vertices[i*w + j].Normal = norm;
			}
		}
		List<Vec3> newNormal;
		newNormal.SetSize(vertices.Count());
		for (int i = 1; i < h - 1; i++)
		{
			for (int j = 1; j < w - 1; j++)
			{
				Vec3 normal;
				normal.SetZero();
				normal += vertices[i*w + j].Normal;
				normal += vertices[(i + 1)*w + j].Normal;
				normal += vertices[i*w + j + 1].Normal;
				normal += vertices[(i + 1)*w + j + 1].Normal;
				normal *= 0.2f;
				newNormal[i * w + j] = normal;
			}
		}
		for (int i = 1; i < h - 1; i++)
		{
			for (int j = 1; j < w - 1; j++)
			{
				vertices[i*w + j].Normal = (newNormal[i*w + j]).Normalize();
				vertices[i*w + j].Tangent = Vec3::Cross(newNormal[i*w + j], Vec3::Create(0.0f, 0.0f, 1.0f)).Normalize();
			}
		}

		List<int> indexBuffer;
		int stride = w;
		for (int y = 0; y < h - 1; y++)
			for (int x = 0; x < w - 1; x++)
			{
				indexBuffer.Add(y*stride + x);
				indexBuffer.Add((y + 1)*stride + x);
				indexBuffer.Add((y + 1)*stride + x + 1);

				indexBuffer.Add(y*stride + x);
				indexBuffer.Add((y + 1)*stride + x + 1);
				indexBuffer.Add(y*stride + x + 1);
			}
		terrainMesh.SetVertexFormat(MeshVertexFormat(0, 1, true, false));
		terrainMesh.AllocVertexBuffer(vertices.Count());
		terrainMesh.Indices = _Move(indexBuffer);
		for (int i = 0; i < vertices.Count(); i++)
		{
			terrainMesh.SetVertexPosition(i, vertices[i].Position);
			Vec3 bitangent = Vec3::Cross(vertices[i].Tangent, vertices[i].Normal);
			terrainMesh.SetVertexTangentFrame(i, Quaternion::FromCoordinates(vertices[i].Tangent, vertices[i].Normal, bitangent));
			terrainMesh.SetVertexUV(i, 0, vertices[i].UV);
		}
	}
	void TerrainActor::SerializeFields(CoreLib::StringBuilder & sb)
	{
		sb << "height " << width << " " << height << " " << cellSpace << " " << heightScale << " " << CoreLib::Text::EscapeStringLiteral(heightmapFileName) << "\n";
		sb << "material " << CoreLib::Text::EscapeStringLiteral(materialFileName) << "\n";
	}
	bool TerrainActor::ParseField(CoreLib::String fieldName, CoreLib::Text::TokenReader & parser)
	{
		if (Actor::ParseField(fieldName, parser))
			return true;
		if (fieldName == "height")
		{
			width = parser.ReadInt();
			height = parser.ReadInt();
			cellSpace = parser.ReadFloat();
			heightScale = parser.ReadFloat();
			heightmapFileName = parser.ReadStringLiteral();
			List<unsigned short> heightField;
			heightField.SetSize(width * height);
			auto heightFileName = Engine::Instance()->FindFile(heightmapFileName, ResourceType::Landscape);
			if (heightFileName.Length())
			{
				CoreLib::IO::BinaryReader binReader(new CoreLib::IO::FileStream(heightFileName));
				binReader.Read(heightField.Buffer(), width * height);
				BuildMesh(width, height, cellSpace, heightScale, heightField.GetArrayView());
			}
			else
			{
				Print("landscape file not found: '%S'.\n", heightmapFileName.ToWString());
				return false;
			}
			return true;
		}
		if (parser.LookAhead("material"))
		{
			parser.ReadToken();
			materialFileName = parser.ReadStringLiteral();
			MaterialInstance = level->LoadMaterial(materialFileName);
			if (!MaterialInstance)
				return false;
			return true;
		}
		return false;
	}
	void TerrainActor::OnLoad()
	{
		SetLocalTransform(*LocalTransform);
	}
	void TerrainActor::GetDrawables(const GetDrawablesParameter & params)
	{
		if (!drawable)
			drawable = params.rendererService->CreateStaticDrawable(&terrainMesh, 0, MaterialInstance);
		if (localTransformChanged)
		{
			drawable->UpdateTransformUniform(*LocalTransform);
			localTransformChanged = false;
		}
		drawable->Bounds = Bounds;
		params.sink->AddDrawable(drawable.Ptr());
	}

	void TerrainActor::SetLocalTransform(const VectorMath::Matrix4 & val)
	{
		Actor::SetLocalTransform(val);
		localTransformChanged = true;
		CoreLib::Graphics::TransformBBox(Bounds, *LocalTransform, terrainMesh.Bounds);
	}
}
