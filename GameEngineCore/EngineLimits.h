#ifndef GAME_ENGINE_LIMITS_H
#define GAME_ENGINE_LIMITS_H

namespace GameEngine
{
	const int MaxWorldRenderPasses = 8;
	const int MaxPostRenderPasses = 32;
	const int MaxShadowCascades = 8;
	const int DynamicBufferLengthMultiplier = 3; // double buffering for dynamic uniforms
}

#endif