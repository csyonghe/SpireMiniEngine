#include "ImageLayoutTransferTaskPool.h"
#include "Engine.h"

namespace GameEngine
{
	void ImageLayoutTransferTaskPool::Reset()
	{
		ptr = 0;
	}

	GeneralRenderTask * ImageLayoutTransferTaskPool::NewImageLayoutTransferTask(CoreLib::ArrayView<Texture*> renderTargetTextures, CoreLib::ArrayView<Texture*> samplingTextures)
	{
		GeneralRenderTask * result = nullptr;
		AsyncCommandBuffer * cmdBuf = nullptr;
		if (ptr < tasks.Count())
		{
			cmdBuf = commandBuffers[ptr].Ptr();
			result = tasks[ptr++].Ptr();
		}
		else
		{
			commandBuffers.Add(new AsyncCommandBuffer(Engine::Instance()->GetRenderer()->GetHardwareRenderer(), DynamicBufferLengthMultiplier));
			tasks.Add(new GeneralRenderTask(commandBuffers.Last().Ptr()));
			result = tasks.Last().Ptr();
			cmdBuf = commandBuffers.Last().Ptr();
			ptr++;
		}
		auto buf = cmdBuf->BeginRecording();
		buf->TransferLayout(renderTargetTextures, TextureLayoutTransfer::UndefinedToRenderAttachment);
		buf->TransferLayout(samplingTextures, TextureLayoutTransfer::RenderAttachmentToSample);
		buf->EndRecording();
		return result;
	}
}