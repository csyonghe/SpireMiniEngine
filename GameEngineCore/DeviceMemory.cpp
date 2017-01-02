#include "DeviceMemory.h"
#include "HardwareRenderer.h"
#include "LibMath.h"

namespace GameEngine
{
	void DeviceMemory::Init(HardwareRenderer * hwRenderer, BufferUsage usage, bool pIsMapped, int log2BufferSize, int alignment)
	{
		isMapped = pIsMapped;
		if (isMapped)
		{
			buffer = hwRenderer->CreateMappedBuffer(usage, 1 << log2BufferSize);
			bufferPtr = (unsigned char*)buffer->Map(0, 1 << log2BufferSize);
		}
		else
		{
			buffer = hwRenderer->CreateBuffer(usage, 1 << log2BufferSize);
			bufferPtr = new unsigned char[(int)(1 << log2BufferSize)];
		}
		int logAlignment = CoreLib::Math::Log2Ceil(alignment);
		memory.Init((unsigned char*)bufferPtr, logAlignment, (1 << (log2BufferSize - logAlignment)));
	}

	DeviceMemory::~DeviceMemory()
	{
		if (!isMapped)
			delete[] bufferPtr;
	}

	void * DeviceMemory::Alloc(int size)
	{
		return memory.Alloc(size);
	}

	void DeviceMemory::Free(void * ptr, int size)
	{
		memory.Free((unsigned char*)ptr, size);
	}

	void DeviceMemory::Sync(void * ptr, int size)
	{
		if (!isMapped)
			buffer->SetDataAsync((int)((unsigned char*)ptr - bufferPtr), ptr, size);
	}

	void DeviceMemory::SetDataAsync(int offset, void * data, int length)
	{
		memcpy(bufferPtr + offset, data, length);
		if (!isMapped)
			buffer->SetDataAsync(offset, data, length);
	}

}

