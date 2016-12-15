#include "RenderPass.h"
#include "RenderContext.h"
#include <assert.h>

namespace GameEngine
{
	void RenderPass::Init(RendererSharedResource * pSharedRes, SceneResource * pSceneRes)
	{
		sharedRes = pSharedRes;
		sceneRes = pSceneRes;
		hwRenderer = pSharedRes->hardwareRenderer.Ptr();
		Create();
	}


}

