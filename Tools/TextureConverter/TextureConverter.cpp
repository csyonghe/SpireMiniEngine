// TextureConverter.cpp : Defines the entry point for the console application.
//

#include "TextureCompressor.h"
#include "Graphics/TextureFile.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Imaging/TextureData.h"
#include "Imaging/Bitmap.h"

using namespace CoreLib;
using namespace CoreLib::Graphics;
using namespace CoreLib::Imaging;
using namespace CoreLib::IO;
using namespace GameEngine;

void ConvertTexture(const String & fileName, TextureStorageFormat format, bool useSquish)
{
	if (format == TextureStorageFormat::BC1 || format == TextureStorageFormat::BC5 || format == TextureStorageFormat::BC3)
	{
		Bitmap bmp(fileName);
		List<unsigned int> pixelsInversed;
		int * sourcePixels = (int*)bmp.GetPixels();
		pixelsInversed.SetSize(bmp.GetWidth() * bmp.GetHeight());
		for (int i = 0; i < bmp.GetHeight(); i++)
		{
			for (int j = 0; j < bmp.GetWidth(); j++)
				pixelsInversed[i*bmp.GetWidth() + j] = sourcePixels[(bmp.GetHeight() - 1 - i)*bmp.GetWidth() + j];
		}
		CoreLib::Graphics::TextureFile texFile;
		if (format == TextureStorageFormat::BC1)
			TextureCompressor::CompressRGBA_BC1(texFile, MakeArrayView((unsigned char*)pixelsInversed.Buffer(), pixelsInversed.Count() * 4), bmp.GetWidth(), bmp.GetHeight(), useSquish);
		else if (format == TextureStorageFormat::BC3)
			TextureCompressor::CompressRGBA_BC3(texFile, MakeArrayView((unsigned char*)pixelsInversed.Buffer(), pixelsInversed.Count() * 4), bmp.GetWidth(), bmp.GetHeight(), useSquish);
		else
			TextureCompressor::CompressRG_BC5(texFile, MakeArrayView((unsigned char*)pixelsInversed.Buffer(), pixelsInversed.Count() * 4), bmp.GetWidth(), bmp.GetHeight());
		texFile.SaveToFile(Path::ReplaceExt(fileName, "texture"));
	}
	else
	{
		CoreLib::Graphics::TextureFile texFile;
		BitmapF bmp(fileName);
		CoreLib::Imaging::TextureData<CoreLib::Imaging::Color4F> tex;
		CoreLib::Imaging::CreateTextureDataFromBitmap(tex, bmp);
		CoreLib::Imaging::CreateTextureFile(texFile, format, tex);
		texFile.SaveToFile(Path::ReplaceExt(fileName, "texture"));
	}
}

int wmain(int argc, const wchar_t ** argv)
{
	if (argc > 1)
	{
		bool useSquish = false;
		TextureStorageFormat format = TextureStorageFormat::BC1;
		String fileName = String::FromWString(argv[1]);
		for (int i = 0; i < argc; i++)
		{
			if (String::FromWString(argv[i]) == "-squish")
				useSquish = true;
			if (String::FromWString(argv[i]) == "-bc1")
				format = TextureStorageFormat::BC1;
			if (String::FromWString(argv[i]) == "-bc5")
				format = TextureStorageFormat::BC5;
			if (String::FromWString(argv[i]) == "-bc3")
				format = TextureStorageFormat::BC3;
			if (String::FromWString(argv[i]) == "-r8")
				format = TextureStorageFormat::R8;
			if (String::FromWString(argv[i]) == "-rg8")
				format = TextureStorageFormat::RG8;
			if (String::FromWString(argv[i]) == "-rgb8")
				format = TextureStorageFormat::RGB8;
			if (String::FromWString(argv[i]) == "-rgba8")
				format = TextureStorageFormat::RGBA8;
			if (String::FromWString(argv[i]) == "-rgba32f")
				format = TextureStorageFormat::RGBA_F32;
		}
		ConvertTexture(fileName, format, useSquish);
	}
	else
	{
		printf("Command Format: TextureConverter file_name -format\n");
		printf("Supported formats: bc1, bc5, r8, rg8, rgb8, rgba8, rgba32f\n");
	}
    return 0;
}

