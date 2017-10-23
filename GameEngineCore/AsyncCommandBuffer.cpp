#include "AsyncCommandBuffer.h"

namespace GameEngine
{
	AsyncCommandBuffer::AsyncCommandBuffer(HardwareRenderer * hwRender, int size)
	{
		for (int i = 0; i < size; i++)
			commandBuffers.Add(hwRender->CreateCommandBuffer());
	}

	CommandBuffer * AsyncCommandBuffer::BeginRecording(FrameBuffer * frameBuffer)
	{
		framePtr++;
		framePtr %= commandBuffers.Count();
		auto rs = commandBuffers[framePtr].Ptr();
		rs->BeginRecording(frameBuffer);
		return rs;
	}

	CommandBuffer * AsyncCommandBuffer::BeginRecording()
	{
		framePtr++;
		framePtr %= commandBuffers.Count();
		auto rs = commandBuffers[framePtr].Ptr();
		rs->BeginRecording();
		return rs;
	}

	CommandBuffer * AsyncCommandBuffer::GetBuffer()
	{
		return commandBuffers[framePtr].Ptr();
	}

}

