#ifndef GAME_ENGINE_CAMERA_ACTOR_H
#define GAME_ENGINE_CAMERA_ACTOR_H

#include "CoreLib/Graphics/ViewFrustum.h"
#include "CoreLib/VectorMath.h"
#include "Actor.h"

namespace GameEngine
{
	class CameraActor : public Actor
	{
	private:
		View view;
		float collisionRadius = 50.0f;
	protected:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid) override;
	public:
		CameraActor()
		{
			view.Position.SetZero();
		}
		VectorMath::Vec3 GetPosition()
		{
			return view.Position;
		}
		void SetPosition(const VectorMath::Vec3 & value);
		float GetYaw() { return view.Yaw; }
		float GetPitch() { return view.Pitch; }
		float GetRoll() { return view.Roll; }
		void SetYaw(float value);
		void SetPitch(float value);
		void SetRoll(float value);
		void SetOrientation(float pYaw, float pPitch, float pRoll);
		View GetView()
		{
			return view;
		}
		float GetCollisionRadius() { return collisionRadius; }
		void SetCollisionRadius(float value);
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
	};
}

#endif