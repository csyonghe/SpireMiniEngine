#ifndef GAME_ENGINE_PIPELINE_CONTEXT_H
#define GAME_ENGINE_PIPELINE_CONTEXT_H

#include "Spire/Spire.h"
#include "HardwareRenderer.h"
#include "DeviceMemory.h"
#include "EngineLimits.h"

namespace GameEngine
{
	using ShaderKeyBuilder = CoreLib::StringBuilder;

	class MeshVertexFormat;
	
	class ModuleInstance
	{
		friend class PipelineContext;
	private:
		SpireCompilationContext * spireContext = nullptr;
		SpireModule * module = nullptr;
		SpireModule * specializedModule = nullptr;
		CoreLib::List<int> currentSpecializationKey;
		CoreLib::Array<CoreLib::RefPtr<DescriptorSet>, DynamicBufferLengthMultiplier> descriptors;
		int currentDescriptor = 0;
		int frameId = 0;
	public:
		CoreLib::RefPtr<DescriptorSetLayout> DescriptorLayout;
		CoreLib::List<int> SpecializeParamOffsets;
		DeviceMemory * UniformMemory = nullptr;
		int BufferOffset = 0, BufferLength = 0;
		unsigned char * UniformPtr = nullptr;
		CoreLib::String BindingName;
		void SetUniformData(void * data, int length);
		ModuleInstance(SpireCompilationContext * ctx, SpireModule * m)
		{
			spireContext = ctx;
			module = m;
			specializedModule = module;
		}
		~ModuleInstance();
		void SetDescriptorSetLayout(HardwareRenderer * hw, DescriptorSetLayout * layout);
		DescriptorSet * UpdateDescriptorSet()
		{
			currentDescriptor++;
			currentDescriptor %= DynamicBufferLengthMultiplier;
			return descriptors[currentDescriptor].Ptr();
		}
		DescriptorSet * GetCurrentDescriptorSet()
		{
			return descriptors[currentDescriptor].Ptr();
		}
		void GetKey(ShaderKeyBuilder & keyBuilder)
		{
			keyBuilder.Append(spGetModuleName(specializedModule));
		}
	};

	using DescriptorSetBindingArray = CoreLib::Array<DescriptorSet*, 32>;

	class PipelineClass
	{
	public:
		CoreLib::List<CoreLib::RefPtr<Shader>> shaders;
		CoreLib::RefPtr<Pipeline> pipeline;
		CoreLib::List<CoreLib::RefPtr<DescriptorSetLayout>> descriptorSetLayouts;
	};

	class PipelineContext
	{
	private:
		SpireCompilationContext * spireContext = nullptr;
		SpireShader * shader = nullptr;
		RenderTargetLayout * renderTargetLayout = nullptr;

		FixedFunctionPipelineStates * fixedFunctionStates = nullptr;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<PipelineClass>> pipelineObjects;
		CoreLib::Array<ModuleInstance*, 32> modules;
		ShaderKeyBuilder shaderKeyBuilder;
		HardwareRenderer * hwRenderer;

		CoreLib::Dictionary<int, VertexFormat> vertexFormats;
	public:
		PipelineContext() = default;
		void Init(SpireCompilationContext * spireCtx, HardwareRenderer * hw) 
		{
			spireContext = spireCtx;
			hwRenderer = hw;
		}
		VertexFormat LoadVertexFormat(MeshVertexFormat vertFormat);
		void BindShader(SpireShader * pShader, RenderTargetLayout * pRenderTargetLayout, FixedFunctionPipelineStates * states)
		{
			shader = pShader;
			renderTargetLayout = pRenderTargetLayout;
			fixedFunctionStates = states;
		}
		void PushModuleInstance(ModuleInstance * module)
		{
			modules.Add(module);
		}
		void PopModuleInstance()
		{
			modules.SetSize(modules.Count() - 1);
		}
		void GetBindings(DescriptorSetBindingArray & bindings)
		{
			bindings.Clear();
			for (auto m : modules)
				bindings.Add(m->GetCurrentDescriptorSet());
		}
		PipelineClass* GetPipeline(MeshVertexFormat * vertFormat);
	};
}

#endif