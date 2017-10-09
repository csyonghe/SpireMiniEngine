#include "ArcBallCameraController.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace VectorMath;

	void ArcBallCameraControllerActor::FindTargetCamera()
	{
		auto actor = level->FindActor(TargetCameraName.GetValue());
		if (actor && actor->GetEngineType() == EngineActorType::Camera)
			targetCamera = (CameraActor*)actor;
	}

	void ArcBallCameraControllerActor::UpdateCamera()
	{
		if (targetCamera)
		{
			Vec3 camPos, up, right, dir;
			CurrentArcBall.GetValue().GetCoordinates(camPos, up, right, dir);
			targetCamera->SetPosition(camPos);
			float rotX = asinf(-dir.y);
			float rotY = -atan2f(-dir.x, -dir.z);
			targetCamera->SetOrientation(rotY, rotX, 0.0f);
		}
	}

	void ArcBallCameraControllerActor::MouseDown(GraphicsUI::UI_Base * /*sender*/, GraphicsUI::UIMouseEventArgs & e)
	{
		lastArcBall = CurrentArcBall.GetValue();
		auto altMask = NeedAlt.GetValue() ? GraphicsUI::SS_ALT : 0;
		auto leftAltMask = (altMask | GraphicsUI::SS_BUTTONLEFT);
		auto rightAltMask = (altMask | GraphicsUI::SS_BUTTONRIGHT);
		lastX = e.X;
		lastY = e.Y;
		if ((e.Shift & leftAltMask) == leftAltMask)
		{
			state = MouseState::Rotate;
		}
		else if ((e.Shift & rightAltMask) == rightAltMask)
		{
			state = MouseState::Translate;
		}
		else
			state = MouseState::None;
	}

	void ArcBallCameraControllerActor::MouseMove(GraphicsUI::UI_Base * /*sender*/, GraphicsUI::UIMouseEventArgs & e)
	{
		FindTargetCamera();
		if (targetCamera)
		{
			int deltaX = e.X - lastX;
			int deltaY = e.Y - lastY;
			if (state == MouseState::Rotate)
			{
				CurrentArcBall->alpha = lastArcBall.alpha + deltaX * this->TurnPrecision.GetValue();
				CurrentArcBall->beta = lastArcBall.beta + deltaY * this->TurnPrecision.GetValue();
                CurrentArcBall->beta = Math::Clamp(CurrentArcBall->beta, -Math::Pi*0.5f + 1e-4f, Math::Pi*0.5f - 1e-4f);
			}
			else if (state == MouseState::Translate)
			{
				Vec3 camPos, up, right, dir;
				CurrentArcBall->GetCoordinates(camPos, up, right, dir);
				CurrentArcBall->center = lastArcBall.center + up * TranslatePrecision.GetValue() * CurrentArcBall->radius * (float)deltaY
					- right * TranslatePrecision.GetValue() * CurrentArcBall->radius * (float)deltaX;
			}
			UpdateCamera();
		}
	}


	void ArcBallCameraControllerActor::MouseUp(GraphicsUI::UI_Base * /*sender*/, GraphicsUI::UIMouseEventArgs & /*e*/)
	{
		state = MouseState::None;
	}

	void ArcBallCameraControllerActor::MouseWheel(GraphicsUI::UI_Base * /*sender*/, GraphicsUI::UIMouseEventArgs & e)
	{
		if (e.Delta < 0)
			CurrentArcBall->radius *= ZoomScale.GetValue();
		else
			CurrentArcBall->radius /= ZoomScale.GetValue();
		CurrentArcBall->radius = Math::Clamp(CurrentArcBall->radius, MinDist.GetValue(), MaxDist.GetValue());
		FindTargetCamera();
		UpdateCamera();
	}

	void ArcBallCameraControllerActor::OnLoad()
	{
		Actor::OnLoad();
	
		Engine::Instance()->GetUiEntry()->OnMouseDown.Bind(this, &ArcBallCameraControllerActor::MouseDown);
		Engine::Instance()->GetUiEntry()->OnMouseMove.Bind(this, &ArcBallCameraControllerActor::MouseMove);
		Engine::Instance()->GetUiEntry()->OnMouseUp.Bind(this, &ArcBallCameraControllerActor::MouseUp);
		Engine::Instance()->GetUiEntry()->OnMouseWheel.Bind(this, &ArcBallCameraControllerActor::MouseWheel);
		Engine::Instance()->GetInputDispatcher()->BindActionHandler("dumpcam", ActionInputHandlerFunc(this, &ArcBallCameraControllerActor::DumpCamera));
		FindTargetCamera();
		UpdateCamera();
	}

	EngineActorType ArcBallCameraControllerActor::GetEngineType()
	{
		return EngineActorType::UserController;
	}

	void ArcBallCameraControllerActor::SetCenter(VectorMath::Vec3 p)
	{
		CurrentArcBall->center = p;
		FindTargetCamera();
		UpdateCamera();
	}

	bool ArcBallCameraControllerActor::DumpCamera(const CoreLib::String & /*axisName*/, ActionInput /*input*/)
	{
		Print("center [%.3f, %.3f, %.3f]\n", CurrentArcBall->center.x, CurrentArcBall->center.y, CurrentArcBall->center.z);
		Print("radius %.3f\n", CurrentArcBall->radius);
		Print("alpha %.3f\n", CurrentArcBall->alpha);
		Print("beta %.3f\n", CurrentArcBall->beta);
		return true;
	}
	void ArcBallParams::Serialize(CoreLib::StringBuilder & sb)
	{
		sb << "{\n";
		sb << "center [" << center.x << " " << center.y << " " << center.z << "]\n";
		sb << "radius " << radius << "\n";
		sb << "alpha " << alpha << "\n";
		sb << "beta " << beta << "\n";
		sb << "}";
	}
	void ArcBallParams::Parse(CoreLib::Text::TokenReader & parser)
	{
		parser.Read("{");
		while (!parser.IsEnd() && !parser.LookAhead("}"))
		{
			auto word = parser.ReadWord().ToLower();
			if (word == "center")
				center = ParseVec3(parser);
			else if (word == "radius")
				radius = parser.ReadFloat();
			else if (word == "alpha")
				alpha = parser.ReadFloat();
			else if (word == "beta")
				beta = parser.ReadFloat();
		}
		parser.Read("}");
	}
	void ArcBallParams::GetCoordinates(VectorMath::Vec3 & camPos, VectorMath::Vec3 & up, VectorMath::Vec3 & right, VectorMath::Vec3 & dir)
	{
		Vec3 axisDir;
		axisDir.x = cos(alpha) * cos(beta);
		axisDir.z = sin(alpha) * cos(beta);
		axisDir.y = sin(beta);
		dir = -axisDir;
        float beta2 = beta + Math::Pi * 0.5f;
		up.x = cos(alpha) * cos(beta2);
		up.z = sin(alpha) * cos(beta2);
		up.y = sin(beta2);
		right = Vec3::Cross(dir, up);
		camPos = center + axisDir * radius;
	}
}
