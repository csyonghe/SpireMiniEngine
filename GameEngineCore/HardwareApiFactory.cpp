#include "HardwareApiFactory.h"
#include "HardwareRenderer.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Tokenizer.h"
#include "CoreLib/WinForm/Debug.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace CoreLib;

	bool CompileShader(ShaderCompilationResult & src, 
		int targetLang,
		const CoreLib::String & searchDir,
		const CoreLib::String & filename, 
		const CoreLib::String & pipelineDef,
		const CoreLib::String & defaultShader,
		const CoreLib::String & vertexDef, 
		const CoreLib::String & entryPoint,
		const CoreLib::String & symbol)
	{
		auto actualFilename = Engine::Instance()->FindFile(filename, ResourceType::Shader);
		auto cachePostfix = (targetLang == SPIRE_GLSL) ? "_glsl" : (targetLang == SPIRE_HLSL) ? "_hlsl" : "_spv";
		// Check disk for shaders
		auto cachedShaderFilename =
			CoreLib::IO::Path::Combine(CoreLib::IO::Path::GetDirectoryName(actualFilename),
				CoreLib::IO::Path::GetFileNameWithoutEXT(actualFilename) + "_" + symbol + cachePostfix + ".cse");

		/*if (CoreLib::IO::File::Exists(cachedShaderFilename))
		{
			src.LoadFromFile(cachedShaderFilename);
			return true;
		}*/

		// Compile shader using Spire

		auto spireCtx = spCreateCompilationContext(nullptr);
		auto diagSink = spCreateDiagnosticSink(spireCtx);
		spAddSearchPath(spireCtx, searchDir.Buffer());
		spSetCodeGenTarget(spireCtx, targetLang);
		spSetShaderToCompile(spireCtx, symbol.Buffer());

		String shaderSrc;
		SpireCompilationResult * compileResult;
		if (actualFilename.Length())
		{
			spLoadModuleLibraryFromSource(spireCtx, pipelineDef.Buffer(), "pipeline_def", diagSink);
			auto fileContent = CoreLib::IO::File::ReadAllText(actualFilename) + vertexDef + entryPoint;
			compileResult = spCompileShaderFromSource(spireCtx, fileContent.Buffer(), actualFilename.Buffer(), diagSink);
			shaderSrc = pipelineDef + fileContent;
		}
		else
		{
			shaderSrc = defaultShader + vertexDef + entryPoint;
			compileResult = spCompileShaderFromSource(spireCtx, shaderSrc.Buffer(), "default_shader", diagSink);
		}

		int count = spGetDiagnosticCount(diagSink);
		for (int i = 0; i < count; i++)
		{
			SpireDiagnostic diag;
			spGetDiagnosticByIndex(diagSink, i, &diag);
			src.Diagnostics.Add(ShaderCompilationError(diag));
			CoreLib::Diagnostics::Debug::WriteLine(String(diag.FileName) + "(" + diag.Line + "): " + String(diag.Message));
		}

		if (spDiagnosticSinkHasAnyErrors(diagSink))
		{
			spDestroyDiagnosticSink(diagSink);
			spDestroyCompilationResult(compileResult);
			spDestroyCompilationContext(spireCtx);
			return false;
		}

		spDestroyDiagnosticSink(diagSink);
		
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
			List<unsigned char> codeBytes;
			int len = 0;
			auto code = spGetShaderStageSource(compileResult, shaderNameBuffer.Buffer(), stage.Buffer(), &len);
			codeBytes.SetSize(len);
			memcpy(codeBytes.Buffer(), code, codeBytes.Count());
			src.Shaders.AddIfNotExists(stage, codeBytes);
		}

		spDestroyCompilationResult(compileResult);
		spDestroyCompilationContext(spireCtx);

		src.SaveToFile(cachedShaderFilename, targetLang != SPIRE_SPIRV);
		return true;
	}

	class OpenGLFactory : public HardwareApiFactory
	{
	public:
		virtual HardwareRenderer * CreateRenderer(int /*gpuId*/) override
		{
			LoadShaderLibrary();
			return CreateGLHardwareRenderer();
		}

		virtual bool CompileShader(ShaderCompilationResult & src, const CoreLib::String & filename, const CoreLib::String & vertexDef, const CoreLib::String & entryPoint, const CoreLib::String & symbol) override
		{
			return GameEngine::CompileShader(src, SPIRE_GLSL, engineShaderDir, filename, pipelineShaderDef, defaultShader, vertexDef, entryPoint, symbol);
		}
	};

	class VulkanFactory : public HardwareApiFactory
	{
	private:
		CoreLib::String pipelineShaderDef;
		CoreLib::String defaultShader;
		CoreLib::String engineShaderDir;
	public:
		virtual HardwareRenderer * CreateRenderer(int gpuId) override
		{
			LoadShaderLibrary();
			return CreateVulkanHardwareRenderer(gpuId);
		}

		virtual bool CompileShader(ShaderCompilationResult & src, const CoreLib::String & filename, const CoreLib::String & vertexDef, const CoreLib::String & entryPoint, const CoreLib::String & symbol) override
		{
			return GameEngine::CompileShader(src, SPIRE_SPIRV, engineShaderDir, filename, pipelineShaderDef, defaultShader, vertexDef, entryPoint, symbol);
		}
	};

	HardwareApiFactory * CreateOpenGLFactory()
	{
		return new OpenGLFactory();
	}

	HardwareApiFactory * CreateVulkanFactory()
	{
		return new VulkanFactory();
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
		auto src = CoreLib::IO::File::ReadAllText(fileName);
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
					List<unsigned char> code;
					code.AddRange((unsigned char*)&val, sizeof(unsigned int));
					this->Shaders[stageName] = code;
				}
				parser.Read("}");
			}
			if (parser.LookAhead("text"))
			{
				parser.ReadToken();
				List<unsigned char> code;
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
		CoreLib::IO::File::WriteAllText(fileName, IndentString(sb.ProduceString()));
	}
	void HardwareApiFactory::LoadShaderLibrary()
	{
		// Load OpenGL shader pipeline
		auto pipelineDefFile = Engine::Instance()->FindFile("GlPipeline.shader", ResourceType::Shader);
		if (!pipelineDefFile.Length())
			throw InvalidOperationException("'Pipeline.shader' not found. Engine directory is not setup correctly.");
		engineShaderDir = CoreLib::IO::Path::GetDirectoryName(pipelineDefFile);
		pipelineShaderDef = "\n" + CoreLib::IO::File::ReadAllText(pipelineDefFile);
		auto utilDefFile = Engine::Instance()->FindFile("Utils.shader", ResourceType::Shader);
		if (!utilDefFile.Length())
			throw InvalidOperationException("'Utils.shader' not found. Engine directory is not setup correctly.");
		auto utilsDef = CoreLib::IO::File::ReadAllText(utilDefFile);
		pipelineShaderDef = pipelineShaderDef + "\n" + utilsDef;

		auto defaultShaderFile = Engine::Instance()->FindFile("DefaultPattern.shader", ResourceType::Shader);
		if (!defaultShaderFile.Length())
			throw InvalidOperationException("'DefaultPattern.shader' not found. Engine directory is not setup correctly.");
		defaultShader = pipelineShaderDef + "\n" + CoreLib::IO::File::ReadAllText(defaultShaderFile);

		auto meshProcessingFile = Engine::Instance()->FindFile("MeshProcessing.shader", ResourceType::Shader);
		if (!meshProcessingFile.Length())
			throw InvalidOperationException("'MeshProcessing.shader' not found. Engine directory is not setup correctly.");
		meshProcessingDef = "\n" + CoreLib::IO::File::ReadAllText(meshProcessingFile);
	}
}