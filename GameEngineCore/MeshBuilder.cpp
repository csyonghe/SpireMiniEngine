#include "MeshBuilder.h"

namespace GameEngine
{
    using namespace VectorMath;

	void MeshBuilder::UpdateElementRange(bool newElement)
	{
		if (newElement)
		{
			MeshElementRange range;
			if (elementRanges.Count())
				range.StartIndex = elementRanges.Last().StartIndex + elementRanges.Last().Count;
			else
				range.StartIndex = 0;
			range.Count = indices.Count() - range.StartIndex;
			elementRanges.Add(range);
		}
		else
		{
			if (elementRanges.Count())
				elementRanges.Last().Count = indices.Count() - elementRanges.Last().StartIndex;
			else
			{
				MeshElementRange range;
				range.StartIndex = 0;
				range.Count = indices.Count();
				elementRanges.Add(range);
			}
		}
	}

	void MeshBuilder::AddVertex(MeshBuilder::Vertex vtx)
	{
		vtx.pos = GetCurrentTransform().TransformHomogeneous(vtx.pos);
		auto bitangent = Vec3::Cross(vtx.tangent, vtx.normal);
		bitangent = GetCurrentTransform().TransformNormal(bitangent);
		vtx.tangent = GetCurrentTransform().TransformNormal(vtx.tangent);
		vtx.normal = Vec3::Cross(bitangent, vtx.tangent).Normalize();
		vertices.Add(vtx);
	}

	void MeshBuilder::PushRotation(VectorMath::Vec3 axis, float angle)
	{
		Matrix4 rot;
		Matrix4::Rotation(rot, axis, angle);
		Matrix4::Multiply(rot, GetCurrentTransform(), rot);
		transformStack.Add(rot);
	}

	void MeshBuilder::PushTranslation(VectorMath::Vec3 offset)
	{
		Matrix4 trans;
		Matrix4::Translation(trans, offset.x, offset.y, offset.z);
		Matrix4::Multiply(trans, GetCurrentTransform(), trans);
		transformStack.Add(trans);
	}

	void MeshBuilder::PushScale(VectorMath::Vec3 val)
	{
		Matrix4 scale;
		Matrix4::Scale(scale, val.x, val.y, val.z);
		Matrix4::Multiply(scale, GetCurrentTransform(), scale);
		transformStack.Add(scale);
	}

	void MeshBuilder::PopTransform()
	{
		transformStack.SetSize(transformStack.Count() - 1);
	}

	void MeshBuilder::AddTriangle(Vec3 v1, Vec3 v2, Vec3 v3, bool asNewElement)
    {
        Vec3 normal = Vec3::Cross(v2 - v1, v3 - v1).Normalize();
        Vec3 tangent = (v2 - v1).Normalize();
        indices.Add(vertices.Count());
        AddVertex(Vertex{ v1, Vec2::Create(0.0f), normal, tangent });
        indices.Add(vertices.Count());
		AddVertex(Vertex{ v2, Vec2::Create(0.0f), normal, tangent });
        indices.Add(vertices.Count());
		AddVertex(Vertex{ v3, Vec2::Create(0.0f), normal, tangent });
		UpdateElementRange(asNewElement);
    }

    void MeshBuilder::AddTriangle(Vec3 v1, Vec2 uv1, Vec3 v2, Vec2 uv2, Vec3 v3, Vec2 uv3, bool asNewElement)
    {
        Vec3 normal = Vec3::Cross(v2 - v1, v3 - v1).Normalize();
        Vec3 tangent = (v2 - v1).Normalize();
        indices.Add(vertices.Count());
		AddVertex(Vertex{v1, uv1, normal, tangent});
        indices.Add(vertices.Count());
		AddVertex(Vertex{v2, uv2, normal, tangent});
        indices.Add(vertices.Count());
		AddVertex(Vertex{v3, uv3, normal, tangent});
		UpdateElementRange(asNewElement);
    }

