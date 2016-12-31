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
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::List<char>> Shaders;
		CoreLib::List<ShaderCompilationError> Diagnostics;
		CoreLib::EnumerableDictionary<CoreLib::String, DescriptorSetInfo> BindingLayouts;
		void LoadFromFile(CoreLib::String fileName);
		void SaveToFile(CoreLib::String fileName, bool codeIsText);
	};

	void GetShaderCompilationResult(ShaderCompilationResult & src, SpireCompilationResult * compileResult, SpireDiagnosticSink * diagSink);

	bool CompileShader(ShaderCompilationResult & src,
		SpireCompilationContext * spireCtx,
		int targetLang,
		const CoreLib::String & filename);
}

#endif