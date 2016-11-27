#include "HardwareApiFactory.h"
#include "HardwareRenderer.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Tokenizer.h"
#include "CoreLib/WinForm/Debug.h"

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
		const CoreLib::String & meshProcessingDef,
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
		spAddSearchPath(spireCtx, searchDir.Buffer());
		spSetCodeGenTarget(spireCtx, targetLang);
		spSetShaderToCompile(spireCtx, symbol.Buffer());

		String shaderSrc;
		SpireCompilationResult * compileResult;
		if (actualFilename.Length())
		{
			spLoadModuleLibraryFromSource(spireCtx, pipelineDef.Buffer(), "pipeline_def");
			auto fileContent = CoreLib::IO::File::ReadAllText(actualFilename) + vertexDef + meshProcessingDef;
			compileResult = spCompileShaderFromSource(spireCtx, fileContent.Buffer(), actualFilename.Buffer());
			shaderSrc = pipelineDef + fileContent;
		}
		else
		{
			shaderSrc = defaultShader + vertexDef + meshProcessingDef;
			compileResult = spCompileShaderFromSource(spireCtx, shaderSrc.Buffer(), "default_shader");
		}

		int warningCount = spGetMessageCount(compileResult, SPIRE_WARNING);
		int errorCount = spGetMessageCount(compileResult, SPIRE_ERROR);
		if (warningCount > 0)
		{
			for (int i = 0; i < warningCount; i++)
			{
				SpireErrorMessage msg;
				spGetMessageContent(compileResult, SPIRE_WARNING, i, &msg);
				src.Warnings.Add(ShaderCompilationError(msg));
			}
		}
		CoreLib::IO::File::WriteAllText("debugSpireShader.spire", shaderSrc);
		if (errorCount > 0)
		{
			for (int i = 0; i < errorCount; i++)
			{
				SpireErrorMessage msg;
				spGetMessageContent(compileResult, SPIRE_ERROR, i, &msg);
				src.Errors.Add(ShaderCompilationError(msg));
				CoreLib::Diagnostics::Debug::WriteLine(String(msg.FileName) + "(" + msg.Line + "): " + String(msg.Message));
			}
		}

		if (errorCount > 0)
		{
			spDestroyCompilationResult(compileResult);
			spDestroyCompilationContext(spireCtx);
			return false;
		}
		int bufferSize = spGetCompiledShaderStageNames(compileResult, symbol.Buffer(), nullptr, 0);
		List<char> buffer;
		buffer.SetSize(bufferSize);
		spGetCompiledShaderStageNames(compileResult, symbol.Buffer(), buffer.Buffer(), bufferSize);

		for (auto stage : CoreLib::Text::Split(buffer.Buffer(), L'\n'))
		{
			List<unsigned char> codeBytes;
			int len = 0;
			auto code = spGetShaderStageSource(compileResult, symbol.Buffer(), stage.Buffer(), &len);
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

		virtual bool CompileShader(ShaderCompilationResult & src, const CoreLib::String & filename, const CoreLib::String & vertexDef, const CoreLib::String & symbol) override
		{
			return GameEngine::CompileShader(src, SPIRE_GLSL, engineShaderDir, filename, pipelineShaderDef, defaultShader, vertexDef, meshProcessingDef, symbol);
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

		virtual bool CompileShader(ShaderCompilationResult & src, const CoreLib::String & filename, const CoreLib::String & vertexDef, const CoreLib::String & symbol) override
		{
			return GameEngine::CompileShader(src, SPIRE_SPIRV, engineShaderDir, filename, pipelineShaderDef, defaultShader, vertexDef, meshProcessingDef, symbol);
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
		pipelineShaderDef = "\n#file " + CoreLib::Text::EscapeStringLiteral(pipelineDefFile) + "\n" + CoreLib::IO::File::ReadAllText(pipelineDefFile);
		auto utilDefFile = Engine::Instance()->FindFile("Utils.shader", ResourceType::Shader);
		if (!utilDefFile.Length())
			throw InvalidOperationException("'Utils.shader' not found. Engine directory is not setup correctly.");
		auto utilsDef = CoreLib::IO::File::ReadAllText(utilDefFile);
		pipelineShaderDef = pipelineShaderDef + "\n#file " + CoreLib::Text::EscapeStringLiteral(utilDefFile) + "\n" + utilsDef;

		auto defaultShaderFile = Engine::Instance()->FindFile("DefaultPattern.shader", ResourceType::Shader);
		if (!defaultShaderFile.Length())
			throw InvalidOperationException("'DefaultPattern.shader' not found. Engine directory is not setup correctly.");
		defaultShader = pipelineShaderDef + "\n#file " + CoreLib::Text::EscapeStringLiteral(defaultShaderFile) + "\n" + CoreLib::IO::File::ReadAllText(defaultShaderFile);

		auto meshProcessingFile = Engine::Instance()->FindFile("MeshProcessing.shader", ResourceType::Shader);
		if (!meshProcessingFile.Length())
			throw InvalidOperationException("'MeshProcessing.shader' not found. Engine directory is not setup correctly.");
		meshProcessingDef = "\n#file " + CoreLib::Text::EscapeStringLiteral(meshProcessingFile) + "\n" + CoreLib::IO::File::ReadAllText(meshProcessingFile);
	}
}