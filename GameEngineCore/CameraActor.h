#ifndef GAME_ENGINE_CAMERA_ACTOR_H
#define GAME_ENGINE_CAMERA_ACTOR_H

#include "CoreLib/Graphics/ViewFrustum.h"
#include "CoreLib/VectorMath.h"
#include "Actor.h"
#include "Physics.h"
#include "View.h"

namespace GameEngine
{
	class CameraActor : public Actor
	{
	public:
		PROPERTY(View, CurrentView);
		PROPERTY_DEF(float, CollisionRadius, 50.0f);
	public:
		CameraActor()
		{
			CurrentView->Position.SetZero();
		}
		VectorMath::Vec3 GetPosition()
		{
			return CurrentView->Position;
		}
		void SetPosition(const VectorMath::Vec3 & value);
		float GetYaw() { return CurrentView->Yaw; }
		float GetPitch() { return CurrentView->Pitch; }
		float GetRoll() { return CurrentView->Roll; }
		void SetYaw(float value);
		void SetPitch(float value);
		void SetRoll(float value);
		void SetOrientation(float pYaw, float pPitch, float pRoll);
		Ray GetRayFromViewCoordinates(float x, float y, float aspect); // coordinate range from 0.0 to 1.0
		View GetView()
		{
			return *CurrentView;
		}
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