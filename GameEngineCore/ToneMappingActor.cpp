#include "ToneMappingActor.h"
#include "CoreLib/LibIO.h"
#include "Engine.h"
namespace GameEngine
{
	void ToneMappingActor::LoadColorLookupTexture(CoreLib::String fileName)
	{
		auto fullFile = Engine::Instance()->FindFile(fileName, ResourceType::Texture);
		if (CoreLib::IO::File::Exists(fullFile))
		{
			CoreLib::IO::BinaryReader reader(new CoreLib::IO::FileStream(fullFile));
			int size = reader.ReadInt32();
			List<int> buffer;
			buffer.SetSize(size*size*size);
			reader.Read(buffer.Buffer(), buffer.Count());
			auto hw = Engine::Instance()->GetRenderer()->GetHardwareRenderer();
			lookupTexture = hw->CreateTexture3D(TextureUsage::Sampled, size, size, size, 1, StorageFormat::RGBA_8);
			lookupTexture->SetData(0, 0, 0, 0, size, size, size, DataType::Byte4, buffer.Buffer());
			Parameters.lookupTexture = lookupTexture.Ptr();
		}
	}
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
		else if (parser.LookAhead("ColorLookup"))
		{
			parser.ReadToken();
			auto fileName = parser.ReadStringLiteral();
			LoadColorLookupTexture(fileName);
			return true;
		}
		return false;
	}
}
