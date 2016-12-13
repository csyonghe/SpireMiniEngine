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

			pipelineBuilder->SetBindingLayout(1, BindingType::UniformBuffer);
			int offset = sharedRes->GetTextureBindingStart();
			pipelineBuilder->SetBindingLayout(offset + 0, BindingType::Texture); // Albedo
			pipelineBuilder->SetBindingLayout(offset + 1, BindingType::Texture); // PBR
			pipelineBuilder->SetBindingLayout(offset + 2, BindingType::Texture); // Normal
			pipelineBuilder->SetBindingLayout(offset + 3, BindingType::Texture); // Depth
			pipelineBuilder->SetBindingLayout(offset + 7, BindingType::Texture); // Shadow
		}
		virtual void UpdatePipelineBinding(PipelineBinding & binding, RenderAttachments & attachments) override
		{
			if (!litColorBuffer->Texture)
				return;

			binding.BindUniformBuffer(1, sharedRes->viewUniformBuffer.Ptr());
			binding.BindUniformBuffer(4, sharedRes->lightUniformBuffer.Ptr());
			int offset = sharedRes->GetTextureBindingStart();
			binding.BindTexture(offset + 0, baseColorBuffer->Texture.Ptr(), sharedRes->nearestSampler.Ptr());
			binding.BindTexture(offset + 1, pbrBuffer->Texture.Ptr(), sharedRes->nearestSampler.Ptr());
			binding.BindTexture(offset + 2, normalBuffer->Texture.Ptr(), sharedRes->nearestSampler.Ptr());
			binding.BindTexture(offset + 3, depthBuffer->Texture.Ptr(), sharedRes->nearestSampler.Ptr());
			binding.BindTexture(offset + 7, sharedRes->shadowMapResources.shadowMapArray.Ptr(), sharedRes->shadowSampler.Ptr());
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
	};

	PostRenderPass * CreateDeferredLightingPostRenderPass()
	{
		return new DeferredLightingPostRenderPass();
	}
}