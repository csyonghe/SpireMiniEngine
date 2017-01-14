#ifndef GAME_ENGINE_VIEW_H
#define GAME_ENGINE_VIEW_H

#include "CoreLib/VectorMath.h"
#include "CoreLib/Graphics/ViewFrustum.h"

namespace GameEngine
{
	class View
	{
	public:
		float Yaw = 0.0f, Pitch = 0.0f, Roll = 0.0f;
		VectorMath::Vec3 Position;
		float ZNear = 40.0f, ZFar = 400000.0f;
		float FOV = 75.0f;
		View()
		{
			Position.SetZero();
		}
		VectorMath::Matrix4 Transform;
		VectorMath::Vec3 GetDirection() const
		{
			return VectorMath::Vec3::Create(-Transform.m[0][2], -Transform.m[1][2], -Transform.m[2][2]);
		}
		CoreLib::Graphics::ViewFrustum GetFrustum(float aspect) const
		{
			CoreLib::Graphics::ViewFrustum result;
			result.FOV = FOV;
			result.Aspect = aspect;
			result.CamDir.x = -Transform.m[0][2];
			result.CamDir.y = -Transform.m[1][2];
			result.CamDir.z = -Transform.m[2][2];
			result.CamUp.x = Transform.m[0][1];
			result.CamUp.y = Transform.m[1][1];
			result.CamUp.z = Transform.m[2][1];
			result.CamPos = Position;
			result.zMin = ZNear;
			result.zMax = ZFar;
			return result;
		}
	};
}

#endif