#include "WorldRenderPass.h"
#include "Material.h"

using namespace CoreLib;

namespace GameEngine
{
	void WorldRenderPass::Bind()
	{
		sharedRes->pipelineManager.BindShader(shader, renderTargetLayout.Ptr(), &fixedFunctionStates);
	}

	CoreLib::RefPtr<WorldPassRenderTask> WorldRenderPass::CreateInstance(RenderOutput * output, bool clearOutput)
	{
		CoreLib::RefPtr<WorldPassRenderTask> result = new WorldPassRenderTask();
#ifdef _DEBUG
		if (renderPassId == -1)
			throw CoreLib::InvalidProgramException("RenderPass must be registered before calling CreateInstance().");
		if (poolAllocPtr == 32)
			throw CoreLib::InvalidProgramException("Too many RenderPassInstances created. Be sure to use ResetInstancePool() to prevent memory leak.");
#endif

		auto &rs = *result;
		rs.viewport.X = rs.viewport.Y = 0;
		rs.commandBuffer = AllocCommandBuffer();
		rs.renderPassId = renderPassId;
		rs.renderOutput = output;
		rs.fixedFunctionStates = &fixedFunctionStates;
		rs.clearOutput = clearOutput;
		return result;
	}

	AsyncCommandBuffer * WorldRenderPass::AllocCommandBuffer()
	{
		if (poolAllocPtr == commandBufferPool.Count())
		{
			commandBufferPool.Add(new AsyncCommandBuffer(hwRenderer));
		}
		return commandBufferPool[poolAllocPtr++].Ptr();
	}

	void WorldRenderPass::Create()
	{
		renderTargetLayout = CreateRenderTargetLayout();
		shader = sharedRes->LoadSpireShader(GetName(), GetShaderSource());
		SetPipelineStates(fixedFunctionStates);
	}
	WorldRenderPass::~WorldRenderPass()
	{
	}
}

