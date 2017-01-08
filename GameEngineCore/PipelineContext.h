#ifndef GAME_ENGINE_PIPELINE_CONTEXT_H
#define GAME_ENGINE_PIPELINE_CONTEXT_H

#include "Spire/Spire.h"
#include "HardwareRenderer.h"
#include "DeviceMemory.h"
#include "EngineLimits.h"
#include "Mesh.h"

namespace GameEngine
{
	class ShaderKey
	{
	public:
		long long ModuleIds[2] = {0, 0};
		int count = 0;
		inline int GetHashCode()
		{
			return (int)(ModuleIds[0] ^ ModuleIds[1]);
		}
		bool operator == (const ShaderKey & key)
		{
			return ModuleIds[0] == key.ModuleIds[0] && ModuleIds[1] == key.ModuleIds[1];
		}
	};
	class ShaderKeyBuilder
	{
	public:
		ShaderKey Key;
		inline void Clear()
		{
			Key.count = 0;
			Key.ModuleIds[0] = 0;
			Key.ModuleIds[1] = 0;
		}
		inline void FlipLeadingByte(unsigned int header)
		{
			Key.ModuleIds[0] ^= ((long long)header << 56);
		}
		inline void Append(unsigned int moduleId)
		{
			Key.ModuleIds[Key.count >> 2] ^= ((long long)moduleId << (Key.count & 3) * 16);
			Key.count++;
		}
		inline void Pop()
		{
			Key.count--;
			Key.ModuleIds[Key.count >> 2] ^= (Key.ModuleIds[Key.count >> 2] & (0xFFFFULL << (Key.count & 3) * 16));
		}
	};

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
		int ModuleId = 0;
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
			ModuleId = spGetModuleUID(specializedModule);
		}
		~ModuleInstance();
		void SetDescriptorSetLayout(HardwareRenderer * hw, DescriptorSetLayout * layout);
		DescriptorSet * GetDescriptorSet(int i)
		{
			return descriptors[i].Ptr();
		}
		DescriptorSet * GetCurrentDescriptorSet()
		{
			return descriptors[currentDescriptor].Ptr();
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

	class RenderStat;

	class PipelineContext
	{
	private:
		SpireCompilationContext * spireContext = nullptr;
		SpireShader * shader = nullptr;
		RenderTargetLayout * renderTargetLayout = nullptr;
		ShaderKey lastKey;
		PipelineClass * lastPipeline = nullptr;
		FixedFunctionPipelineStates * fixedFunctionStates = nullptr;
		CoreLib::EnumerableDictionary<ShaderKey, CoreLib::RefPtr<PipelineClass>> pipelineObjects;
		CoreLib::Array<ModuleInstance*, 32> modules;
		ShaderKeyBuilder shaderKeyBuilder;
		HardwareRenderer * hwRenderer;
		RenderStat * renderStats = nullptr;
		CoreLib::Dictionary<int, VertexFormat> vertexFormats;
		PipelineClass* CreatePipeline(MeshVertexFormat * vertFormat);
	public:
		PipelineContext() = default;
		void Init(SpireCompilationContext * spireCtx, HardwareRenderer * hw, RenderStat * pRenderStats)
		{
			spireContext = spireCtx;
			hwRenderer = hw;
			renderStats = pRenderStats;
		}
		inline RenderStat * GetRenderStat() 
		{
			return renderStats;
		}
		VertexFormat LoadVertexFormat(MeshVertexFormat vertFormat);
		void BindShader(SpireShader * pShader, RenderTargetLayout * pRenderTargetLayout, FixedFunctionPipelineStates * states)
		{
			shader = pShader;
			renderTargetLayout = pRenderTargetLayout;
			fixedFunctionStates = states;
			shaderKeyBuilder.Clear();
			shaderKeyBuilder.Append(spShaderGetId(shader));
		}
		void PushModuleInstance(ModuleInstance * module)
		{
			modules.Add(module);
			shaderKeyBuilder.Append(module->ModuleId);
		}
		void PopModuleInstance()
		{
			modules.SetSize(modules.Count() - 1);
			shaderKeyBuilder.Pop();
		}
		void GetBindings(DescriptorSetBindingArray & bindings)
		{
			bindings.Clear();
			for (auto m : modules)
				bindings.Add(m->GetCurrentDescriptorSet());
		}
		__forceinline PipelineClass* GetPipeline(MeshVertexFormat * vertFormat)
		{
			unsigned int vtxId = (unsigned int)vertFormat->GetTypeId();
			shaderKeyBuilder.FlipLeadingByte(vtxId);
			if (shaderKeyBuilder.Key == lastKey)
			{
				shaderKeyBuilder.FlipLeadingByte(vtxId);
				return lastPipeline;
			}
			if (auto pipeline = pipelineObjects.TryGetValue(shaderKeyBuilder.Key))
			{
				lastKey = shaderKeyBuilder.Key;
				lastPipeline = pipeline->Ptr();
				shaderKeyBuilder.FlipLeadingByte(vtxId);
				return pipeline->Ptr();
			}
			lastKey = shaderKeyBuilder.Key;
			lastPipeline = CreatePipeline(vertFormat);
			shaderKeyBuilder.FlipLeadingByte(vtxId);
			return lastPipeline;
		}
	};
}

#endif