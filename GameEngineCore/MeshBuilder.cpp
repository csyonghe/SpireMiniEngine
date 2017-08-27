#include "MeshBuilder.h"

namespace GameEngine
{
    using namespace VectorMath;

    void MeshBuilder::AddTriangle(Vec3 v1, Vec3 v2, Vec3 v3)
    {
        Vec3 normal = Vec3::Cross(v2 - v1, v3 - v1).Normalize();
        Vec3 tangent = (v2 - v1).Normalize();
        indices.Add(vertices.Count());
        vertices.Add(Vertex{ v1, Vec2::Create(0.0f), normal, tangent });
        indices.Add(vertices.Count());
        vertices.Add(Vertex{ v2, Vec2::Create(0.0f), normal, tangent });
        indices.Add(vertices.Count());
        vertices.Add(Vertex{ v3, Vec2::Create(0.0f), normal, tangent });
    }

    void MeshBuilder::AddTriangle(Vec3 v1, Vec2 uv1, Vec3 v2, Vec2 uv2, Vec3 v3, Vec2 uv3)
    {
        Vec3 normal = Vec3::Cross(v2 - v1, v3 - v1).Normalize();
        Vec3 tangent = (v2 - v1).Normalize();
        indices.Add(vertices.Count());
        vertices.Add(Vertex{v1, uv1, normal, tangent});
        indices.Add(vertices.Count());
        vertices.Add(Vertex{v2, uv2, normal, tangent});
        indices.Add(vertices.Count());
        vertices.Add(Vertex{v3, uv3, normal, tangent});
    }

    void MeshBuilder::AddQuad(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
    {
        Vec3 normal = Vec3::Cross(v2 - v1, v3 - v1).Normalize();
        Vec3 tangent = (v2 - v1).Normalize();
        indices.Add(vertices.Count());
        vertices.Add(Vertex{ v1, Vec2::Create(0.0f), normal, tangent });
        indices.Add(vertices.Count());
        vertices.Add(Vertex{ v2, Vec2::Create(1.0f, 0.0f), normal, tangent });
        indices.Add(vertices.Count());
        vertices.Add(Vertex{ v3, Vec2::Create(1.0f, 1.0f), normal, tangent });
        indices.Add(vertices.Count());
        vertices.Add(Vertex{ v1, Vec2::Create(0.0f), normal, tangent });
        indices.Add(vertices.Count());
        vertices.Add(Vertex{ v3, Vec2::Create(1.0f, 1.0f), normal, tangent });
        indices.Add(vertices.Count());
        vertices.Add(Vertex{ v4, Vec2::Create(0.0f, 1.0f), normal, tangent });
    }

    void MeshBuilder::AddLine(Vec3 v1, Vec3 v2, Vec3 normal, float width)
    {
        Vec3 tangent = (v2 - v1).Normalize();
        Vec3 binormal = Vec3::Cross(tangent, normal).Normalize();
        auto delta = binormal * (width * 0.5f);
        Vec3 p1 = v1 + delta;
        Vec3 p2 = v2 + delta;
        Vec3 p3 = v2 - delta;
        Vec3 p4 = v1 - delta;
        AddQuad(p1, p2, p3, p4);
    }

    void MeshBuilder::AddDisk(Vec3 center, Vec3 normal, float radiusOuter, float radiusInner, int edges)
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
            vertices.Add(Vertex{ center + tangent * x1 + binormal * y1, uvInner, normal, tangent });
            vertices.Add(Vertex{ center + tangent * x2 + binormal * y2, uvOuter, normal, tangent });
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
    }

    void MeshBuilder::AddBox(Vec3 vmin, Vec3 vmax)
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
    }

    void MeshBuilder::AddFrustum(Vec3 /*v1*/, Vec3 /*v2*/, float /*radius1*/, float /*radius2*/, int /*edges*/)
    {
        // TO be implemented
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
