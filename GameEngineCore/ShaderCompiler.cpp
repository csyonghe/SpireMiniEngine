#include "ShaderCompiler.h"
#include "CoreLib/LibIO.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace CoreLib;
	using namespace CoreLib::IO;

	void GetShaderCompilationResult(ShaderCompilationResult & src, SpireCompilationResult * compileResult, SpireDiagnosticSink * diagSink)
	{
		int count = spGetDiagnosticCount(diagSink);
		for (int i = 0; i < count; i++)
		{
			SpireDiagnostic diag;
			spGetDiagnosticByIndex(diagSink, i, &diag);
			src.Diagnostics.Add(ShaderCompilationError(diag));
			Print("%S(%d): %S\n", String(diag.FileName).ToWString(), diag.Line, String(diag.Message).ToWString());
		}
		if (spDiagnosticSinkHasAnyErrors(diagSink))
			return;

		int bufferSize = spGetCompiledShaderNames(compileResult, nullptr, 0);
		List<char> shaderNameBuffer;
		shaderNameBuffer.SetSize(bufferSize);
		spGetCompiledShaderNames(compileResult, shaderNameBuffer.Buffer(), shaderNameBuffer.Count());
		shaderNameBuffer.Add(0);
		auto shaderName = shaderNameBuffer.Buffer();
		bufferSize = spGetCompiledShaderStageNames(compileResult, shaderName, nullptr, 0);
		List<char> buffer;
		buffer.SetSize(bufferSize);
		spGetCompiledShaderStageNames(compileResult, shaderNameBuffer.Buffer(), buffer.Buffer(), bufferSize);

		for (auto stage : CoreLib::Text::Split(buffer.Buffer(), L'\n'))
		{
			List<char> codeBytes;
			int len = 0;
			auto code = spGetShaderStageSource(compileResult, shaderNameBuffer.Buffer(), stage.Buffer(), &len);
			codeBytes.SetSize(len);
			memcpy(codeBytes.Buffer(), code, codeBytes.Count());
			src.Shaders.AddIfNotExists(stage, codeBytes);
		}

		int descSetCount = spGetShaderParameterSetCount(compileResult, shaderNameBuffer.Buffer());
		for (int i = 0; i < descSetCount; i++)
		{
			auto descSet = spGetShaderParameterSet(compileResult, shaderNameBuffer.Buffer(), i);
			DescriptorSetInfo setInfo;
			setInfo.BindingName = spParameterSetGetBindingName(descSet);
			setInfo.BindingPoint = spParameterSetGetBindingIndex(descSet);
			if (spParameterSetGetBufferSize(descSet) != 0)
				setInfo.Descriptors.Add(DescriptorLayout(sfGraphics, 0, BindingType::UniformBuffer, spParameterSetGetUniformBufferLegacyBindingPoint(descSet)));
			int numDescs = spParameterSetGetBindingSlotCount(descSet);
			for (int j = 0; j < numDescs; j++)
			{
				auto slot = spParameterSetGetBindingSlot(descSet, j);
				DescriptorLayout desc;
				if (slot->NumLegacyBindingPoints)
					desc.LegacyBindingPoints.AddRange(slot->LegacyBindingPoints, slot->NumLegacyBindingPoints);
				desc.Location = setInfo.Descriptors.Count();
				switch (slot->Type)
				{
				case SPIRE_SAMPLER:
					desc.Type = BindingType::Sampler;
					break;
				case SPIRE_TEXTURE:
					desc.Type = BindingType::Texture;
					break;
				case SPIRE_UNIFORM_BUFFER:
					desc.Type = BindingType::UniformBuffer;
					break;
				case SPIRE_STORAGE_BUFFER:
					desc.Type = BindingType::StorageBuffer;
					break;
				}
				setInfo.Descriptors.Add(_Move(desc));
			}
			src.BindingLayouts.Add(String(setInfo.BindingName), _Move(setInfo));
		}
	}

	bool CompileShader(ShaderCompilationResult & src,
		SpireCompilationContext * spireCtx,
		int targetLang,
		const String & filename)
	{
		auto actualFilename = Engine::Instance()->FindFile(filename, ResourceType::Shader);
		if (!actualFilename.Length())
			return false;

		auto cachePostfix = (targetLang == SPIRE_GLSL) ? "glsl.cse" : (targetLang == SPIRE_HLSL) ? "hlsl.cse" : "spv.cse";
		// Check disk for shaders
		auto cachedShaderFilename =
			Path::Combine(Engine::Instance()->GetDirectory(false, ResourceType::ShaderCache),
				Path::GetFileName(Path::ReplaceExt(actualFilename, cachePostfix)));

		//if (!Engine::Instance()->RecompileShaders && CoreLib::IO::File::Exists(cachedShaderFilename))
		//{
		//	src.LoadFromFile(cachedShaderFilename);
		//	return true;
		//}

		// Compile shader using Spire
		auto diagSink = spCreateDiagnosticSink(spireCtx);
		spSetCodeGenTarget(spireCtx, targetLang);
		String shaderSrc;
		SpireCompilationResult * compileResult;

		shaderSrc = File::ReadAllText(actualFilename);
		spSetShaderToCompile(spireCtx, "");
		compileResult = spCompileShaderFromSource(spireCtx, shaderSrc.Buffer(), actualFilename.Buffer(), diagSink);

		if (spDiagnosticSinkHasAnyErrors(diagSink))
		{
			auto outputFileName = Path::ReplaceExt(actualFilename, "spire");
			Print("Spire source written to %S\n", outputFileName.ToWString());
			File::WriteAllText(outputFileName, shaderSrc);
			for (int i = 0; i < spGetDiagnosticCount(diagSink); i++)
			{
				SpireDiagnostic diag;
				spGetDiagnosticByIndex(diagSink, i, &diag);
				Print("%S\n", String(diag.Message).ToWString());
			}
			spDestroyDiagnosticSink(diagSink);
			spDestroyCompilationResult(compileResult);

			return false;
		}

		GetShaderCompilationResult(src, compileResult, diagSink);
		spDestroyDiagnosticSink(diagSink);
		spDestroyCompilationResult(compileResult);

		src.SaveToFile(cachedShaderFilename, targetLang != SPIRE_SPIRV);
		return true;
	}


	String IndentString(String src)
	{
		StringBuilder  sb;
		int indent = 0;
		bool beginTrim = true;
		for (int c = 0; c < src.Length(); c++)
		{
			auto ch = src[c];
			if (ch == '\n')
			{
				sb << "\n";

				beginTrim = true;
			}
			else
			{
				if (beginTrim)
				{
					while (c < src.Length() - 1 && (src[c] == L'\t' || src[c] == L'\n' || src[c] == L'\r' || src[c] == L' '))
					{
						c++;
						ch = src[c];
					}
					for (int i = 0; i < indent - 1; i++)
						sb << '\t';
					if (ch != '}' && indent > 0)
						sb << '\t';
					beginTrim = false;
				}

				if (ch == '{')
					indent++;
				else if (ch == '}')
					indent--;
				if (indent < 0)
					indent = 0;

				sb << ch;
			}
		}
		return sb.ProduceString();
	}

	void ShaderCompilationResult::LoadFromFile(String fileName)
	{
		auto src = File::ReadAllText(fileName);
		CoreLib::Text::TokenReader parser(src);
		auto getShaderSource = [&]()
		{
			auto token = parser.ReadToken();
			int endPos = token.Position.Pos + 1;
			int brace = 0;
			while (endPos < src.Length() && !(src[endPos] == L'}' && brace == 0))
			{
				if (src[endPos] == L'{')
					brace++;
				else if (src[endPos] == L'}')
					brace--;
				endPos++;
			}
			while (!parser.IsEnd() && parser.NextToken().Position.Pos != endPos)
				parser.ReadToken();
			parser.ReadToken();
			return src.SubString(token.Position.Pos + 1, endPos - token.Position.Pos - 1);
		};
		while (!parser.IsEnd())
		{
			auto stageName = parser.ReadWord();
			if (parser.LookAhead("binary"))
			{
				parser.ReadToken();
				parser.Read("{");
				while (!parser.LookAhead("}") && !parser.IsEnd())
				{
					auto val = parser.ReadUInt();
					List<char> code;
					code.AddRange((char*)&val, sizeof(unsigned int));
					this->Shaders[stageName] = code;
				}
				parser.Read("}");
			}
			if (parser.LookAhead("text"))
			{
				parser.ReadToken();
				List<char> code;
				auto codeStr = getShaderSource();
				int len = (int)strlen(codeStr.Buffer());
				code.SetSize(len);
				memcpy(code.Buffer(), codeStr.Buffer(), len);
				code.Add(0);
				Shaders[stageName] = code;
			}
		}
	}

	void ShaderCompilationResult::SaveToFile(String fileName, bool codeIsText)
	{
		StringBuilder sb;
		for (auto & code : Shaders)
		{
			sb << code.Key;
			if (codeIsText)
			{
				sb << "\ntext\n{\n";
				sb << (char *)code.Value.Buffer() << "\n}\n";
			}
			else
			{
				sb << "\nbinary\n{\n";
				for (auto i = 0; i < code.Value.Count(); i++)
				{
					sb << (int)code.Value[i] << " ";
					if (i % 12 == 0) sb << "\n";
				}
				sb << "\n}\n";
			}
		}
		File::WriteAllText(fileName, IndentString(sb.ProduceString()));
	}
}