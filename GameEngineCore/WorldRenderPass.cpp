#include "WorldRenderPass.h"
#include "Material.h"

using namespace CoreLib;

namespace GameEngine
{
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

	CoreLib::RefPtr<PipelineClass> WorldRenderPass::CreatePipelineStateObject(SceneResource * sceneRes, Material * material, Mesh * mesh, DrawableType drawableType)
	{
		auto animModule = drawableType == DrawableType::Skeletal ? "SkeletalAnimation" : "NoAnimation";
		auto entryPoint = GetEntryPointShader().ReplaceAll("ANIMATION", animModule);
		StringBuilder identifierSB(128);
		identifierSB << material->ShaderFile << GetName() << "_" << mesh->GetVertexFormat().GetTypeId();
		auto identifier = identifierSB.ProduceString();

		auto meshVertexFormat = mesh->GetVertexFormat();

		auto pipelineClass = sceneRes->LoadMaterialPipeline(identifier, material, renderTargetLayout.Ptr(), meshVertexFormat, entryPoint, 
			[this](PipelineBuilder* pb) {SetPipelineStates(pb); });

		return pipelineClass;
	}
}

