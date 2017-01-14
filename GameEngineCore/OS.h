#ifndef GAME_ENGINE_OS_H
#define GAME_ENGINE_OS_H

namespace GameEngine
{
	typedef void* WindowHandle;
	enum class RenderAPI
	{
		OpenGL, Vulkan, VulkanSingle
	};

}

#endif