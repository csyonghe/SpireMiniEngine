#ifndef GAME_ENGINE_CAMERA_ACTOR_H
#define GAME_ENGINE_CAMERA_ACTOR_H

#include "CoreLib/Graphics/ViewFrustum.h"
#include "CoreLib/VectorMath.h"
#include "GizmoActor.h"
#include "Physics.h"
#include "View.h"

namespace GameEngine
{
	class CameraActor : public GizmoActor
	{
	private:
		VectorMath::Matrix4 cameraTransform;
		void TransformUpdated();
		void LocalTransform_Changed();
	public:
		PROPERTY_DEF(VectorMath::Vec3, Orientation, VectorMath::Vec3::Create(0.0f));
		PROPERTY(VectorMath::Vec3, Position);
		PROPERTY_DEF(float, ZNear, 40.0f);
		PROPERTY_DEF(float, ZFar, 400000.0f);
		PROPERTY_DEF(float, FOV, 75.0f);
		PROPERTY_DEF(float, Radius, 50.0f);
	public:
		VectorMath::Vec3 GetPosition()
		{
			return Position.GetValue();
		}
		void SetPosition(const VectorMath::Vec3 & value);
		float GetYaw() { return Orientation->x; }
		float GetPitch() { return Orientation->y; }
		float GetRoll() { return Orientation->z; }
		void SetYaw(float value);
		void SetPitch(float value);
		void SetRoll(float value);
		void SetOrientation(float pYaw, float pPitch, float pRoll);
		VectorMath::Matrix4 GetCameraTransform();
		Ray GetRayFromViewCoordinates(float x, float y, float aspect); // coordinate range from 0.0 to 1.0
		View GetView();
		CoreLib::Graphics::ViewFrustum GetFrustum(float aspect);
		virtual void Tick() override { }
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Camera;
		}
		virtual CoreLib::String GetTypeName() override
		{
			return "Camera";
		}
		virtual void OnLoad() override;
	};
}

#endif