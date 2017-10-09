#include "Atmosphere.h"
#include "CoreLib/Tokenizer.h"
#include "Property.h"

namespace GameEngine
{
	void AtmosphereParameters::Parse(CoreLib::Text::TokenReader & parser)
	{
		parser.Read("{");
		while (!parser.IsEnd() && !parser.LookAhead("}"))
		{
			auto word = parser.ReadWord();
			if (word == "AtmosphericFogScaleFactor")
			{
				AtmosphericFogScaleFactor = parser.ReadFloat();
			}
			else if (word == "SunDir")
			{
				SunDir = ParseVec3(parser);
			}
		}
		parser.Read("}");
	}
	void AtmosphereParameters::Serialize(CoreLib::StringBuilder & sb)
	{
		sb << "{\n";
		sb << "SunDir ";
		GameEngine::Serialize(sb, SunDir);
		sb << "\nAtmosphericFogScaleFactor " << AtmosphericFogScaleFactor;
		sb << "\n}";
	}
}
