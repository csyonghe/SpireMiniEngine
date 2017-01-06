#include "PostRenderPass.h"
#include "Material.h"
#include "Mesh.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
	class DeferredLightingPostRenderPass : public PostRenderPass
	{
	protected:
		RefPtr<RenderTarget> baseColorBuffer, pbrBuffer, normalBuffer, depthBuffer, litColorBuffer;
		RefPtr<DescriptorSet> deferredDescSet;
	public:
		virtual void AcquireRenderTargets() override
		{
			baseColorBuffer = sharedRes->LoadSharedRenderTarget("baseColorBuffer", StorageFormat::RGBA_8);
			depthBuffer = sharedRes->LoadSharedRenderTarget("depthValBuffer", StorageFormat::RG_F32);
			pbrBuffer = sharedRes->LoadSharedRenderTarget("pbrBuffer", StorageFormat::RGBA_8);
			normalBuffer = sharedRes->LoadSharedRenderTarget("normalBuffer", StorageFormat::RGB10_A2);
			litColorBuffer = sharedRes->LoadSharedRenderTarget("litColor", StorageFormat::RGBA_8);
		}
		virtual void SetupPipelineBindingLayout(PipelineBuilder * pipelineBuilder, List<AttachmentLayout> & renderTargets) override
		{
			renderTargets.Add(AttachmentLayout(TextureUsage::ColorAttachment, StorageFormat::RGBA_8));
			pipelineBuilder->SetDebugName("deferred_lighting");
			deferredDescSet = hwRenderer->CreateDescriptorSet(descLayouts[0].Ptr());
		}
		virtual void UpdateDescriptorSetBinding(SharedModuleInstances sharedModules, DescriptorSetBindings & binding) override
		{
			binding.Bind(0, deferredDescSet.Ptr());
			binding.Bind(1, sharedModules.View->GetCurrentDescriptorSet());
			binding.Bind(2, sharedModules.Lighting->GetCurrentDescriptorSet());
		}
		virtual void UpdateRenderAttachments(RenderAttachments & attachments) override
		{
			if (!litColorBuffer->Texture)
				return;
			
			deferredDescSet->BeginUpdate();
			deferredDescSet->Update(0, baseColorBuffer->Texture.Ptr(), TextureAspect::Color);
			deferredDescSet->Update(1, pbrBuffer->Texture.Ptr(), TextureAspect::Color);
			deferredDescSet->Update(2, normalBuffer->Texture.Ptr(), TextureAspect::Color);
			deferredDescSet->Update(3, depthBuffer->Texture.Ptr(), TextureAspect::Color);
			deferredDescSet->Update(4, sharedRes->nearestSampler.Ptr());
			deferredDescSet->EndUpdate();

			attachments.SetAttachment(0, litColorBuffer->Texture.Ptr());
		}
		virtual String GetShaderFileName() override
		{
			return "DeferredLighting.shader";
		}
		virtual char * GetName() override
		{
			return "DeferredLighting";
		}
		virtual void SetParameters(void * /*data*/, int /*count*/) override
		{}
	};

	PostRenderPass * CreateDeferredLightingPostRenderPass()
	{
		return new DeferredLightingPostRenderPass();
	}
}