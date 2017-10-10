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
		Position = value;
		UpdatePosition(*LocalTransform, value);
	}

	void CameraActor::SetYaw(float value)
	{
		Orientation->x = value;
		TransformFromCamera(*LocalTransform, Orientation->x, Orientation->y, Orientation->z, Position.GetValue());
	}
	void CameraActor::SetPitch(float value)
	{
		Orientation->y = value;
		TransformFromCamera(*LocalTransform, Orientation->x, Orientation->y, Orientation->z, Position.GetValue());
	}
	void CameraActor::SetRoll(float value)
	{
		Orientation->z = value;
		TransformFromCamera(*LocalTransform, Orientation->x, Orientation->y, Orientation->z, Position.GetValue());

	}
	void CameraActor::SetOrientation(float pYaw, float pPitch, float pRoll)
	{
		Orientation = Vec3::Create(pYaw, pPitch, pRoll);
		TransformFromCamera(*LocalTransform, pYaw, pPitch, pRoll, Position.GetValue());
	}

	Ray CameraActor::GetRayFromViewCoordinates(float x, float y, float aspect)
	{
		Vec3 camDir, camUp;
		camDir.x = -LocalTransform->m[0][2];
		camDir.y = -LocalTransform->m[1][2];
		camDir.z = -LocalTransform->m[2][2];
		camUp.x = LocalTransform->m[0][1];
		camUp.y = LocalTransform->m[1][1];
		camUp.z = LocalTransform->m[2][1];
		auto right = Vec3::Cross(camDir, camUp).Normalize();
		float zNear = 1.0f;
		auto nearCenter = Position.GetValue() + camDir * zNear;
		auto tanFOV = tan(FOV.GetValue() / 180.0f * (Math::Pi * 0.5f));
		auto nearUpScale = tanFOV * zNear;
		auto nearRightScale = nearUpScale * aspect;
		auto tarPos = nearCenter + right * nearRightScale * (x * 2.0f - 1.0f) + camUp * nearUpScale * (1.0f - y * 2.0f);
		Ray r;
		r.Origin = Position.GetValue();
		r.Dir = tarPos - r.Origin;
		r.Dir = r.Dir.Normalize();
		return r;
	}

	// coordinate range from 0.0 to 1.0

	View CameraActor::GetView()
	{
		View rs;
		rs.FOV = FOV.GetValue();
		rs.Yaw = Orientation->x;
		rs.Pitch = Orientation->y;
		rs.Roll = Orientation->z;
		rs.Position = Position.GetValue();
		rs.ZFar = ZFar.GetValue();
		rs.ZNear = ZNear.GetValue();
		rs.Transform = LocalTransform.GetValue();
		return rs;
	}
	
	CoreLib::Graphics::ViewFrustum CameraActor::GetFrustum(float aspect)
	{
		CoreLib::Graphics::ViewFrustum result;
		result.FOV = FOV.GetValue();
		result.Aspect = aspect;
		result.CamDir.x = -LocalTransform->m[0][2];
		result.CamDir.y = -LocalTransform->m[1][2];
		result.CamDir.z = -LocalTransform->m[2][2];
		result.CamUp.x = LocalTransform->m[0][1];
		result.CamUp.y = LocalTransform->m[1][1];
		result.CamUp.z = LocalTransform->m[2][1];
		result.CamPos = Position.GetValue();
		result.zMin = ZNear.GetValue();
		result.zMax = ZFar.GetValue();
		return result;
	}

	void CameraActor::OnLoad()
	{
		TransformFromCamera(*LocalTransform, Orientation->x, Orientation->y, Orientation->z, Position.GetValue());
	}
	
}
