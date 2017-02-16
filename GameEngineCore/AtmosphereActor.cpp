#include "AtmosphereActor.h"

namespace GameEngine
{
	bool AtmosphereActor::ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("AtmosphericFogScaleFactor"))
		{
			parser.ReadToken();
			Parameters.AtmosphericFogScaleFactor = parser.ReadFloat();
			return true;
		}
		if (parser.LookAhead("SunDir"))
		{
			parser.ReadToken();
			Parameters.SunDir = ParseVec3(parser);
			return true;
		}
		return false;
	}

}

