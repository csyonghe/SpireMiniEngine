#include "PointLightActor.h"

namespace GameEngine
{
	void PointLightActor::OnLoad()
	{
		Direction = Direction.GetValue().Normalize();
	}

}
