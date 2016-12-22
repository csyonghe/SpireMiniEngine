#ifndef GAME_ENGINE_SHADER_COMPILER_H
#define GAME_ENGINE_SHADER_COMPILER_H

#include "Spire/Spire.h"
#include "CoreLib/Basic.h"
#include "HardwareRenderer.h"

namespace GameEngine
{
	class ShaderCompilationError
	{
	public:
		CoreLib::String Message;
		int ErrorId;
		CoreLib::String FileName;
		int Line, Col;
		ShaderCompilationError() {}
		ShaderCompilationError(const SpireDiagnostic & msg)
		{
			Message = msg.Message;
			ErrorId = msg.ErrorId;
			FileName = msg.FileName;
			Line = msg.Line;
			Col = msg.Col;
		}
	};

	struct DescriptorSetInfo
	{
		CoreLib::List<DescriptorLayout> Descriptors;
		int BindingPoint;
		CoreLib::String BindingName;
	};

	class ShaderCompilationResult
	{
	public:
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::List<unsigned char>> Shaders;
		CoreLib::List<ShaderCompilationError> Diagnostics;
		CoreLib::EnumerableDictionary<CoreLib::String, DescriptorSetInfo> BindingLayouts;
		void LoadFromFile(CoreLib::String fileName);
		void SaveToFile(CoreLib::String fileName, bool codeIsText);
	};

	bool CompileShader(ShaderCompilationResult & src,
		SpireCompilationContext * spireCtx,
		int targetLang,
		const CoreLib::String & filename,
		const CoreLib::String & entryPoint);
}

#endif