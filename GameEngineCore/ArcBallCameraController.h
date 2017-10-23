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
		float alpha = CoreLib::Math::Pi * 0.5f, beta = 0.0f;
		ArcBallParams()
		{
			center.SetZero();
		}
		void GetCoordinates(VectorMath::Vec3 & camPos, VectorMath::Vec3 & up, VectorMath::Vec3 & right, VectorMath::Vec3 & dir);
	};
	class ArcBallCameraControllerActor : public Actor
	{
	private:
		ArcBallParams lastArcBall;
		ArcBallParams GetCurrentArcBall();
		int lastX = 0, lastY = 0;
		MouseState state = MouseState::None;
		CameraActor * targetCamera = nullptr;
		void FindTargetCamera();
		void UpdateCamera();
		void MouseMove(GraphicsUI::UI_Base * sender, GraphicsUI::UIMouseEventArgs & e);
		void MouseDown(GraphicsUI::UI_Base * sender, GraphicsUI::UIMouseEventArgs & e);
		void MouseUp(GraphicsUI::UI_Base * sender, GraphicsUI::UIMouseEventArgs & e);
		void MouseWheel(GraphicsUI::UI_Base * sender, GraphicsUI::UIMouseEventArgs & e);
	public:
		PROPERTY_DEF(VectorMath::Vec3, Center, VectorMath::Vec3::Create(0.0f));
		PROPERTY_DEF(float, Radius, 800.0f);
		PROPERTY_DEF(float, Alpha, CoreLib::Math::Pi * 0.5f);
		PROPERTY_DEF(float, Beta, 0.0f);
		PROPERTY_DEF(float, TurnPrecision, CoreLib::Math::Pi / 1000.0f);
		PROPERTY_DEF(float, TranslatePrecision, 0.001f);
		PROPERTY_DEF(float, ZoomScale, 1.1f);
		PROPERTY_DEF(float, MinDist, 10.0f);
		PROPERTY_DEF(float, MaxDist, 10000.0f);
		PROPERTY_DEF(bool, NeedAlt, true);
		PROPERTY(CoreLib::String, TargetCameraName);
	public:
		virtual void OnLoad() override;
		virtual void OnUnload() override;
		virtual EngineActorType GetEngineType() override;
		virtual CoreLib::String GetTypeName() override
		{
			return "ArcBallCameraController";
		}
	public:
		void SetCenter(VectorMath::Vec3 center);
		bool DumpCamera(const CoreLib::String & axisName, ActionInput scale);
	};
}

#endif