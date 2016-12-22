#include "RenderPass.h"
#include "RenderContext.h"
#include <assert.h>

namespace GameEngine
{
	void RenderPass::Init(RendererSharedResource * pSharedRes)
	{
		sharedRes = pSharedRes;
		hwRenderer = pSharedRes->hardwareRenderer.Ptr();
		Create();
	}


}

