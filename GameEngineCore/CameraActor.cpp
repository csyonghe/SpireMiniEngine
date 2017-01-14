#include "CameraActor.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace VectorMath;

	void UpdatePosition(Matrix4 & rs, const Vec3 & pos)
	{
		rs.values[12] = -(rs.values[0] * pos.x + rs.values[4] * pos.y + rs.values[8] * pos.z);
		rs.values[13] = -(rs.values[1] * pos.x + rs.values[5] * pos.y + rs.values[9] * pos.z);
		rs.values[14] = -(rs.values[2] * pos.x + rs.values[6] * pos.y + rs.values[10] * pos.z);
	}

	void TransformFromCamera(Matrix4 & rs, float yaw, float pitch, float roll, const Vec3 & pos)
	{
		Matrix4 m0, m1, m2, m3;
		Matrix4::RotationY(m0, yaw);
		Matrix4::RotationX(m1, pitch);
		Matrix4::RotationZ(m2, roll);
		Matrix4::Multiply(m3, m2, m1);
		Matrix4::Multiply(rs, m3, m0);

		UpdatePosition(rs, pos);
		rs.values[15] = 1.0f;
	}

	void CameraActor::SetPosition(const VectorMath::Vec3 & value)
	{
		view.Position = value;
		UpdatePosition(localTransform, value);
		view.Transform = localTransform;
	}

	void CameraActor::SetYaw(float value)
	{
		view.Yaw = value;
		TransformFromCamera(localTransform, view.Yaw, view.Pitch, view.Roll, view.Position);
		view.Transform = localTransform;
	}
	void CameraActor::SetPitch(float value)
	{
		view.Pitch = value;
		TransformFromCamera(localTransform, view.Yaw, view.Pitch, view.Roll, view.Position);
		view.Transform = localTransform;
	}
	void CameraActor::SetRoll(float value)
	{
		view.Roll = value;
		TransformFromCamera(localTransform, view.Yaw, view.Pitch, view.Roll, view.Position);
		view.Transform = localTransform;

	}
	void CameraActor::SetOrientation(float pYaw, float pPitch, float pRoll)
	{
		view.Yaw = pYaw;
		view.Pitch = pPitch;
		view.Roll = pRoll;
		TransformFromCamera(localTransform, view.Yaw, view.Pitch, view.Roll, view.Position);
		view.Transform = localTransform;
	}
	
	void CameraActor::SetCollisionRadius(float value)
	{
		collisionRadius = value;
	}
	CoreLib::Graphics::ViewFrustum CameraActor::GetFrustum(float aspect)
	{
		CoreLib::Graphics::ViewFrustum result;
		result.FOV = view.FOV;
		result.Aspect = aspect;
		result.CamDir.x = -localTransform.m[0][2];
		result.CamDir.y = -localTransform.m[1][2];
		result.CamDir.z = -localTransform.m[2][2];
		result.CamUp.x = localTransform.m[0][1];
		result.CamUp.y = localTransform.m[1][1];
		result.CamUp.z = localTransform.m[2][1];
		result.CamPos = view.Position;
		result.zMin = view.ZNear;
		result.zMax = view.ZFar;
		return result;
	}
	bool CameraActor::ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("position"))
		{
			parser.ReadToken();
			view.Position = ParseVec3(parser);
			TransformFromCamera(localTransform, view.Yaw, view.Pitch, view.Roll, view.Position);
			view.Transform = localTransform;
			return true;
		}
		if (parser.LookAhead("orientation"))
		{
			parser.ReadToken();
			auto orientation = ParseVec3(parser);
			view.Yaw = orientation.x;
			view.Pitch = orientation.y;
			view.Roll = orientation.z;
			TransformFromCamera(localTransform, view.Yaw, view.Pitch, view.Roll, view.Position);
			view.Transform = localTransform;
			return true;
		}
		if (parser.LookAhead("znear"))
		{
			parser.ReadToken();
			view.ZNear = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead("zfar"))
		{
			parser.ReadToken();
			view.ZFar = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead("fov"))
		{
			parser.ReadToken();
			view.FOV = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead("radius"))
		{
			parser.ReadToken();
			collisionRadius = (float)parser.ReadDouble();
			return true;
		}
		return false;
	}
}
