#ifndef GAME_ENGINE_DEVICE_MEMORY_H
#define GAME_ENGINE_DEVICE_MEMORY_H

#include "CoreLib/MemoryPool.h"
#include "HardwareRenderer.h"

namespace GameEngine
{
	class DeviceMemory
	{
	private:
		CoreLib::MemoryPool memory;
		CoreLib::RefPtr<Buffer> buffer;
		unsigned char * bufferPtr = nullptr;
	public:
		DeviceMemory() {}
		~DeviceMemory();
		void Init(HardwareRenderer * hwRenderer, BufferUsage usage, int log2BufferSize, int alignment);
		void * Alloc(int size);
		void Free(void * ptr, int size);
		void Sync(void * ptr, int size);
		Buffer * GetBuffer()
		{
			return buffer.Ptr();
		}
		void * BufferPtr()
		{
			return bufferPtr;
		}
	};
}

#endif