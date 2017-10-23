#include "RigMapping.h"
#include "CoreLib/Tokenizer.h"
#include "CoreLib/LibIO.h"

namespace GameEngine
{
	namespace Tools
	{
		using namespace CoreLib;
		using namespace CoreLib::Text;
		RigMappingFile::RigMappingFile(CoreLib::String fileName)
		{
			TokenReader parser(CoreLib::IO::File::ReadAllText(fileName));
			while (!parser.IsEnd())
			{
				if (parser.LookAhead("mesh_rotate"))
				{
					parser.ReadToken();
					RootRotation.x = (float)parser.ReadDouble();
					parser.Read(",");
					RootRotation.y = (float)parser.ReadDouble();
					parser.Read(",");
					RootRotation.z = (float)parser.ReadDouble();
				}
				else if (parser.LookAhead("anim_scale"))
				{
					parser.ReadToken();
					TranslationScale = (float)parser.ReadDouble();
				}
				else if (parser.LookAhead("mapping"))
				{
					parser.ReadToken();
					while (!parser.IsEnd())
					{
						auto src = parser.ReadStringLiteral();
						parser.Read("=");
						auto dest = parser.ReadWord();
						Mapping[src] = dest;
					}
				}
				else
					throw TextFormatException("unknown field");
			}
		}
	}
}
