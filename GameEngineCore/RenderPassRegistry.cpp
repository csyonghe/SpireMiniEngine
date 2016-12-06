#include "RenderPass.h"
#include "Renderer.h"
#include "Engine.h"

#define DECLARE_WORLD_RENDER_PASS(name) WorldRenderPass* Create##name##RenderPass()
#define REGISTER_WORLD_RENDER_PASS(name) renderer->RegisterWorldRenderPass(Create##name##RenderPass())

#define DECLARE_POST_RENDER_PASS(name) PostRenderPass* Create##name##PostRenderPass()
#define REGISTER_POST_RENDER_PASS(name) renderer->RegisterPostRenderPass(Create##name##PostRenderPass())

namespace GameEngine
{
	DECLARE_WORLD_RENDER_PASS(ForwardBase);
	DECLARE_WORLD_RENDER_PASS(GBuffer);

	DECLARE_POST_RENDER_PASS(DeferredLighting);

	void RegisterRenderPasses(Renderer * renderer)
	{
		if (Engine::Instance()->GetGraphicsSettings().UseDeferredRenderer)
		{
			REGISTER_WORLD_RENDER_PASS(GBuffer);
			REGISTER_POST_RENDER_PASS(DeferredLighting);
		}
		else
			REGISTER_WORLD_RENDER_PASS(ForwardBase);
	}
}