#ifndef GAME_ENGINE_COMMON_H
#define GAME_ENGINE_COMMON_H

namespace GameEngine
{
	typedef void* WindowHandle;
	const int MaxWorldRenderPasses = 8;
	const int MaxPostRenderPasses = 32;
	const int MaxTextureBindings = 32;
	const int MaxShadowCascades = 8;
	const int MaxViewUniformSize = 4096;
	const int MaxLightBufferSize = 16384;
	const int DynamicBufferLengthMultiplier = 3; // triple buffering for dynamic uniforms
}

#endif