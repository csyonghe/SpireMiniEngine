#include "PointLightActor.h"

namespace GameEngine
{
	bool PointLightActor::ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("Direction"))
		{
			parser.ReadToken();
			Direction = ParseVec3(parser).Normalize();
			return true;
		}
		if (parser.LookAhead("EnableShadows"))
		{
			parser.ReadToken();
			EnableShadows = ParseBool(parser);
			return true;
		}
		if (parser.LookAhead("IsSpotLight"))
		{
			parser.ReadToken();
			IsSpotLight = ParseBool(parser);
			return true;
		}
		if (parser.LookAhead("Radius"))
		{
			parser.ReadToken();
			Radius = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead("SpotLightStartAngle"))
		{
			parser.ReadToken();
			SpotLightStartAngle = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead("SpotLightEndAngle"))
		{
			parser.ReadToken();
			SpotLightEndAngle = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead("DecayDistance90Percent"))
		{
			parser.ReadToken();
			DecayDistance90Percent = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead("Color"))
		{
			parser.ReadToken();
			Color = ParseVec3(parser);
			return true;
		}
		return false;
	}
}