#ifndef GAME_ENGINE_PIPELINE_CONTEXT_H
#define GAME_ENGINE_PIPELINE_CONTEXT_H

#include "Spire/Spire.h"
#include "HardwareRenderer.h"
#include "DeviceMemory.h"

namespace GameEngine
{
	using ShaderKeyBuilder = CoreLib::StringBuilder;

	class MeshVertexFormat;
	
	class ModuleInstance
	{
		friend class PipelineContext;
	private:
		SpireModule * module = nullptr;
		CoreLib::String moduleName;
	public:
		CoreLib::RefPtr<DescriptorSetLayout> DescriptorLayout;
		CoreLib::RefPtr<DescriptorSet> Descriptors;
		DeviceMemory * UniformMemory = nullptr;
		int BufferOffset = 0, BufferLength = 0;
		unsigned char * UniformPtr = nullptr;
		CoreLib::String BindingName;
		void SetUniformData(void * data, int length)
		{
#ifdef _DEBUG
			if (length > BufferLength)
				throw HardwareRendererException("insufficient uniform buffer.");
#endif
			UniformMemory->GetBuffer()->SetData(BufferOffset, data, CoreLib::Math::Min(length, BufferLength));
		}
		ModuleInstance(SpireModule * m)
		{
			module = m;
			moduleName = spGetModuleName(m);
		}
		~ModuleInstance()
		{
			if (UniformMemory)
				UniformMemory->Free(UniformPtr, BufferLength);
		}
		void GetKey(ShaderKeyBuilder & keyBuilder)
		{
			keyBuilder.Append(moduleName);
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
				bindings.Add(m->Descriptors.Ptr());
		}
		PipelineClass* GetPipeline(MeshVertexFormat * vertFormat);
	};
}

#endif