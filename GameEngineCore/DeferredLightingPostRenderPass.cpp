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
			depthBuffer = sharedRes->LoadSharedRenderTarget("depthBuffer", StorageFormat::Depth24Stencil8);
			pbrBuffer = sharedRes->LoadSharedRenderTarget("pbrBuffer", StorageFormat::RGBA_8);
			normalBuffer = sharedRes->LoadSharedRenderTarget("normalBuffer", StorageFormat::RGB10_A2);
			litColorBuffer = sharedRes->LoadSharedRenderTarget("litColor", StorageFormat::RGBA_8);
		}
		virtual void SetupPipelineBindingLayout(PipelineBuilder * pipelineBuilder, List<TextureUsage> & renderTargets) override
		{
			renderTargets.Add(TextureUsage::ColorAttachment);
			renderTargets.Add(TextureUsage::ColorAttachment);
			renderTargets.Add(TextureUsage::ColorAttachment);
			renderTargets.Add(TextureUsage::DepthAttachment);
			pipelineBuilder->SetDebugName("deferred_lighting");
			deferredDescSet = hwRenderer->CreateDescriptorSet(descLayouts[0].Ptr());
		}
		virtual void UpdatePipelineBinding(SharedModuleInstances sharedModules, DescriptorSetBindings & binding, RenderAttachments & attachments) override
		{
			if (!litColorBuffer->Texture)
				return;
			binding.Bind(0, deferredDescSet.Ptr());
			binding.Bind(1, sharedModules.View->Descriptors.Ptr());
			binding.Bind(2, sharedModules.Lighting->Descriptors.Ptr());

			deferredDescSet->BeginUpdate();
			deferredDescSet->Update(1, baseColorBuffer->Texture.Ptr());
			deferredDescSet->Update(2, pbrBuffer->Texture.Ptr());
			deferredDescSet->Update(3, normalBuffer->Texture.Ptr());
			deferredDescSet->Update(4, depthBuffer->Texture.Ptr());
			deferredDescSet->Update(5, sharedRes->nearestSampler.Ptr());
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