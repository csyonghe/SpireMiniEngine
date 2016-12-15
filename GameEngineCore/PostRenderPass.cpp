#include "PostRenderPass.h"
#include "HardwareRenderer.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
	void PostRenderPass::Create()
	{
		// Create deferred pipeline
		VertexFormat deferredVertexFormat;
		deferredVertexFormat.Attributes.Add(VertexAttributeDesc(DataType::Float2, 0, 0, 0));
		deferredVertexFormat.Attributes.Add(VertexAttributeDesc(DataType::Float2, 0, 2 * sizeof(float), 1));
		RefPtr<PipelineBuilder> pipelineBuilder = hwRenderer->CreatePipelineBuilder();
		List<TextureUsage> renderTargets;
		SetupPipelineBindingLayout(pipelineBuilder.Ptr(), renderTargets);
		pipelineBuilder->SetVertexLayout(deferredVertexFormat);
		renderTargetLayout = hwRenderer->CreateRenderTargetLayout(renderTargets.GetArrayView());
		CoreLib::List<Shader*> shaderList;
		ShaderCompilationResult rs;
		auto shaderFileName = GetShaderFileName();
		if (!sharedRes->hardwareFactory->CompileShader(rs, shaderFileName, "", "", ""))
			throw HardwareRendererException("Shader compilation failure");

		for (auto& compiledShader : rs.Shaders)
		{
			Shader* shader;
			if (compiledShader.Key == "vs")
			{
				shader = sceneRes->LoadShader(Path::ReplaceExt(shaderFileName, compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::VertexShader);
			}
			else if (compiledShader.Key == "fs")
			{
				shader = sceneRes->LoadShader(Path::ReplaceExt(shaderFileName, compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::FragmentShader);
			}
			shaderList.Add(shader);
		}
		pipelineBuilder->SetShaders(shaderList.GetArrayView());
		pipelineBuilder->FixedFunctionStates.PrimitiveTopology = PrimitiveType::TriangleFans;
		pipeline = pipelineBuilder->ToPipeline(renderTargetLayout.Ptr());

		commandBuffer = hwRenderer->CreateCommandBuffer();
		AcquireRenderTargets();
	}

	void PostRenderPass::Execute()
	{
		hwRenderer->ExecuteCommandBuffers(frameBuffer.Ptr(), MakeArrayView(commandBuffer.Ptr()));
	}

	void PostRenderPass::RecordCommandBuffer(int screenWidth, int screenHeight)
	{
		PipelineBinding pipelineBinding;
		RenderAttachments renderAttachments;
		UpdatePipelineBinding(pipelineBinding, renderAttachments);
		pipelineInstance = pipeline->CreateInstance(pipelineBinding);
		frameBuffer = renderTargetLayout->CreateFrameBuffer(renderAttachments);
		commandBuffer->BeginRecording(frameBuffer.Ptr());
		if (clearFrameBuffer)
			commandBuffer->ClearAttachments(frameBuffer.Ptr());

		commandBuffer->SetViewport(0, 0, screenWidth, screenHeight);
		commandBuffer->BindVertexBuffer(sharedRes->fullScreenQuadVertBuffer.Ptr());
		commandBuffer->BindPipeline(pipelineInstance.Ptr());
		commandBuffer->Draw(0, 4);
		commandBuffer->EndRecording();
	}
}

