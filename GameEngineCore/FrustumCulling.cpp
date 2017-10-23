#include "FrustumCulling.h"

using namespace VectorMath;
using namespace CoreLib;

namespace GameEngine
{
	void CullFrustum::FromVerts(CoreLib::ArrayView<VectorMath::Vec3> verts)
	{
		// top: 0, 1, 5, 4
		{
			auto normal = Vec3::Cross(verts[4] - verts[0], verts[1] - verts[0]).Normalize();
			float d = -Vec3::Dot(normal, verts[0]);
			Planes[0] = Vec4::Create(normal, d);
		}
		// bottom: 3, 2, 7, 6
		{
			auto normal = Vec3::Cross(verts[2] - verts[3], verts[7] - verts[3]).Normalize();
			float d = -Vec3::Dot(normal, verts[3]);
			Planes[1] = Vec4::Create(normal, d);
		}
		// left: 0, 4, 7, 3
		{
			auto normal = Vec3::Cross(verts[3] - verts[0], verts[4] - verts[0]).Normalize();
			float d = -Vec3::Dot(normal, verts[0]);
			Planes[2] = Vec4::Create(normal, d);
		}
		// right: 1, 5, 6, 2
		{
			auto normal = Vec3::Cross(verts[5] - verts[1], verts[2] - verts[1]).Normalize();
			float d = -Vec3::Dot(normal, verts[1]);
			Planes[3] = Vec4::Create(normal, d);
		}
		// near: 0, 1, 2, 3
		{
			auto normal = Vec3::Cross(verts[1] - verts[0], verts[2] - verts[0]).Normalize();
			float d = -Vec3::Dot(normal, verts[0]);
			Planes[4] = Vec4::Create(normal, d);
		}
		// far: 4, 5, 6, 7
		{
			auto normal = Vec3::Cross(verts[6] - verts[4], verts[5] - verts[4]).Normalize();
			float d = -Vec3::Dot(normal, verts[4]);
			Planes[5] = Vec4::Create(normal, d);
		}
	}
	bool CullFrustum::IsBoxInFrustum(CoreLib::Graphics::BBox box)
	{
		// for each plane
		for (int i = 0; i < 6; i++)
		{
			// test the positive vertex
			Vec3 p = box.Min;
			if (Planes[i].x >= 0)
				p.x = box.Max.x;
			if (Planes[i].y >= 0)
				p.y = box.Max.y;
			if (Planes[i].z >= 0)
				p.z = box.Max.z;

			float dist = Planes[i].x * p.x + Planes[i].y * p.y + Planes[i].z * p.z + Planes[i].w;
			if (dist < 0)
				return false;
		}
		return true;
	}

	CullFrustum::CullFrustum(CoreLib::Graphics::ViewFrustum f)
	{
		auto verts = f.GetVertices(f.zMin, f.zMax);
		FromVerts(verts.GetArrayView());
	}

	CullFrustum::CullFrustum(CoreLib::Graphics::Matrix4 invViewProj)
	{
		Array<Vec3, 8> verts;
		verts.Add(invViewProj.TransformHomogeneous(Vec3::Create(-1.0f, 1.0f, 0.0f)));
		verts.Add(invViewProj.TransformHomogeneous(Vec3::Create(1.0f, 1.0f, 0.0f)));
		verts.Add(invViewProj.TransformHomogeneous(Vec3::Create(1.0f, -1.0f, 0.0f)));
		verts.Add(invViewProj.TransformHomogeneous(Vec3::Create(-1.0f, -1.0f, 0.0f)));

		verts.Add(invViewProj.TransformHomogeneous(Vec3::Create(-1.0f, 1.0f, 1.0f)));
		verts.Add(invViewProj.TransformHomogeneous(Vec3::Create(1.0f, 1.0f, 1.0f)));
		verts.Add(invViewProj.TransformHomogeneous(Vec3::Create(1.0f, -1.0f, 1.0f)));
		verts.Add(invViewProj.TransformHomogeneous(Vec3::Create(-1.0f, -1.0f, 1.0f)));
		FromVerts(verts.GetArrayView());
	}

}
