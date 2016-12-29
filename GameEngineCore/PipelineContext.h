#ifndef GAME_ENGINE_PIPELINE_CONTEXT_H
#define GAME_ENGINE_PIPELINE_CONTEXT_H

#include "Spire/Spire.h"
#include "HardwareRenderer.h"
#include "DeviceMemory.h"

namespace GameEngine
{

	class ModuleInstance
	{
	public:
		CoreLib::RefPtr<DescriptorSetLayout> DescriptorLayout;
		CoreLib::RefPtr<DescriptorSet> Descriptors;
		DeviceMemory * UniformMemory;
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
		~ModuleInstance()
		{
			if (UniformMemory)
				UniformMemory->Free(UniformPtr, BufferLength);
		}
	};

	class PipelineContext
	{
	public:
		void BindShader(SpireShader * shader, const FixedFunctionPipelineStates & states);
		void PushModuleInstance(ModuleInstance * module);
		void PopModuleInstance();
		Pipeline* GetPipeline();
	};
}

#endif