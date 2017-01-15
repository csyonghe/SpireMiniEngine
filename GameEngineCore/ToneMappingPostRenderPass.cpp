#ifndef GAME_ENGINE_TONE_MAPPING_H
#define GAME_ENGINE_TONE_MAPPING_H

#include "PostRenderPass.h"

namespace GameEngine
{
	class ToneMappingPostRenderPass : public PostRenderPass
	{
	protected:
		CoreLib::RefPtr<RenderTarget> litColorBuffer, colorOutBuffer;
		CoreLib::RefPtr<DescriptorSet> descSet;
		CoreLib::RefPtr<Buffer> buffer;
		CoreLib::String sourceBuffer;
	public:
		virtual void AcquireRenderTargets() override
		{
			litColorBuffer = viewRes->LoadSharedRenderTarget(sources[0].Name, sources[0].Format);
			colorOutBuffer = viewRes->LoadSharedRenderTarget(sources[1].Name, sources[1].Format);

		}
		virtual void SetupPipelineBindingLayout(PipelineBuilder * pipelineBuilder, CoreLib::List<AttachmentLayout> & renderTargets) override
		{
			renderTargets.Add(AttachmentLayout(TextureUsage::ColorAttachment, StorageFormat::RGBA_8));
			pipelineBuilder->SetDebugName("tone_mapping");
			descSet = hwRenderer->CreateDescriptorSet(descLayouts[0].Ptr());
		}
		virtual void UpdateDescriptorSetBinding(SharedModuleInstances /*sharedModules*/, DescriptorSetBindings & binding) override
		{
			binding.Bind(0, descSet.Ptr());
		}
		virtual void UpdateRenderAttachments(RenderAttachments & attachments) override
		{
			if (!litColorBuffer->Texture)
				return;

			descSet->BeginUpdate();
			descSet->Update(0, litColorBuffer->Texture.Ptr(), TextureAspect::Color);
			descSet->Update(1, sharedRes->nearestSampler.Ptr());
			descSet->EndUpdate();

			attachments.SetAttachment(0, colorOutBuffer->Texture.Ptr());
		}
		virtual CoreLib::String GetShaderFileName() override
		{
			return "ToneMapping.shader";
		}
		virtual char * GetName() override
		{
			return "ToneMappingPostPass";
		}
		virtual void SetParameters(void * /*data*/, int /*count*/) override
		{}
	public:
		ToneMappingPostRenderPass(ViewResource * viewRes)
			: PostRenderPass(viewRes)
		{
		}
	};
	PostRenderPass * CreateToneMappingPostRenderPass(ViewResource * viewRes)
	{
		return new ToneMappingPostRenderPass(viewRes);
	}
}

#endif