    void MeshBuilder::AddQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4, bool asNewElement)
    {
        Vec3 normal = Vec3::Cross(v2 - v1, v3 - v1).Normalize();
        Vec3 tangent = (v2 - v1).Normalize();
        indices.Add(vertices.Count());
		AddVertex(Vertex{ v1, Vec2::Create(0.0f), normal, tangent });
        indices.Add(vertices.Count());
		AddVertex(Vertex{ v2, Vec2::Create(1.0f, 0.0f), normal, tangent });
        indices.Add(vertices.Count());
		AddVertex(Vertex{ v3, Vec2::Create(1.0f, 1.0f), normal, tangent });
        indices.Add(vertices.Count());
		AddVertex(Vertex{ v1, Vec2::Create(0.0f), normal, tangent });
        indices.Add(vertices.Count());
		AddVertex(Vertex{ v3, Vec2::Create(1.0f, 1.0f), normal, tangent });
        indices.Add(vertices.Count());
		AddVertex(Vertex{ v4, Vec2::Create(0.0f, 1.0f), normal, tangent });
		UpdateElementRange(asNewElement);
    }

    void MeshBuilder::AddLine(Vec3 v1, Vec3 v2, Vec3 normal, float width, bool asNewElement)
    {
        Vec3 tangent = (v2 - v1).Normalize();
        Vec3 binormal = Vec3::Cross(tangent, normal).Normalize();
        auto delta = binormal * (width * 0.5f);
        Vec3 p1 = v1 + delta;
        Vec3 p2 = v2 + delta;
        Vec3 p3 = v2 - delta;
        Vec3 p4 = v1 - delta;
        AddQuad(p1, p2, p3, p4);
		UpdateElementRange(asNewElement);
    }

    void MeshBuilder::AddDisk(Vec3 center, Vec3 normal, float radiusOuter, float radiusInner, int edges, bool asNewElement)
    {
        Vec3 tangent;
        GetOrthoVec(tangent, normal);
        Vec3 binormal = Vec3::Cross(tangent, normal).Normalize();
        int startId = vertices.Count();

        for (int i = 0; i < edges; i++)
        {
            float theta = Math::Pi * 2.0f * (i / (float)(edges));
            float c = cos(theta);
            float s = sin(theta);
            float x1 = c * radiusInner;
            float y1 = s * radiusInner;
            float x2 = c * radiusOuter;
            float y2 = s * radiusOuter;
            auto uvOuter = Vec2::Create((c + 1.0f) * 0.5f, 1.0f - (s + 1.0f) * 0.5f);
            auto uvInner = (uvOuter - Vec2::Create(0.5f))*(radiusInner / radiusOuter) + Vec2::Create(0.5f);
			AddVertex(Vertex{ center + tangent * x1 + binormal * y1, uvInner, normal, tangent });
			AddVertex(Vertex{ center + tangent * x2 + binormal * y2, uvOuter, normal, tangent });
        }

        int vertCount = edges * 2;
        
        for (int i = 0; i < edges; i++)
        {
            indices.Add(startId + i * 2);
            indices.Add(startId + (i * 2 + 3) % vertCount);
            indices.Add(startId + i * 2 + 1);

            indices.Add(startId + i * 2);
            indices.Add(startId + (i * 2 + 2) % vertCount);
            indices.Add(startId + (i * 2 + 3) % vertCount);
        }
		UpdateElementRange(asNewElement);
    }

	void MeshBuilder::AddCone(float radius, float height, int edges, bool asNewElement)
	{
		auto topVertex = Vec3::Create(0.0f, height, 0.0f);
		for (int i = 0; i < edges; i++)
		{
			float t = i / (float)(edges);
			float t1 = (i + 1) / (float)(edges);
			float t2 = (i + 2) / (float)(edges);
			float angle = Math::Pi * 2.0f * t;
			float angle1 = Math::Pi * 2.0f * t1;
			float angle2 = Math::Pi * 2.0f * t2;
			auto v0 = Vec3::Create(cos(angle)*radius, 0.0f, -sin(angle)*radius);
			auto v1 = Vec3::Create(cos(angle1)*radius, 0.0f, -sin(angle1)*radius);
			auto v2 = Vec3::Create(cos(angle2)*radius, 0.0f, -sin(angle2)*radius);

			auto tangent = (v1 - v0).Normalize();
			auto tangent2 = (v2 - v1).Normalize();
			Vertex v;
			v.pos = topVertex;
			v.normal = Vec3::Create(cos(angle), radius / height, -sin(angle)).Normalize();
			v.tangent = tangent;
			v.uv - Vec2::Create(0.0f);
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v0;
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v1;
			v.tangent = tangent2;
			v.normal = Vec3::Create(cos(angle1), radius / height, -sin(angle1)).Normalize();
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = Vec3::Create(0.0f);
			v.normal = Vec3::Create(0.0f, -1.0f, 0.0f);
			v.tangent = Vec3::Create(1.0f, 0.0f, 0.0f);
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v1;
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v0;
			indices.Add(vertices.Count());
			AddVertex(v);
		}
		UpdateElementRange(asNewElement);
	}

	void MeshBuilder::AddCylinder(float radius, float height, int edges, bool asNewElement)
	{
		auto topVertex = Vec3::Create(0.0f, height, 0.0f);
		for (int i = 0; i < edges; i++)
		{
			float t = i / (float)(edges);
			float t1 = (i + 1) / (float)(edges);
			float t2 = (i + 2) / (float)(edges);

			float angle = Math::Pi * 2.0f * t;
			float angle1 = Math::Pi * 2.0f * t1;
			float angle2 = Math::Pi * 2.0f * t2;
			auto v0 = Vec3::Create(cos(angle)*radius, 0.0f, -sin(angle)*radius);
			auto v1 = Vec3::Create(cos(angle1)*radius, 0.0f, -sin(angle1)*radius);
			auto v2 = v1;
			v2.y = height;
			auto v3 = v0;
			v3.y = height;
			auto tangent = (v1 - v0).Normalize();
			auto v4 = Vec3::Create(cos(angle2)*radius, 0.0f, -sin(angle2)*radius);
			auto tangent2 = (v4 - v1).Normalize();
			auto normal1 = Vec3::Create(cos(angle), 0.0f, -sin(angle));
			auto normal2 = Vec3::Create(cos(angle1), 0.0f, -sin(angle1));
			Vertex v;
			v.pos = v0;
			v.normal = normal1;
			v.tangent = tangent;
			v.uv - Vec2::Create(0.0f);
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v1;
			v.tangent = tangent2;
			v.normal = normal2;
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v2;
			v.normal = normal2;
			v.tangent = tangent2;
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v0;
			v.normal = normal1;
			v.tangent = tangent;
			indices.Add(vertices.Count());
			AddVertex(v);

			v.pos = v2;
			v.normal = normal2;
			v.tangent = tangent2;
			indices.Add(vertices.Count());
			AddVertex(v);

			v.pos = v3;
			v.normal = normal1;
			v.tangent = tangent;
			indices.Add(vertices.Count());
			AddVertex(v);

			v.pos = Vec3::Create(0.0f);
			v.normal = Vec3::Create(0.0f, -1.0f, 0.0f);
			v.tangent = Vec3::Create(-1.0f, 0.0f, 0.0f);
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v1;
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v0;
			indices.Add(vertices.Count());
			AddVertex(v);

			v.pos = Vec3::Create(0.0f, height, 0.0f);
			v.normal = Vec3::Create(0.0f, 1.0f, 0.0f);
			v.tangent = Vec3::Create(1.0f, 0.0f, 0.0f);
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v3;
			indices.Add(vertices.Count());
			AddVertex(v);
			v.pos = v2;
			indices.Add(vertices.Count());
			AddVertex(v);
		}
		UpdateElementRange(asNewElement);
	}

    void MeshBuilder::AddBox(Vec3 vmin, Vec3 vmax, bool asNewElement)
    {
        AddQuad(Vec3::Create(vmax.x, vmax.y, vmin.z),
                Vec3::Create(vmin.x, vmax.y, vmin.z),
                Vec3::Create(vmin.x, vmax.y, vmax.z),
                Vec3::Create(vmax.x, vmax.y, vmax.z));

        AddQuad(Vec3::Create(vmin.x, vmin.y, vmax.z),
                Vec3::Create(vmin.x, vmin.y, vmin.z),
                Vec3::Create(vmax.x, vmin.y, vmin.z),
                Vec3::Create(vmax.x, vmin.y, vmax.z));

        AddQuad(Vec3::Create(vmin.x, vmin.y, vmax.z),
                Vec3::Create(vmax.x, vmin.y, vmax.z),
                Vec3::Create(vmax.x, vmax.y, vmax.z),
                Vec3::Create(vmin.x, vmax.y, vmax.z));

        AddQuad(Vec3::Create(vmin.x, vmin.y, vmin.z),
                Vec3::Create(vmin.x, vmax.y, vmin.z),
                Vec3::Create(vmax.x, vmax.y, vmin.z),
                Vec3::Create(vmax.x, vmin.y, vmin.z));

        AddQuad(Vec3::Create(vmin.x, vmin.y, vmin.z),
                Vec3::Create(vmin.x, vmin.y, vmax.z),
                Vec3::Create(vmin.x, vmax.y, vmax.z),
                Vec3::Create(vmin.x, vmax.y, vmin.z));

        AddQuad(Vec3::Create(vmax.x, vmin.y, vmax.z),
                Vec3::Create(vmax.x, vmin.y, vmin.z),
                Vec3::Create(vmax.x, vmax.y, vmin.z),
                Vec3::Create(vmax.x, vmax.y, vmax.z));
		UpdateElementRange(asNewElement);
    }

    void MeshBuilder::AddFrustum(Vec3 /*v1*/, Vec3 /*v2*/, float /*radius1*/, float /*radius2*/, int /*edges*/, bool /*asNewElement*/)
    {
        // TO be implemented
    }

	void MeshBuilder::AddPyramid(float width, float depth, float height, bool asNewElement)
	{
		auto v0 = Vec3::Create(-width * 0.5f, 0.0f, -depth *0.5f);
		auto v1 = Vec3::Create(-width * 0.5f, 0.0f, depth *0.5f);
		auto v2 = Vec3::Create(width * 0.5f, 0.0f, depth *0.5f);
		auto v3 = Vec3::Create(width * 0.5f, 0.0f, -depth *0.5f);
		auto vTop = Vec3::Create(0.0f, height, 0.0f);
		AddQuad(v3, v2, v1, v0);
		AddTriangle(vTop, v0, v1);
		AddTriangle(vTop, v1, v2);
		AddTriangle(vTop, v2, v3);
		AddTriangle(vTop, v3, v0);
		UpdateElementRange(asNewElement);
	}

    Mesh MeshBuilder::ToMesh()
    {
        Mesh rs;
		rs.Bounds.Init();
        rs.SetVertexFormat(MeshVertexFormat(0, 1, true, false));
        rs.AllocVertexBuffer(vertices.Count());
        for (int i = 0; i < vertices.Count(); i++)
        {
            auto v = vertices[i];
			rs.Bounds.Union(v.pos);
            Vec3 binormal = Vec3::Cross(v.tangent, v.normal).Normalize();
            Quaternion q = Quaternion::FromCoordinates(v.tangent, v.normal, binormal);
            rs.SetVertexPosition(i, v.pos);
            rs.SetVertexUV(i, 0, v.uv);
            rs.SetVertexTangentFrame(i, q);
        }
        rs.Indices = indices;
		rs.ElementRanges = elementRanges;
        return rs;
    }

	Mesh MeshBuilder::TransformMesh(Mesh & input, Matrix4 & transform)
	{
		Mesh rs = input;
		for (int i = 0; i < rs.GetVertexCount(); i++)
		{
			auto pos = rs.GetVertexPosition(i);
			pos = transform.TransformHomogeneous(pos);
			rs.SetVertexPosition(i, pos);
			auto mat3 = transform.GetMatrix3();
			auto q = Quaternion::FromMatrix(mat3);
			auto t = rs.GetVertexTangentFrame(i);
			t = q * t; // or t * q ?
			rs.SetVertexTangentFrame(i, t);
		}
		return rs;
	}

}
