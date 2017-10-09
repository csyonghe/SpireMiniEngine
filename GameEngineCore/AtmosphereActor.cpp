#include "AtmosphereActor.h"

namespace GameEngine
{
	void AtmosphereActor::OnLoad()
	{
		Parameters->SunDir = Parameters->SunDir.Normalize();
	}

}