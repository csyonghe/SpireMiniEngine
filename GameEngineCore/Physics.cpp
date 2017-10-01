#include "Physics.h"
using namespace VectorMath;
namespace GameEngine
{
	unsigned int PackNormal(Vec3 normal)
	{
		unsigned int x = Math::Clamp((unsigned int)((normal.x * 0.5 + 0.5) * 1023), 0u, 1023u);
		unsigned int y = Math::Clamp((unsigned int)((normal.x * 0.5 + 0.5) * 1023), 0u, 1023u);
		unsigned int z = Math::Clamp((unsigned int)((normal.x * 0.5 + 0.5) * 1023), 0u, 1023u);
		return x + (y << 10) + (z << 20);
	}
	Vec3 UnpackNormal(unsigned int n)
	{
		unsigned int x = n & 1023;
		unsigned int y = (n >> 10) & 1023;
		unsigned int z = (n >> 20) & 1023;
		Vec3 rs;
		const float scale = 2.0f / 1023.0f;
		rs.x = x * scale - 1.0f;
		rs.y = y * scale - 1.0f;
		rs.z = z * scale - 1.0f;
		return rs;
	}
	inline bool RayTriangleTest(HitPoint & inter, const PhysicsModel::MeshFace & face, Vec3 origin, Vec3 dir, float tmin, float tmax)
	{
		const int mod3[] = { 0,1,2,0,1 };
		int u = mod3[face.ProjectionAxis + 1];
		int v = mod3[face.ProjectionAxis + 2];
		float invNdotD = 1.0f / (face.PlaneU * dir[u] + face.PlaneV * dir[v] + dir[face.ProjectionAxis]);
		float tplane = -(face.PlaneU * origin[u] + face.PlaneV * origin[v] + origin[face.ProjectionAxis] + face.PlaneD) * invNdotD;
		if (tplane < tmin || tplane > tmax)
			return false;
		float hitU = origin[u] + dir[u] * tplane;
		float hitV = origin[v] + dir[v] * tplane;

		float beta = face.K_beta_u * hitU + face.K_beta_v * hitV + face.K_beta_d;
		if (beta < -Epsilon)
			return false;
		float gamma = face.K_gamma_u * hitU + face.K_gamma_v * hitV + face.K_gamma_d;
		if (gamma < -Epsilon)
			return false;
		if (beta + gamma > 1.0f + Epsilon)
			return false;
		inter.Position[u] = hitU;
		inter.Position[v] = hitV;
		inter.Position[face.ProjectionAxis] = origin[face.ProjectionAxis] + dir[face.ProjectionAxis] * tplane;
		inter.PackedNormal = face.PackedNormal;
		inter.Distance = tplane;
		
		return true;
	}

	HitPoint PhysicsModel::TraceRay(VectorMath::Vec3 origin, VectorMath::Vec3 dir, float tmin, float tmax)
	{
		HitPoint current;
		current.Distance = tmax;
		for (int i = 0; i < faces.Count(); i++)
		{
			HitPoint hit;
			if (RayTriangleTest(hit, faces[i], origin, dir, tmin, current.Distance))
			{
				current = hit;
				current.FaceId = i;
			}
		}
		return current;
	}

	void PhysicsScene::AddObject(PhysicsObject * obj)
	{
		objects.Add(obj);
	}

	void PhysicsScene::RemoveObject(PhysicsObject * obj)
	{
		objects.Remove(obj);
	}

	void PhysicsScene::Tick()
	{
	}

	TraceResult PhysicsScene::RayTraceFirst(const Ray & ray, float maxDist)
	{
		TraceResult rs;
		HitPoint curHitPoint;
		curHitPoint.Distance = maxDist;
		for (auto obj : objects)
		{
			float tmin = 0.0f;
			float tmax = 0.0f;
			if (CoreLib::Graphics::RayBBoxIntersection(obj->GetBounds(), ray.Origin, ray.Dir, tmin, tmax))
			{
				if (tmax < maxDist)
				{
					// inverse transform ray
					VectorMath::Vec3 objOrigin, objDir;
					objOrigin = obj->GetInverseModelTransform().TransformHomogeneous(ray.Origin);
					objDir = obj->GetInverseModelTransform().TransformNormal(ray.Dir);
					float distScale = objDir.Length();
					objDir *= 1.0f / distScale;
					// perform object space ray casting
					auto hit = obj->GetModel()->TraceRay(objOrigin, objDir, 0.0f, 1e30f);
					hit.Distance *= distScale;
					if (hit.Distance < curHitPoint.Distance && hit.FaceId != -1)
					{
						curHitPoint = hit;
						rs.Object = obj.Ptr();
					}
				}
			}
		}
		if (rs.Object)
		{
			rs.Position = rs.Object->GetModelTransform().TransformHomogeneous(curHitPoint.Position);
			rs.Object->GetInverseModelTransform().TransposeTransformNormal(rs.Normal, curHitPoint.GetNormal());
			rs.Distance = curHitPoint.Distance;
		}
		return rs;
	}

	PhysicsModelBuilder::PhysicsModelBuilder()
	{
		model = new PhysicsModel();
		model->bounds.Init();
	}

	void PhysicsModelBuilder::AddFace(const PhysicsModelFace & face)
	{
		PhysicsModel::MeshFace f;
		Vec3 n, c, b, a;
		Vec3::Subtract(c, face.Vertices[1], face.Vertices[0]);
		Vec3::Subtract(b, face.Vertices[2], face.Vertices[0]);
		Vec3::Subtract(a, face.Vertices[2], face.Vertices[1]);
		Vec3::Cross(n, c, b);
		Vec3::Normalize(n, n);
		int k;
		Vec3 absN = Vec3::Create(abs(n.x), abs(n.y), abs(n.z));
		if (absN.x >= absN.y && absN.x >= absN.z)
			k = 0;
		else if (absN.y >= absN.x && absN.y >= absN.z)
			k = 1;
		else
			k = 2;
		int mod3[5] = { 0, 1, 2, 0, 1 };
		int u = mod3[k + 1];
		int v = mod3[k + 2];
		f.ProjectionAxis = k;
		float invNk = 1.0f / n[k];
		Vec3::Scale(n, n, invNk);
		f.PlaneU = n[u];
		f.PlaneV = n[v];
		f.PlaneD = -Vec3::Dot(n, face.Vertices[0]);
		float divisor = 1.0f / (b[u] * c[v] - b[v] * c[u]);
		f.K_beta_u = -b[v] * divisor;
		f.K_beta_v = b[u] * divisor;
		const Vec3 & A = face.Vertices[0];
		f.K_beta_d = (b[v] * A[u] - b[u] * A[v]) * divisor;
		f.K_gamma_u = c[v] * divisor;
		f.K_gamma_v = -c[u] * divisor;
		f.K_gamma_d = (c[u] * A[v] - c[v] * A[u]) * divisor;
		f.PackedNormal = PackNormal(face.Normal);
		model->faces.Add(f);
		for (int i = 0; i < 3; i++)
			model->bounds.Union(face.Vertices[i]);
	}

	CoreLib::RefPtr<PhysicsModel> PhysicsModelBuilder::GetModel()
	{
		auto rs = model;
		model = nullptr;
		return rs;
	}

	Vec3 HitPoint::GetNormal()
	{
		return UnpackNormal(PackedNormal);
	}

}

