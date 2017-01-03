#ifndef GAME_ENGINE_POST_RENDER_PASS_H
#define GAME_ENGINE_POST_RENDER_PASS_H

#include "RenderPass.h"

namespace GameEngine
{
	struct DescriptorSetBindings
	{
		struct Binding
		{
			int index;
			DescriptorSet * descriptorSet;
		};
		CoreLib::Array<Binding, 16> bindings;
		void Bind(int id, DescriptorSet * set)
		{
			bindings.Add(Binding{ id, set });
		}
	};

	class PostRenderPass : public RenderPass
	{
	private:
		int width, height;
	protected:
		bool clearFrameBuffer = false;
		CoreLib::RefPtr<RenderOutput> renderOutput;
		CoreLib::RefPtr<FrameBuffer> frameBuffer;
		CoreLib::RefPtr<CommandBuffer> commandBuffer;
		CoreLib::RefPtr<Pipeline> pipeline;
		CoreLib::List<CoreLib::RefPtr<Shader>> shaders;
		CoreLib::List<CoreLib::RefPtr<DescriptorSetLayout>> descLayouts;
		virtual void SetupPipelineBindingLayout(PipelineBuilder * pipelineBuilder, CoreLib::List<TextureUsage> & renderTargets) = 0;
		virtual void UpdateDescriptorSetBinding(SharedModuleInstances sharedModules, DescriptorSetBindings & binding) = 0;
		virtual void UpdateRenderAttachments(RenderAttachments & attachments) = 0;
		virtual CoreLib::String GetShaderFileName() = 0;
		virtual void Create() override;
		virtual void AcquireRenderTargets() = 0;
	public:
		void Execute(SharedModuleInstances sharedModules);
		virtual void Resize(int screenWidth, int screenHeight);
		virtual void SetParameters(void * data, int count) = 0;
	};
}

#endif
