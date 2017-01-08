#include "PipelineContext.h"
#include "Mesh.h"
#include "Engine.h"
#include "CoreLib/LibIO.h"
#include "ShaderCompiler.h"
#include "EngineLimits.h"
#include "Renderer.h"

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
	VertexFormat PipelineContext::LoadVertexFormat(MeshVertexFormat vertFormat)
	{
		VertexFormat rs;
		auto vertTypeId = vertFormat.GetTypeId();
		if (vertexFormats.TryGetValue(vertTypeId, rs))
			return rs;
		VertexFormat vertexFormat;
		int location = 0;

		const int UNNORMALIZED = 0;
		const int NORMALIZED = 1;

		// Always starts with vec3 pos
		rs.Attributes.Add(VertexAttributeDesc(DataType::Float3, UNNORMALIZED, 0, location));
		location++;

		for (int i = 0; i < vertFormat.GetUVChannelCount(); i++)
		{
			rs.Attributes.Add(VertexAttributeDesc(DataType::Half2, UNNORMALIZED, vertFormat.GetUVOffset(i), location));
			location++;
		}
		if (vertFormat.HasTangent())
		{
			rs.Attributes.Add(VertexAttributeDesc(DataType::UInt, UNNORMALIZED, vertFormat.GetTangentFrameOffset(), location));
			location++;
		}
		for (int i = 0; i < vertFormat.GetColorChannelCount(); i++)
		{
			rs.Attributes.Add(VertexAttributeDesc(DataType::Byte4, NORMALIZED, vertFormat.GetColorOffset(i), location));
			location++;
		}
		if (vertFormat.HasSkinning())
		{
			rs.Attributes.Add(VertexAttributeDesc(DataType::UInt, UNNORMALIZED, vertFormat.GetBoneIdsOffset(), location));
			location++;
			rs.Attributes.Add(VertexAttributeDesc(DataType::UInt, UNNORMALIZED, vertFormat.GetBoneWeightsOffset(), location));
			location++;
		}

		vertexFormats[vertTypeId] = rs;
		return rs;
	}

	PipelineClass * PipelineContext::GetPipelineInternal(MeshVertexFormat * vertFormat, int vtxId)
	{
		shaderKeyChanged = false;
		lastVtxId = vtxId;

		shaderKeyBuilder.Clear();
		shaderKeyBuilder.Append(spShaderGetId(shader));
		shaderKeyBuilder.FlipLeadingByte(vtxId);
		for (int i = 0; i < modulePtr; i++)
			shaderKeyBuilder.Append(modules[i]->ModuleId);
		/*
		if (shaderKeyBuilder.Key == lastKey)
		{
		return lastPipeline;
		}*/
		if (auto pipeline = pipelineObjects.TryGetValue(shaderKeyBuilder.Key))
		{
			//lastKey = shaderKeyBuilder.Key;
			lastPipeline = pipeline->Ptr();
			return pipeline->Ptr();
		}
		//lastKey = shaderKeyBuilder.Key;
		lastPipeline = CreatePipeline(vertFormat);
		return lastPipeline;
	}

	PipelineClass * PipelineContext::CreatePipeline(MeshVertexFormat * vertFormat)
	{
		RefPtr<PipelineBuilder> pipelineBuilder = hwRenderer->CreatePipelineBuilder();

		pipelineBuilder->FixedFunctionStates = *fixedFunctionStates;

		// Set vertex layout
		pipelineBuilder->SetVertexLayout(LoadVertexFormat(*vertFormat));

		// Compile shaders
		Array<SpireModule*, 32> spireModules;
		for (int i = 0; i < modulePtr; i++)
			spireModules.Add(modules[i]->specializedModule);
		SpireDiagnosticSink * sink = spCreateDiagnosticSink(spireContext);
		auto compileRs = spCompileShader(spireContext, shader, spireModules.Buffer(), spireModules.Count(), vertFormat->GetShaderDefinition().Buffer(), sink);

		int count = spGetDiagnosticCount(sink);
		for (int i = 0; i < count; i++)
		{
			SpireDiagnostic diag;
			spGetDiagnosticByIndex(sink, i, &diag);
			Print("%S(%d): %S\n", String(diag.FileName).ToWString(), diag.Line, String(diag.Message).ToWString());
		}

		if (spDiagnosticSinkHasAnyErrors(sink))
		{
			spDestroyDiagnosticSink(sink);
			spDestroyCompilationResult(compileRs);
			return nullptr;
		}

		ShaderCompilationResult rs;
		GetShaderCompilationResult(rs, compileRs, sink);

		spDestroyDiagnosticSink(sink);
		spDestroyCompilationResult(compileRs);

		RefPtr<PipelineClass> pipelineClass = new PipelineClass();

		for (auto& compiledShader : rs.Shaders)
		{
			Shader* shaderObj = nullptr;
			if (compiledShader.Key == "vs")
			{
				shaderObj = hwRenderer->CreateShader(ShaderType::VertexShader, compiledShader.Value.Buffer(), compiledShader.Value.Count());
			}
			else if (compiledShader.Key == "fs")
			{
				shaderObj = hwRenderer->CreateShader(ShaderType::FragmentShader, compiledShader.Value.Buffer(), compiledShader.Value.Count());
			}
			else if (compiledShader.Key == "tcs")
			{
				shaderObj = hwRenderer->CreateShader(ShaderType::HullShader, compiledShader.Value.Buffer(), compiledShader.Value.Count());
			}
			else if (compiledShader.Key == "tes")
			{
				shaderObj = hwRenderer->CreateShader(ShaderType::DomainShader, compiledShader.Value.Buffer(), compiledShader.Value.Count());
			}
			pipelineClass->shaders.Add(shaderObj);
		}

		//String keyStr = key;
		//pipelineBuilder->SetDebugName(keyStr);

		pipelineBuilder->SetShaders(From(pipelineClass->shaders).Select([](const RefPtr<Shader>& s) {return s.Ptr(); }).ToList().GetArrayView());
		List<RefPtr<DescriptorSetLayout>> descSetLayouts;
		for (auto & descSet : rs.BindingLayouts)
		{
			auto layout = hwRenderer->CreateDescriptorSetLayout(descSet.Value.Descriptors.GetArrayView());
			if (descSet.Value.BindingPoint >= descSetLayouts.Count())
				descSetLayouts.SetSize(descSet.Value.BindingPoint + 1);
			descSetLayouts[descSet.Value.BindingPoint] = layout;

		}
		pipelineBuilder->SetBindingLayout(From(descSetLayouts).Select([](auto x) {return x.Ptr(); }).ToList().GetArrayView());
		pipelineClass->pipeline = pipelineBuilder->ToPipeline(renderTargetLayout);
		pipelineObjects[shaderKeyBuilder.Key] = pipelineClass;
		return pipelineClass.Ptr();
	}

	void ModuleInstance::SetUniformData(void * data, int length)
	{
#ifdef _DEBUG
		if (length > BufferLength)
			throw HardwareRendererException("insufficient uniform buffer.");
#endif
		if (length && UniformPtr)
		{
			frameId = frameId % DynamicBufferLengthMultiplier;
			int alternateBufferOffset = frameId * BufferLength;
			memcpy(UniformPtr + alternateBufferOffset, data, length);
			currentDescriptor = frameId;
			frameId++;
		}
		bool keyChanged = false;
		if (SpecializeParamOffsets.Count())
		{
			if (currentSpecializationKey.Count() != SpecializeParamOffsets.Count())
			{
				currentSpecializationKey.SetSize(SpecializeParamOffsets.Count());
				keyChanged = true;
			}
			for (int i = 0; i < SpecializeParamOffsets.Count(); i++)
			{
				int param = *(int*)((char*)data + SpecializeParamOffsets[i]);
				if (currentSpecializationKey[i] != param)
				{
					keyChanged = true;
					currentSpecializationKey[i] = param;
				}
			}
		}
		if (keyChanged)
		{
			specializedModule = spSpecializeModule(spireContext, module, currentSpecializationKey.Buffer(), currentSpecializationKey.Count(), nullptr);
			ModuleId = spGetModuleUID(specializedModule);
		}
		//UniformMemory->GetBuffer()->SetData(BufferOffset, data, CoreLib::Math::Min(length, BufferLength));
	}

	//IMPL_POOL_ALLOCATOR(ModuleInstance, MaxModuleInstances)

	ModuleInstance::~ModuleInstance()
	{
		if (UniformMemory)
			UniformMemory->Free(UniformPtr, BufferLength * DynamicBufferLengthMultiplier);
	}

	void ModuleInstance::SetDescriptorSetLayout(HardwareRenderer * hw, DescriptorSetLayout * layout)
	{
		this->DescriptorLayout = layout;
		descriptors.Clear();
		for (int i = 0; i < descriptors.GetCapacity(); i++)
			descriptors.Add(hw->CreateDescriptorSet(layout));
	}

}

