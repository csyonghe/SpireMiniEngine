#include "ToneMappingActor.h"

namespace GameEngine
{
	bool ToneMappingActor::ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("Exposure"))
		{
			parser.ReadToken();
			Parameters.Exposure = parser.ReadFloat();
			return true;
		}
		return false;
	}
}
