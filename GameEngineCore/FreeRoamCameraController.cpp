#include "FreeRoamCameraController.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace VectorMath;

	void FreeRoamCameraControllerActor::FindTargetCamera()
	{
		auto actor = Engine::Instance()->GetLevel()->FindActor(targetCameraName);
		if (actor && actor->GetEngineType() == EngineActorType::Camera)
			targetCamera = (CameraActor*)actor;
	}

	bool FreeRoamCameraControllerActor::ParseField(Level * level, CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		if (Actor::ParseField(level, parser, isInvalid))
			return true;
		if (parser.LookAhead("TargetCamera"))
		{
			parser.ReadToken();
			targetCameraName = parser.ReadStringLiteral();
			return true;
		}
		else if (parser.LookAhead("Speed"))
		{
			parser.ReadToken();
			cameraSpeed = (float)parser.ReadDouble();
			return true;
		}
		else if (parser.LookAhead("TurnPrecision"))
		{
			parser.ReadToken();
			turnPrecision = (float)parser.ReadDouble();
			return true;
		}
		return false;
	}
	void FreeRoamCameraControllerActor::OnLoad()
	{
		Actor::OnLoad();

		Engine::Instance()->GetInputDispatcher()->BindActionHandler("MoveForward", ActionInputHandlerFunc(this, &FreeRoamCameraControllerActor::MoveForward));
		Engine::Instance()->GetInputDispatcher()->BindActionHandler("MoveRight", ActionInputHandlerFunc(this, &FreeRoamCameraControllerActor::MoveRight));
		Engine::Instance()->GetInputDispatcher()->BindActionHandler("MoveUp", ActionInputHandlerFunc(this, &FreeRoamCameraControllerActor::MoveUp));
		Engine::Instance()->GetInputDispatcher()->BindActionHandler("TurnRight", ActionInputHandlerFunc(this, &FreeRoamCameraControllerActor::TurnRight));
		Engine::Instance()->GetInputDispatcher()->BindActionHandler("TurnUp", ActionInputHandlerFunc(this, &FreeRoamCameraControllerActor::TurnUp));
	}
	void FreeRoamCameraControllerActor::OnUnload()
	{
		Engine::Instance()->GetInputDispatcher()->UnbindActionHandler("MoveForward", ActionInputHandlerFunc(this, &FreeRoamCameraControllerActor::MoveForward));
		Engine::Instance()->GetInputDispatcher()->UnbindActionHandler("MoveRight", ActionInputHandlerFunc(this, &FreeRoamCameraControllerActor::MoveRight));
		Engine::Instance()->GetInputDispatcher()->UnbindActionHandler("MoveUp", ActionInputHandlerFunc(this, &FreeRoamCameraControllerActor::MoveUp));
		Engine::Instance()->GetInputDispatcher()->UnbindActionHandler("TurnRight", ActionInputHandlerFunc(this, &FreeRoamCameraControllerActor::TurnRight));
		Engine::Instance()->GetInputDispatcher()->UnbindActionHandler("TurnUp", ActionInputHandlerFunc(this, &FreeRoamCameraControllerActor::TurnUp));
	}
	EngineActorType FreeRoamCameraControllerActor::GetEngineType()
	{
		return EngineActorType::UserController;
	}
	bool FreeRoamCameraControllerActor::MoveForward(const CoreLib::String & /*axisName*/, float scale)
	{
		FindTargetCamera();
		if (targetCamera)
		{
			Vec3 moveDir = -Vec3::Create(targetCamera->GetLocalTransform().values[2], targetCamera->GetLocalTransform().values[6], targetCamera->GetLocalTransform().values[10]);
			float dTime = Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
			targetCamera->SetPosition(targetCamera->GetPosition() + moveDir * (scale * dTime * cameraSpeed));
			return true;
		}
		return false;
	}
	bool FreeRoamCameraControllerActor::MoveRight(const CoreLib::String & /*axisName*/, float scale)
	{
		FindTargetCamera();
		if (targetCamera)
		{
			Vec3 moveDir = Vec3::Create(targetCamera->GetLocalTransform().values[0], targetCamera->GetLocalTransform().values[4], targetCamera->GetLocalTransform().values[8]);
			float dTime = Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
			targetCamera->SetPosition(targetCamera->GetPosition() + moveDir * (scale * dTime * cameraSpeed));
			return true;
		}
		return false;
	}
	bool FreeRoamCameraControllerActor::MoveUp(const CoreLib::String & /*axisName*/, float scale)
	{
		FindTargetCamera();
		if (targetCamera)
		{
			Vec3 moveDir = Vec3::Create(targetCamera->GetLocalTransform().values[1], targetCamera->GetLocalTransform().values[5], targetCamera->GetLocalTransform().values[9]);
			float dTime = Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
			targetCamera->SetPosition(targetCamera->GetPosition() + moveDir * (scale * dTime * cameraSpeed));
			return true;
		}
		return false;
	}
	bool FreeRoamCameraControllerActor::TurnRight(const CoreLib::String & /*axisName*/, float scale)
	{
		FindTargetCamera();
		if (targetCamera)
		{
			float dTime = Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
			targetCamera->SetYaw(targetCamera->GetYaw() + scale * dTime * turnPrecision);
			return true;
		}
		return false;
	}
	bool FreeRoamCameraControllerActor::TurnUp(const CoreLib::String & /*axisName*/, float scale)
	{
		FindTargetCamera();
		if (targetCamera)
		{
			float dTime = Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
			targetCamera->SetPitch(targetCamera->GetPitch() - scale * dTime * turnPrecision);
			return true;
		}
		return false;
	}
}
