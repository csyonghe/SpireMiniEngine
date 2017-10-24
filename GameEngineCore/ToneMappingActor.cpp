#include "ToneMappingActor.h"
#include "CoreLib/LibIO.h"
#include "Engine.h"
namespace GameEngine
{
    void ToneMappingActor::ColorLUT_Changing(CoreLib::String & newFileName)
    {
        if (!LoadColorLookupTexture(newFileName))
            newFileName = "";
    }
    void ToneMappingActor::Exposure_Changed()
    {
        Parameters.Exposure = Exposure.GetValue();
    }
    bool ToneMappingActor::LoadColorLookupTexture(CoreLib::String fileName)
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
            hw->Wait();
			lookupTexture = hw->CreateTexture3D(TextureUsage::Sampled, size, size, size, 1, StorageFormat::RGBA_8);
			lookupTexture->SetData(0, 0, 0, 0, size, size, size, DataType::Byte4, buffer.Buffer());
			Parameters.lookupTexture = lookupTexture.Ptr();
            return true;
		}
        return false;
	}
    void ToneMappingActor::OnLoad()
    {
        Actor::OnLoad();
        Exposure.OnChanged.Bind(this, &ToneMappingActor::Exposure_Changed);
        ColorLUT.OnChanging.Bind(this, &ToneMappingActor::ColorLUT_Changing);
        Exposure_Changed();
        String fileName = ColorLUT.GetValue();
        ColorLUT_Changing(fileName);
    }
    ToneMappingActor::~ToneMappingActor()
    {
        Engine::Instance()->GetRenderer()->Wait();
        lookupTexture = nullptr;
        Engine::Instance()->GetRenderer()->Wait();
    }
}
