#include "ViewResource.h"
#include "RenderContext.h"

namespace GameEngine
{
	using namespace CoreLib;
	
	RefPtr<RenderTarget> ViewResource::LoadSharedRenderTarget(String name, StorageFormat format, float ratio, int w, int h)
	{
		RefPtr<RenderTarget> result;
		if (renderTargets.TryGetValue(name, result))
		{
			if (result->Format == format)
				return result;
			else
				throw InvalidProgramException("the required buffer is not in required format.");
		}
		result = new RenderTarget();
		result->Format = format;
		result->UseFixedResolution = ratio == 0.0f;
		if (screenWidth > 0 || result->UseFixedResolution)
		{
			if (ratio == 0.0f)
			{
				result->Width = w;
				result->Height = h;
			}
			else
			{
				result->Width = (int)(screenWidth * ratio);
				result->Height = (int)(screenHeight * ratio);
			}
			if (format == StorageFormat::Depth24Stencil8 || format == StorageFormat::Depth32 || format == StorageFormat::Depth24)
				result->Texture = hwRenderer->CreateTexture2D(TextureUsage::SampledDepthAttachment, result->Width, result->Height, 1, format);
			else
				result->Texture = hwRenderer->CreateTexture2D(TextureUsage::SampledColorAttachment, result->Width, result->Height, 1, format);
			
		}
		result->FixedWidth = w;
		result->FixedHeight = h;
		renderTargets[name] = result;
		return result;
	}

	void ViewResource::Resize(int w, int h)
	{
		screenWidth = w;
		screenHeight = h;
		for (auto & r : renderTargets)
		{
			if (!r.Value->UseFixedResolution)
			{
				r.Value->Width = (int)(screenWidth * r.Value->ResolutionScale);
				r.Value->Height = (int)(screenHeight * r.Value->ResolutionScale);
				if (r.Value->Format == StorageFormat::Depth24Stencil8 || r.Value->Format == StorageFormat::Depth32 || r.Value->Format == StorageFormat::Depth24)
					r.Value->Texture = hwRenderer->CreateTexture2D(TextureUsage::SampledDepthAttachment, r.Value->Width, r.Value->Height, 1, r.Value->Format);
				else
					r.Value->Texture = hwRenderer->CreateTexture2D(TextureUsage::SampledColorAttachment, r.Value->Width, r.Value->Height, 1, r.Value->Format);
			}
		}
		for (auto & output : renderOutputs)
		{
			if (!output->bindings.First()->UseFixedResolution)
				UpdateRenderResultFrameBuffer(output.Ptr());
		}
		Resized();
	}

	void ViewResource::UpdateRenderResultFrameBuffer(RenderOutput * output)
	{
		RenderAttachments attachments;
		for (int i = 0; i < output->bindings.Count(); i++)
		{
			if (output->bindings[i]->Texture)
				attachments.SetAttachment(i, output->bindings[i]->Texture.Ptr());
			else if (output->bindings[i]->TextureArray)
				attachments.SetAttachment(i, output->bindings[i]->TextureArray.Ptr(), output->bindings[i]->Layer);
		}
		if (attachments.attachments.Count())
			output->frameBuffer = output->renderTargetLayout->CreateFrameBuffer(attachments);
	}
}