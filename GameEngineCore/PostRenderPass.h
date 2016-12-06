#ifndef GAME_ENGINE_POST_RENDER_PASS_H
#define GAME_ENGINE_POST_RENDER_PASS_H

#include "RenderPass.h"

namespace GameEngine
{
	class PostRenderPass : public RenderPass
	{
	protected:
		bool clearFrameBuffer = false;
		CoreLib::RefPtr<Pipeline> pipeline;
		CoreLib::RefPtr<PipelineInstance> pipelineInstance;
		CoreLib::RefPtr<CommandBuffer> commandBuffer;
		CoreLib::RefPtr<RenderTargetLayout> renderTargetLayout;
		virtual void SetupPipelineBindingLayout(PipelineBuilder * pipelineBuilder, CoreLib::List<TextureUsage> & renderTargets) = 0;
		virtual void UpdatePipelineBinding(PipelineBinding & binding, RenderAttachments & attachments) = 0;
		virtual CoreLib::String GetShaderFileName() = 0;
		virtual void Create() override;
		void RecordCommandBuffer();
	public:
		virtual void UpdateFrameBuffer() override;
		void Execute();
	};
}

#endif
