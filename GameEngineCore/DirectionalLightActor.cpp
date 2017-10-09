#include "DirectionalLightActor.h"

namespace GameEngine
{
	void DirectionalLightActor::OnLoad()
	{
		Direction = Direction.GetValue().Normalize();
	}
}
