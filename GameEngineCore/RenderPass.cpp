#include "RenderPass.h"
#include "RenderContext.h"

namespace GameEngine
{
	void RenderPass::Create()
	{
		clearCommandBuffer = hwRenderer->CreateCommandBuffer();
		AcquireRenderTargets();
		UpdateFrameBuffer();
	}
	void RenderPass::Init(RendererSharedResource * pSharedRes, SceneResource * pSceneRes)
	{
		sharedRes = pSharedRes;
		sceneRes = pSceneRes;
		hwRenderer = pSharedRes->hardwareRenderer.Ptr();
		Create();
	}
}

