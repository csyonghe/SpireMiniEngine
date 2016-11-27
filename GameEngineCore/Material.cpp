#include "Material.h"
#include "CoreLib/LibIO.h"

namespace GameEngine
{
	void Material::Parse(CoreLib::Text::TokenReader & parser)
	{
		parser.Read("material");
		parser.Read("{");
		while (!parser.IsEnd() && !parser.LookAhead("}"))
		{
			if (parser.LookAhead("shader"))
			{
				parser.ReadToken();
				ShaderFile = parser.ReadStringLiteral();
			}
			else if (parser.LookAhead("var"))
			{
				parser.ReadToken();
				auto name = parser.ReadWord();
				parser.Read("=");
				auto val = DynamicVariable::Parse(parser);
				Variables[name] = val;
			}
		}
		parser.Read("}");
	}

	void Material::LoadFromFile(const CoreLib::String & fullFileName)
	{
		CoreLib::Text::TokenReader parser(CoreLib::IO::File::ReadAllText(fullFileName));
		Parse(parser);
	}
}

