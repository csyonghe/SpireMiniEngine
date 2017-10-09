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
	bool ToneMappingActor::ParseField(CoreLib::String fieldName, CoreLib::Text::TokenReader & parser)
	{
		if (Actor::ParseField(fieldName, parser))
			return true;
		if (fieldName  == "Exposure")
		{
			Parameters.Exposure = parser.ReadFloat();
			return true;
		}
		else if (fieldName == "ColorLookup")
		{
			lookupTextureFileName = parser.ReadStringLiteral();
			LoadColorLookupTexture(lookupTextureFileName);
			return true;
		}
		return false;
	}
	void ToneMappingActor::SerializeFields(CoreLib::StringBuilder & sb)
	{
		sb << "Exposure " << Parameters.Exposure << "\n";
		if (lookupTextureFileName.Length())
			sb << "ColorLookup " << CoreLib::Text::EscapeStringLiteral(lookupTextureFileName) << "\n";
	}
}
