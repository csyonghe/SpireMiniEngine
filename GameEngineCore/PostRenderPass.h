#ifndef GAME_ENGINE_POST_RENDER_PASS_H
#define GAME_ENGINE_POST_RENDER_PASS_H

#include "RenderPass.h"

namespace GameEngine
{
	class ViewResource;

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

	struct PostPassSource
	{
		const char * Name;
		StorageFormat Format;
		PostPassSource() = default;
		PostPassSource(const char * name, StorageFormat format)
		{
			Name = name;
			Format = format;
		}
	};

	class PostRenderPass : public RenderPass
	{
	protected:
		ViewResource * viewRes = nullptr;
		CoreLib::List<PostPassSource> sources;
		CoreLib::List<Texture*> textures;
	protected:
		bool clearFrameBuffer = false;
		CoreLib::RefPtr<RenderOutput> renderOutput;
		CoreLib::RefPtr<FrameBuffer> frameBuffer;
		CoreLib::RefPtr<AsyncCommandBuffer> commandBuffer, transferInCommandBuffer, transferOutCommandBuffer;
		CoreLib::RefPtr<Pipeline> pipeline;
		CoreLib::List<CoreLib::RefPtr<Shader>> shaders;
		CoreLib::List<CoreLib::RefPtr<DescriptorSetLayout>> descLayouts;
		virtual void SetupPipelineBindingLayout(PipelineBuilder * pipelineBuilder, CoreLib::List<AttachmentLayout> & renderTargets) = 0;
		virtual void UpdateDescriptorSetBinding(SharedModuleInstances sharedModules, DescriptorSetBindings & binding) = 0;
		virtual void UpdateRenderAttachments(RenderAttachments & attachments) = 0;
		virtual void AcquireRenderTargets() = 0;
		virtual CoreLib::String GetShaderFileName() = 0;
		virtual void Create(Renderer * renderer) override;
		virtual void Resized();
	public:
		PostRenderPass(ViewResource * view);
		~PostRenderPass();
		void Execute(SharedModuleInstances sharedModules);
		CoreLib::RefPtr<RenderTask> CreateInstance(SharedModuleInstances sharedModules);
		virtual void SetParameters(void * data, int count) = 0;
		void SetSource(CoreLib::ArrayView<PostPassSource> sourceTextures);
		virtual int GetShaderId() override
		{
			return -1;
		}
	};
}

#endif
