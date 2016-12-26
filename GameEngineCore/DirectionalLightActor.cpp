#include "DirectionalLightActor.h"

namespace GameEngine
{
	bool DirectionalLightActor::ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("Direction"))
		{
			parser.ReadToken();
			Direction = ParseVec3(parser).Normalize();
			return true;
		}
		if (parser.LookAhead("EnableCascadedShadows"))
		{
			parser.ReadToken();
			EnableCascadedShadows = ParseBool(parser);
			return true;
		}
		if (parser.LookAhead("ShadowDistance"))
		{
			parser.ReadToken();
			ShadowDistance = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead("NumShadowCascades"))
		{
			parser.ReadToken();
			NumShadowCascades = parser.ReadInt();
			return true;
		}
		if (parser.LookAhead("TransitionFactor"))
		{
			parser.ReadToken();
			TransitionFactor = (float)parser.ReadDouble();
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


