#ifndef GAME_ENGINE_ARC_BALL_CAMERA_CONTROLLER_H
#define GAME_ENGINE_ARC_BALL_CAMERA_CONTROLLER_H

#include "Actor.h"
#include "InputDispatcher.h"
#include "CoreLib/LibUI/LibUI.h"

namespace GameEngine
{
	class CameraActor;
	enum class MouseState
	{
		None, Translate, Rotate
	};
	class ArcBallParams
	{
	public:
		VectorMath::Vec3 center;
		float radius = 800.0f;
		float alpha = Math::Pi * 0.5f, beta = 0.0f;
		ArcBallParams()
		{
			center.SetZero();
		}
		void GetCoordinates(VectorMath::Vec3 & camPos, VectorMath::Vec3 & up, VectorMath::Vec3 & right, VectorMath::Vec3 & dir);
	};
	class ArcBallCameraControllerActor : public Actor
	{
	private:
		ArcBallParams currentArcBall, lastArcBall;
		int lastX = 0, lastY = 0;
		
		float turnPrecision = CoreLib::Math::Pi / 1000.0f;
		float translatePrecision = 0.001f;
		float zoomScale = 1.1f;
		float minDist = 10.0f;
		float maxDist = 10000.0f;
		bool needAlt = true;
		MouseState state = MouseState::None;

		CoreLib::String targetCameraName;
		CameraActor * targetCamera = nullptr;
		void FindTargetCamera();
		void UpdateCamera();
		void MouseMove(GraphicsUI::UI_Base * sender, GraphicsUI::UIMouseEventArgs & e);
		void MouseDown(GraphicsUI::UI_Base * sender, GraphicsUI::UIMouseEventArgs & e);
		void MouseUp(GraphicsUI::UI_Base * sender, GraphicsUI::UIMouseEventArgs & e);
		void MouseWheel(GraphicsUI::UI_Base * sender, GraphicsUI::UIMouseEventArgs & e);
	public:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid) override;
	public:
		virtual void OnLoad() override;
		virtual EngineActorType GetEngineType() override;
		virtual CoreLib::String GetTypeName() override
		{
			return "ArcBallCameraController";
		}
	public:
		bool DumpCamera(const CoreLib::String & axisName, ActionInput scale);
	};
}

#endif