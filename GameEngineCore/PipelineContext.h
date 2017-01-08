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
			Key.ModuleIds[0] ^= ((long long)header << 54);
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
	public:
		int ModuleId;
		//USE_POOL_ALLOCATOR(ModuleInstance, MaxModuleInstances)
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
		ModuleInstance() = default;
		void Init(SpireCompilationContext * ctx, SpireModule * m)
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
		operator bool()
		{
			return module != nullptr;
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
		int modulePtr = 0;
		ShaderKey lastKey; 
		unsigned int lastVtxId = 0;
		bool shaderKeyChanged = true;
		SpireCompilationContext * spireContext = nullptr;
		ModuleInstance* modules[32];
		SpireShader * shader = nullptr;
		RenderTargetLayout * renderTargetLayout = nullptr;
		PipelineClass * lastPipeline = nullptr;
		FixedFunctionPipelineStates * fixedFunctionStates = nullptr;
		CoreLib::EnumerableDictionary<ShaderKey, CoreLib::RefPtr<PipelineClass>> pipelineObjects;
		ShaderKeyBuilder shaderKeyBuilder;
		HardwareRenderer * hwRenderer;
		RenderStat * renderStats = nullptr;
		CoreLib::Dictionary<int, VertexFormat> vertexFormats;
		PipelineClass * GetPipelineInternal(MeshVertexFormat * vertFormat, int vtxId);
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
			shaderKeyChanged = true;
			modulePtr = 0;
			for (int i = 0; i < sizeof(modules) / sizeof(ModuleInstance*); i++)
				modules[i] = nullptr;
		}
		void PushModuleInstance(ModuleInstance * module)
		{
			if (!modules[modulePtr] || modules[modulePtr]->ModuleId != module->ModuleId)
			{
				shaderKeyChanged = true;
			}
			modules[modulePtr] = module;
			++modulePtr;
		}
		void PushModuleInstanceNoShaderChange(ModuleInstance * module)
		{
			modules[modulePtr] = module;
			++modulePtr;
		}
		void PopModuleInstance()
		{
			--modulePtr;
		}
		void GetBindings(DescriptorSetBindingArray & bindings)
		{
			bindings.Clear();
			for (int i = 0; i < modulePtr; i++)
				bindings.Add(modules[i]->GetCurrentDescriptorSet());
		}
		inline PipelineClass* GetPipeline(MeshVertexFormat * vertFormat)
		{
			unsigned int vtxId = (unsigned int)vertFormat->GetTypeId();
			if (!shaderKeyChanged && vtxId == lastVtxId)
				return lastPipeline;
			return GetPipelineInternal(vertFormat, vtxId);
		}
	};
}

#endif