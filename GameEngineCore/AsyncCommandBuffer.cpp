#include "AsyncCommandBuffer.h"

namespace GameEngine
{
	AsyncCommandBuffer::AsyncCommandBuffer(HardwareRenderer * hwRender)
	{
		for (int i = 0; i < commandBuffers.GetCapacity(); i++)
			commandBuffers.Add(hwRender->CreateCommandBuffer());
	}

	CommandBuffer * AsyncCommandBuffer::BeginRecording(FrameBuffer * frameBuffer)
	{
		framePtr++;
		framePtr %= CommandBufferMultiplier;
		auto rs = commandBuffers[framePtr].Ptr();
		rs->BeginRecording(frameBuffer);
		return rs;
	}

	CommandBuffer * AsyncCommandBuffer::BeginRecording()
	{
		framePtr++;
		framePtr %= CommandBufferMultiplier;
		auto rs = commandBuffers[framePtr].Ptr();
		rs->BeginRecording();
		return rs;
	}

	CommandBuffer * AsyncCommandBuffer::GetBuffer()
	{
		return commandBuffers[framePtr].Ptr();
	}

}

