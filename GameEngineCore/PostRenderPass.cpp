#include "PostRenderPass.h"
#include "HardwareRenderer.h"
#include "ShaderCompiler.h"
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
		pipelineBuilder->SetVertexLayout(deferredVertexFormat);
		renderTargetLayout = hwRenderer->CreateRenderTargetLayout(renderTargets.GetArrayView());
		ShaderCompilationResult rs;
		auto shaderFileName = GetShaderFileName();
		if (!CompileShader(rs, sharedRes->spireContext, hwRenderer->GetSpireTarget(), shaderFileName))
			throw HardwareRendererException("Shader compilation failure");

		for (auto& compiledShader : rs.Shaders)
		{
			Shader* shader = nullptr;
			if (compiledShader.Key == "vs")
			{
				shader = hwRenderer->CreateShader(ShaderType::VertexShader, (char*)compiledShader.Value.Buffer(), compiledShader.Value.Count());
			}
			else if (compiledShader.Key == "fs")
			{
				shader = hwRenderer->CreateShader(ShaderType::FragmentShader, (char*)compiledShader.Value.Buffer(), compiledShader.Value.Count());
			}
			shaders.Add(shader);
		}
		pipelineBuilder->SetShaders(From(shaders).Select([](auto x) { return x.Ptr(); }).ToList().GetArrayView());
		pipelineBuilder->FixedFunctionStates.PrimitiveTopology = PrimitiveType::TriangleFans;
		descLayouts.SetSize(rs.BindingLayouts.Count());
		for (auto & desc : rs.BindingLayouts)
		{
			if (desc.Value.BindingPoint >= descLayouts.Count())
				descLayouts.SetSize(desc.Value.BindingPoint + 1);
			descLayouts[desc.Value.BindingPoint] = hwRenderer->CreateDescriptorSetLayout(desc.Value.Descriptors.GetArrayView());
		}
		pipelineBuilder->SetBindingLayout(From(descLayouts).Select([](auto x) { return x.Ptr(); }).ToList().GetArrayView());
		
		SetupPipelineBindingLayout(pipelineBuilder.Ptr(), renderTargets);
		pipeline = pipelineBuilder->ToPipeline(renderTargetLayout.Ptr());
		
		commandBuffer = hwRenderer->CreateCommandBuffer();
		AcquireRenderTargets();
	}

	void PostRenderPass::Execute(SharedModuleInstances sharedModules)
	{
		commandBuffer->BeginRecording(frameBuffer.Ptr());
		if (clearFrameBuffer)
			commandBuffer->ClearAttachments(frameBuffer.Ptr());

		DescriptorSetBindings descBindings;
		UpdateDescriptorSetBinding(sharedModules, descBindings);

		commandBuffer->SetViewport(0, 0, width, height);
		commandBuffer->BindPipeline(pipeline.Ptr());
		commandBuffer->BindVertexBuffer(sharedRes->fullScreenQuadVertBuffer.Ptr(), 0);
		for (auto & binding : descBindings.bindings)
			commandBuffer->BindDescriptorSet(binding.index, binding.descriptorSet);
		commandBuffer->Draw(0, 4);
		commandBuffer->EndRecording();
		hwRenderer->ExecuteCommandBuffers(frameBuffer.Ptr(), MakeArrayView(commandBuffer.Ptr()), nullptr);
	}

	void PostRenderPass::Resize(int screenWidth, int screenHeight)
	{
		width = screenWidth;
		height = screenHeight;
		RenderAttachments renderAttachments;
		UpdateRenderAttachments(renderAttachments);
		frameBuffer = renderTargetLayout->CreateFrameBuffer(renderAttachments);
	}
}

