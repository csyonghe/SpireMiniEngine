#include "WorldRenderPass.h"
#include "Material.h"

using namespace CoreLib;

namespace GameEngine
{
	void WorldRenderPass::Bind()
	{
		sharedRes->pipelineManager.BindShader(shader, renderTargetLayout.Ptr(), &fixedFunctionStates);
	}

	RenderPassInstance WorldRenderPass::CreateInstance(RenderOutput * output)
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
		rs.fixedFunctionStates = &fixedFunctionStates;
		return rs;
	}

	CommandBuffer * WorldRenderPass::AllocCommandBuffer()
	{
		if (poolAllocPtr == commandBufferPool.Count())
		{
			commandBufferPool.Add(hwRenderer->CreateCommandBuffer());
		}
		return commandBufferPool[poolAllocPtr++].Ptr();
	}

	void WorldRenderPass::Create()
	{
		renderTargetLayout = hwRenderer->CreateRenderTargetLayout(MakeArray(TextureUsage::ColorAttachment, TextureUsage::DepthAttachment).GetArrayView());
		shader = spCreateShaderFromSource(sharedRes->spireContext, GetShaderSource());
		SetPipelineStates(fixedFunctionStates);
	}
	WorldRenderPass::~WorldRenderPass()
	{
		if (shader)
			spDestroyShader(shader);
	}
}

