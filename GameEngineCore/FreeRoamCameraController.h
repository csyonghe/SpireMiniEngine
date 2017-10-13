#ifndef GAME_ENGINE_FREE_ROAM_CAMERA_CONTROLLER_H
#define GAME_ENGINE_FREE_ROAM_CAMERA_CONTROLLER_H

#include "Actor.h"
#include "InputDispatcher.h"

namespace GameEngine
{
	class CameraActor;
	class FreeRoamCameraControllerActor : public Actor
	{
	public:
		PROPERTY_DEF(int, InputChannel, 0);
		PROPERTY_DEF(float, Speed, 700.0f);
		PROPERTY_DEF(float, TurnPrecision, CoreLib::Math::Pi / 4.0f);
		PROPERTY(CoreLib::String, TargetCameraName);
	private:
		CameraActor * targetCamera = nullptr;
		void FindTargetCamera();
	public:
		virtual void OnLoad() override;
		virtual void OnUnload() override;
		virtual EngineActorType GetEngineType() override;
		virtual CoreLib::String GetTypeName() override
		{
			return "FreeRoamCameraController";
		}
	public:
		bool MoveForward(const CoreLib::String & axisName, ActionInput scale);
		bool MoveRight(const CoreLib::String & axisName, ActionInput scale);
		bool MoveUp(const CoreLib::String & axisName, ActionInput scale);
		bool TurnRight(const CoreLib::String & axisName, ActionInput scale);
		bool TurnUp(const CoreLib::String & axisName, ActionInput scale);
		bool DumpCamera(const CoreLib::String & axisName, ActionInput scale);
		void SetTargetCamera(CameraActor * targetCam)
		{
			targetCamera = targetCam;
		}
	};
}

#endif