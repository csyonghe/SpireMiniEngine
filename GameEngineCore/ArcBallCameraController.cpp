#include "ArcBallCameraController.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace VectorMath;

	void ArcBallCameraControllerActor::FindTargetCamera()
	{
		auto actor = Engine::Instance()->GetLevel()->FindActor(targetCameraName);
		if (actor && actor->GetEngineType() == EngineActorType::Camera)
			targetCamera = (CameraActor*)actor;
	}

	void ArcBallCameraControllerActor::UpdateCamera()
	{
		if (targetCamera)
		{
			Vec3 camPos, up, right, dir;
			currentArcBall.GetCoordinates(camPos, up, right, dir);
			targetCamera->SetPosition(camPos);
			float rotX = asinf(-dir.y);
			float rotY = -atan2f(-dir.x, -dir.z);
			targetCamera->SetOrientation(rotY, rotX, 0.0f);
		}
	}

	void ArcBallCameraControllerActor::MouseDown(GraphicsUI::UI_Base * /*sender*/, GraphicsUI::UIMouseEventArgs & e)
	{
		lastArcBall = currentArcBall;
		auto leftAltMask = (GraphicsUI::SS_ALT | GraphicsUI::SS_BUTTONLEFT);
		auto rightAltMask = (GraphicsUI::SS_ALT | GraphicsUI::SS_BUTTONRIGHT);
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
				currentArcBall.alpha = lastArcBall.alpha + deltaX * this->turnPrecision;
				currentArcBall.beta = lastArcBall.beta + deltaY * this->turnPrecision;
                currentArcBall.beta = Math::Clamp(currentArcBall.beta, -Math::Pi*0.5f + 1e-4f, Math::Pi*0.5f - 1e-4f);
			}
			else if (state == MouseState::Translate)
			{
				Vec3 camPos, up, right, dir;
				currentArcBall.GetCoordinates(camPos, up, right, dir);
				currentArcBall.center = lastArcBall.center + up * translatePrecision * currentArcBall.radius * (float)deltaY
					- right * translatePrecision * currentArcBall.radius * (float)deltaX;
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
			currentArcBall.radius *= zoomScale;
		else
			currentArcBall.radius /= zoomScale;
		currentArcBall.radius = Math::Clamp(currentArcBall.radius, minDist, maxDist);
		FindTargetCamera();
		UpdateCamera();
	}

	bool ArcBallCameraControllerActor::ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("TargetCamera"))
		{
			parser.ReadToken();
			targetCameraName = parser.ReadStringLiteral();
			return true;
		}
		else if (parser.LookAhead("TurnPrecision"))
		{
			parser.ReadToken();
			turnPrecision = (float)parser.ReadDouble();
			return true;
		}
		else if (parser.LookAhead("TranslatePrecision"))
		{
			parser.ReadToken();
			translatePrecision = (float)parser.ReadDouble();
			return true;
		}
		else if (parser.LookAhead("MinDist"))
		{
			parser.ReadToken();
			minDist = (float)parser.ReadDouble();
			return true;
		}
		else if (parser.LookAhead("MaxDist"))
		{
			parser.ReadToken();
			maxDist = (float)parser.ReadDouble();
			return true;
		}
		else if (parser.LookAhead("ZoomScale"))
		{
			parser.ReadToken();
			zoomScale = (float)parser.ReadDouble();
			return true;
		}
        else if (parser.LookAhead("Center"))
        {
            parser.ReadToken();
            currentArcBall.center = ParseVec3(parser);
			return true;
        }
        else if (parser.LookAhead("Radius"))
        {
            parser.ReadToken();
            currentArcBall.radius = parser.ReadFloat();
			return true;
        }
        else if (parser.LookAhead("Alpha"))
        {
            parser.ReadToken();
            currentArcBall.alpha = parser.ReadFloat();
			return true;
        }
        else if (parser.LookAhead("Beta"))
        {
            parser.ReadToken();
            currentArcBall.beta = parser.ReadFloat();
			return true;
        }
		return false;
	}

	void ArcBallCameraControllerActor::OnLoad()
	{
		Actor::OnLoad();
	
		Engine::Instance()->GetUiEntry()->OnMouseDown.Bind(this, &ArcBallCameraControllerActor::MouseDown);
		Engine::Instance()->GetUiEntry()->OnMouseMove.Bind(this, &ArcBallCameraControllerActor::MouseMove);
		Engine::Instance()->GetUiEntry()->OnMouseUp.Bind(this, &ArcBallCameraControllerActor::MouseUp);
		Engine::Instance()->GetUiEntry()->OnMouseWheel.Bind(this, &ArcBallCameraControllerActor::MouseWheel);
		FindTargetCamera();
		UpdateCamera();
	}

	EngineActorType ArcBallCameraControllerActor::GetEngineType()
	{
		return EngineActorType::UserController;
	}

	bool ArcBallCameraControllerActor::DumpCamera(const CoreLib::String & /*axisName*/, ActionInput /*input*/)
	{
		FindTargetCamera();
		if (targetCamera)
		{
			Print("orientation [%.3f %.3f %.3f]\nposition [%.2f %.2f %.2f]\n", targetCamera->GetYaw(), targetCamera->GetPitch(), targetCamera->GetRoll(),
				targetCamera->GetPosition().x, targetCamera->GetPosition().y, targetCamera->GetPosition().z);
		}
		return true;
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
