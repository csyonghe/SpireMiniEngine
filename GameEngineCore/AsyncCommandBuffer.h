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
		CoreLib::List<CoreLib::RefPtr<CommandBuffer>> commandBuffers;
	public:
		AsyncCommandBuffer(HardwareRenderer * hwRender, int size = 12);
		CommandBuffer * BeginRecording(FrameBuffer * frameBuffer);
		CommandBuffer * BeginRecording();
		CommandBuffer * GetBuffer();
	};
}

#endif