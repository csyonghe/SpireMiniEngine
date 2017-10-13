#include "RenderPass.h"
#include "RenderContext.h"
#include <assert.h>

namespace GameEngine
{
	void RenderPass::Init(Renderer * renderer)
	{
		sharedRes = renderer->GetSharedResource();
		hwRenderer = renderer->GetHardwareRenderer();
		Create(renderer);
	}


}

