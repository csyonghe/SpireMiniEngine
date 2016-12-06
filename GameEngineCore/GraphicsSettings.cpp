#include "GraphicsSettings.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Tokenizer.h"

using namespace CoreLib;
using namespace CoreLib::IO;
using namespace CoreLib::Text;

namespace GameEngine
{
	void GraphicsSettings::LoadFromFile(CoreLib::String fileName)
	{
		TokenReader parser(File::ReadAllText(fileName));
		while (!parser.IsEnd())
		{
			auto settingsName = parser.ReadWord();
			parser.Read("=");
			auto settingsValue = parser.ReadStringLiteral();
			if (settingsName == "UseDeferredRenderer")
				UseDeferredRenderer = settingsValue == "true";
		}
	}
	void GraphicsSettings::SaveToFile(CoreLib::String fileName)
	{
		StringBuilder sb;
		sb << "UseDeferredRenderer = \"" << (UseDeferredRenderer ? "true" : "false") << "\"\n";

		File::WriteAllText(fileName, sb.ProduceString());
	}
}


