#ifndef GAME_ENGINE_RENDER_PASS_REGISTRY_H
#define GAME_ENGINE_RENDER_PASS_REGISTRY_H

#define DECLARE_WORLD_RENDER_PASS(name) WorldRenderPass* Create##name##RenderPass()
#define DECLARE_POST_RENDER_PASS(name) PostRenderPass* Create##name##PostRenderPass()

namespace GameEngine
{
	class WorldRenderPass;
	class PostRenderPass;

	DECLARE_WORLD_RENDER_PASS(ForwardBase);
	DECLARE_WORLD_RENDER_PASS(GBuffer);
	DECLARE_WORLD_RENDER_PASS(Shadow);
	DECLARE_POST_RENDER_PASS(DeferredLighting);

}

#endif