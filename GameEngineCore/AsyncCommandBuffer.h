#ifndef GAME_ENGINE_ASYNC_COMMAND_BUFFER_H
#define GAME_ENGINE_ASYNC_COMMAND_BUFFER_H

#include "HardwareRenderer.h"
#include "EngineLimits.h"

namespace GameEngine
{
	class AsyncCommandBuffer
	{
	private:
		int framePtr = 0;
		CoreLib::Array<CoreLib::RefPtr<CommandBuffer>, DynamicBufferLengthMultiplier> commandBuffers;
	public:
		AsyncCommandBuffer(HardwareRenderer * hwRender);
		CommandBuffer * BeginRecording(FrameBuffer * frameBuffer);
		CommandBuffer * GetBuffer();
	};
}

#endif