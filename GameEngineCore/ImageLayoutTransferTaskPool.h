#ifndef GAME_ENGINE_IMAGE_LAYOUT_TRANSFER_TASK_POOL_H
#define GAME_ENGINE_IMAGE_LAYOUT_TRANSFER_TASK_POOL_H

#include "CoreLib/VectorMath.h"
#include "CoreLib/Basic.h"
#include "HardwareRenderer.h"
#include "RenderContext.h"

namespace GameEngine
{
	class GeneralRenderTask;

	class ImageLayoutTransferTaskPool
	{
	private:
		CoreLib::List<CoreLib::RefPtr<AsyncCommandBuffer>> commandBuffers;
		CoreLib::List<CoreLib::RefPtr<GeneralRenderTask>> tasks;
		int ptr = 0;
	public:
		void Reset();
		GeneralRenderTask * NewImageLayoutTransferTask(CoreLib::ArrayView<Texture*> renderTargetTextures, CoreLib::ArrayView<Texture*> samplingTextures);
	};
}

#endif