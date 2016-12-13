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
	CommandBuffer * RenderPass::AllocCommandBuffer()
	{
		if (poolAllocPtr == commandBufferPool.Count())
		{
			commandBufferPool.Add(hwRenderer->CreateCommandBuffer());
		}
		return commandBufferPool[poolAllocPtr++].Ptr();
	}
	RenderPassInstance RenderPass::CreateInstance(RenderOutput * output, void * viewUniformData, int viewUniformSize)
	{
#ifdef _DEBUG
		if (renderPassId == -1)
			throw CoreLib::InvalidProgramException("RenderPass must be registered before calling CreateInstance().");
		if (poolAllocPtr == 32)
			throw CoreLib::InvalidProgramException("Too many RenderPassInstances created. Be sure to use ResetInstancePool() to prevent memory leak.");
#endif
		RenderPassInstance rs;
		rs.viewport.X = rs.viewport.Y = 0;
		rs.commandBuffer = AllocCommandBuffer();
		rs.renderPassId = renderPassId;
		rs.renderOutput = output;
		rs.viewUniformPtr = viewUniformData;
		rs.viewUniformSize = viewUniformSize;
		return rs;
	}
}

