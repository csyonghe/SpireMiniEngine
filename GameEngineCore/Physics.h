#ifndef GAME_ENGINE_PHYSICS_H
#define GAME_ENGINE_PHYSICS_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Graphics/BBox.h"

namespace GameEngine
{
	struct Ray
	{
		VectorMath::Vec3 Origin;
		VectorMath::Vec3 Dir;
	};
	struct PhysicsModelFace
	{
		VectorMath::Vec3 Vertices[3];
		VectorMath::Vec3 Normal;
	};

	struct HitPoint
	{
		VectorMath::Vec3 Position;
		unsigned int PackedNormal;
		float Distance = 0.0f;
		int FaceId = -1;
		VectorMath::Vec3 GetNormal();
	};

	class PhysicsModel : public CoreLib::RefObject
	{
	public:
		struct MeshFace
		{
			unsigned int ProjectionAxis : 2;
			unsigned int PackedNormal : 30;
			float PlaneU, PlaneV, PlaneD;
			float K_beta_u, K_beta_v, K_beta_d;
			float K_gamma_u, K_gamma_v, K_gamma_d;
		};
	private:
		CoreLib::Graphics::BBox bounds;
		CoreLib::List<MeshFace> faces;
		friend class PhysicsModelBuilder;
	public:
		int GetFaceCount()
		{
			return faces.Count();
		}
		CoreLib::Graphics::BBox GetBounds()
		{
			return bounds;
		}
		HitPoint TraceRay(VectorMath::Vec3 origin, VectorMath::Vec3 dir, float tmin, float tmax);
	};

	class PhysicsModelBuilder
	{
	private:
		CoreLib::RefPtr<PhysicsModel> model;
	public:
		PhysicsModelBuilder();
		void AddFace(const PhysicsModelFace & face);
		CoreLib::RefPtr<PhysicsModel> GetModel();
	};

	class Actor;

	class PhysicsObject : public CoreLib::RefObject
	{
	private:
		CoreLib::RefPtr<PhysicsModel> model = nullptr;
		VectorMath::Matrix4 modelTransform, inverseModelTransform;
		CoreLib::Graphics::BBox bounds;
		bool modelTransformChanged = false;
	public:
		void * Tag = nullptr;
		Actor * ParentActor = nullptr;
		int SkeletalBoneId = -1;
		VectorMath::Matrix4 GetInverseModelTransform()
		{
			return inverseModelTransform;
		}
		VectorMath::Matrix4 GetModelTransform()
		{
			return modelTransform;
		}
		void SetModelTransform(const VectorMath::Matrix4 & m)
		{
			modelTransform = m;
			m.Inverse(inverseModelTransform);
			modelTransformChanged = true;
			CoreLib::Graphics::TransformBBox(bounds, m, model->GetBounds());
		}
		PhysicsObject(PhysicsModel * physModel)
		{
			model = physModel;
			VectorMath::Matrix4 identity;
			VectorMath::Matrix4::CreateIdentityMatrix(identity);
			SetModelTransform(identity);
		}
		CoreLib::Graphics::BBox GetBounds()
		{
			return bounds;
		}
		PhysicsModel * GetModel()
		{
			return model.Ptr();
		}
		bool CheckModelTransformDirtyBit()
		{
			return modelTransformChanged;
		}
		void ClearModelTransformDirtyBit()
		{
			modelTransformChanged = false;
		}
	};

	struct TraceResult
	{
		VectorMath::Vec3 Position;
		VectorMath::Vec3 Normal;
		float Distance;
		PhysicsObject * Object = nullptr;
	};

	class PhysicsScene : public CoreLib::RefObject
	{
	private:
		CoreLib::EnumerableHashSet<CoreLib::RefPtr<PhysicsObject>> objects;
	public:
		void AddObject(PhysicsObject * obj);
		void RemoveObject(PhysicsObject * obj);
		void Tick();
		TraceResult RayTraceFirst(const Ray & ray, float maxDist = 1e30f);
	};
}

#endif