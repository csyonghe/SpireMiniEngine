#include "LightActor.h"
using namespace VectorMath;
using namespace CoreLib;

namespace GameEngine
{
	Vec3 LightActor::GetDirection()
	{
		auto localTransform = LocalTransform.GetValue();
		return Vec3::Create(localTransform.m[2][0], localTransform.m[2][1], localTransform.m[2][2]);
	}
	bool LightActor::ParseField(CoreLib::String fieldName, CoreLib::Text::TokenReader & parser)
	{
		if (Actor::ParseField(fieldName, parser))
			return true;
		if (fieldName.ToLower() == "direction")
		{
			auto dir = ParseVec3(parser).Normalize();
			Vec3 x;
			GetOrthoVec(x, dir);
			Vec3 y;
			y = Vec3::Cross(dir, x);
			Matrix4 localTrans;
			localTrans = LocalTransform.GetValue();
			localTrans.m[0][0] = x.x;
			localTrans.m[0][1] = x.y;
			localTrans.m[0][2] = x.z;
			localTrans.m[1][0] = y.x;
			localTrans.m[1][1] = y.y;
			localTrans.m[1][2] = y.z;
			localTrans.m[2][0] = dir.x;
			localTrans.m[2][1] = dir.y;
			localTrans.m[2][2] = dir.z;
			LocalTransform = localTrans;
			return true;
		}
		return false;
	}
}

