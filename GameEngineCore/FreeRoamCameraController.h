#ifndef GAME_ENGINE_FREE_ROAM_CAMERA_CONTROLLER_H
#define GAME_ENGINE_FREE_ROAM_CAMERA_CONTROLLER_H

#include "Actor.h"

namespace GameEngine
{
	class CameraActor;
	class FreeRoamCameraControllerActor : public Actor
	{
	private:
		float cameraSpeed = 700.0f;
		float turnPrecision = CoreLib::Math::Pi / 4.0f;
		CoreLib::String targetCameraName;
		CameraActor * targetCamera = nullptr;
		void FindTargetCamera();
	public:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid) override;
	public:
		virtual void OnLoad() override;
		virtual void OnUnload() override;
		virtual EngineActorType GetEngineType() override;
		virtual CoreLib::String GetTypeName() override
		{
			return "FreeRoamCameraController";
		}
	public:
		bool MoveForward(const CoreLib::String & axisName, float scale);
		bool MoveRight(const CoreLib::String & axisName, float scale);
		bool MoveUp(const CoreLib::String & axisName, float scale);
		bool TurnRight(const CoreLib::String & axisName, float scale);
		bool TurnUp(const CoreLib::String & axisName, float scale);

	};
}

#endif