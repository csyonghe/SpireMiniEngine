#ifndef GAME_ENGINE_POST_RENDER_PASS_H
#define GAME_ENGINE_POST_RENDER_PASS_H

#include "RenderPass.h"

namespace GameEngine
{
	class SharedModuleInstances
	{
	public:
		ModuleInstance * View;
		ModuleInstance * Lighting;
	};

	class PostRenderPass : public RenderPass
	{
	protected:
		bool clearFrameBuffer = false;
		CoreLib::RefPtr<RenderOutput> renderOutput;
		CoreLib::RefPtr<FrameBuffer> frameBuffer;
		CoreLib::RefPtr<CommandBuffer> commandBuffer;
		CoreLib::RefPtr<Pipeline> pipeline;
		CoreLib::List<CoreLib::RefPtr<Shader>> shaders;
		CoreLib::List<CoreLib::RefPtr<DescriptorSetLayout>> descLayouts;
		virtual void SetupPipelineBindingLayout(PipelineBuilder * pipelineBuilder, CoreLib::List<TextureUsage> & renderTargets) = 0;
		virtual void UpdatePipelineBinding(SharedModuleInstances sharedModules, DescriptorSetBindings & bindings, RenderAttachments & attachments) = 0;
		virtual CoreLib::String GetShaderFileName() = 0;
		virtual void Create() override;
		virtual void AcquireRenderTargets() = 0;
	public:
		void Execute();
		virtual void RecordCommandBuffer(SharedModuleInstances sharedModules, int screenWidth, int screenHeight);
		virtual void SetParameters(void * data, int count) = 0;
	};
}

#endif
