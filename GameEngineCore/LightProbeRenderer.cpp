#include "LightProbeRenderer.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace CoreLib;

	class Renderer;
	IRenderProcedure * CreateStandardRenderProcedure();

	LightProbeRenderer::LightProbeRenderer(Renderer * prenderer, RendererService * prenderService, IRenderProcedure * pRenderProc, ViewResource * pViewRes)
	{
		renderer = prenderer;
		renderService = prenderService;
		renderProc = pRenderProc;
		viewRes = pViewRes;
	}

	const char * spirePipeline = R"(
			pipeline EnginePipeline
			{
				[Pinned]
				input world rootVert;

				world vs;
				world fs;

				require @vs vec4 projCoord;
				
				[VertexInput]
				extern @vs rootVert vertAttribIn;
				import(rootVert->vs) vertexImport()
				{
					return project(vertAttribIn);
				}

				extern @fs vs vsIn;
				import(vs->fs) standardImport()
				{
					return project(vsIn);
				}
    
				stage vs : VertexShader
				{
					World: vs;
					Position: projCoord;
				}
    
				stage fs : FragmentShader
				{
					World: fs;
				}
			}
		)";
	const char * copyPixelShader = R"(
			module CopyParams
			{
				public param Texture2D srcTex;
				public param SamplerState nearestSampler;
			}
			shader CopyPixelShader
			{
				[Binding: "0"]
				public using CopyParams;

				@rootVert vec2 vert_pos;
				@rootVert vec2 vert_uv;
				
				vec4 projCoord = vec4(vert_pos, 0.0, 1.0);

				public out @fs vec4 color
				{
					vec4 result = srcTex.Sample(nearestSampler, vert_uv);
					return result;
				}
			}
		)";
	class ShaderSet
	{
	public:
		RefPtr<Shader> vertexShader, fragmentShader;
	};
	ShaderSet CompileShader(PipelineBuilder * pb, HardwareRenderer * hw, const char * src)
	{
		SpireCompilationContext * ctx = spCreateCompilationContext("");
		auto rs = spCompileShaderFromSource(ctx, (String(spirePipeline) + src).Buffer(), "", nullptr);
		int vsLen, fsLen;
		auto vs = spGetShaderStageSource(rs, nullptr, "vs", &vsLen);
		auto fs = spGetShaderStageSource(rs, nullptr, "fs", &fsLen);
		ShaderSet set;
		set.vertexShader = hw->CreateShader(ShaderType::VertexShader, vs, vsLen);
		set.fragmentShader = hw->CreateShader(ShaderType::FragmentShader, fs, fsLen);
		pb->SetShaders(MakeArray(set.vertexShader.Ptr(), set.fragmentShader.Ptr()).GetArrayView());
		spDestroyCompilationResult(rs);
		spDestroyCompilationContext(ctx);
		return set;
	}

	RefPtr<TextureCube> LightProbeRenderer::RenderLightProbe(Level * level, VectorMath::Vec3 position)
	{
		HardwareRenderer * hw = renderer->GetHardwareRenderer();
		int resolution = viewRes->GetWidth();

		RefPtr<TextureCube> rs = hw->CreateTextureCube(TextureUsage::SampledColorAttachment, resolution, Math::Log2Ceil(resolution), StorageFormat::RGBA_F16);
		viewRes->Resize(resolution, resolution);
		RenderStat stat;
		FrameRenderTask task;
		RenderProcedureParameters params;
		params.level = level;
		params.renderer = renderer;
		params.rendererService = renderService;
		params.renderStats = &stat;
		params.view.FOV = 90.0f;
		params.view.ZFar = 40000.0f;
		params.view.ZNear = 20.0f;
		params.view.Position = position;
		
		auto sharedRes = renderer->GetSharedResource();
		RefPtr<PipelineBuilder> pb = hw->CreatePipelineBuilder();
		VertexFormat quadVert;
		pb->FixedFunctionStates.PrimitiveTopology = PrimitiveType::TriangleFans;
		quadVert.Attributes.Add(VertexAttributeDesc(DataType::Float2, 0, 0, 0));
		quadVert.Attributes.Add(VertexAttributeDesc(DataType::Float2, 0, 8, 1));
		pb->SetVertexLayout(quadVert);
		RefPtr<DescriptorSetLayout> copyPassLayout = hw->CreateDescriptorSetLayout(MakeArray(DescriptorLayout(StageFlags::sfGraphics, 0, BindingType::Texture, 0),
			DescriptorLayout(StageFlags::sfGraphics, 0, BindingType::Sampler, 1)).GetArrayView());
		pb->SetBindingLayout(copyPassLayout.Ptr());
		auto copyShaderSet = CompileShader(pb.Ptr(), hw, copyPixelShader);
		RefPtr<RenderTargetLayout> copyRTLayout = hw->CreateRenderTargetLayout(MakeArrayView(AttachmentLayout(TextureUsage::ColorAttachment, StorageFormat::RGBA_F16)));
		RefPtr<Pipeline> copyPipeline = pb->ToPipeline(copyRTLayout.Ptr());
		RefPtr<DescriptorSet> copyDescSet = hw->CreateDescriptorSet(copyPassLayout.Ptr());
		RefPtr<FrameBuffer> frameBuffers[6];
		RefPtr<CommandBuffer> cmdBuffers[6];
		for (int f = 0; f < 6; f++)
		{
			Matrix4 viewMatrix;
			Matrix4::CreateIdentityMatrix(viewMatrix);
			Matrix4 mm1, mm2;
			switch (f)
			{
			case 0:
			{
				Matrix4::RotationY(mm1, Math::Pi * 0.5f);
				Matrix4::RotationX(mm2, Math::Pi);
				Matrix4::Multiply(viewMatrix, mm1, mm2);
				break;
			}
			case 1:
			{
				Matrix4::RotationY(mm1, -Math::Pi * 0.5f);
				Matrix4::RotationX(mm2, Math::Pi);
				Matrix4::Multiply(viewMatrix, mm1, mm2);
				break;
			}
			case 2:
			{
				Matrix4::RotationX(viewMatrix, -Math::Pi * 0.5f);
				break;
			}
			case 3:
			{
				Matrix4::RotationX(viewMatrix, Math::Pi * 0.5f);
				break;
			}
			case 4:
			{
				Matrix4::RotationX(viewMatrix, Math::Pi);
				break;
			}
			case 5:
			{
				Matrix4::RotationZ(viewMatrix, Math::Pi);
				break;
			}
			}

			viewMatrix.values[12] = -(viewMatrix.values[0] * position.x + viewMatrix.values[4] * position.y + viewMatrix.values[8] * position.z);
			viewMatrix.values[13] = -(viewMatrix.values[1] * position.x + viewMatrix.values[5] * position.y + viewMatrix.values[9] * position.z);
			viewMatrix.values[14] = -(viewMatrix.values[2] * position.x + viewMatrix.values[6] * position.y + viewMatrix.values[10] * position.z);

			hw->BeginDataTransfer();
			params.view.Transform = viewMatrix;
			renderProc->Run(task, params);
			hw->EndDataTransfer();
			for (auto & pass : task.renderPasses)
				pass.Execute(hw);

			copyDescSet->BeginUpdate();
			copyDescSet->Update(0, renderProc->GetOutput()->Texture.Ptr(), TextureAspect::Color);
			copyDescSet->Update(1, sharedRes->nearestSampler.Ptr());
			copyDescSet->EndUpdate();
			RenderAttachments attachments;
			attachments.SetAttachment(0, rs.Ptr(), (TextureCubeFace)f, 0);
			RefPtr<FrameBuffer> fb = copyRTLayout->CreateFrameBuffer(attachments);
			frameBuffers[f] = fb;
			cmdBuffers[f] = hw->CreateCommandBuffer();
			auto cmdBuffer = cmdBuffers[f].Ptr();
			cmdBuffer->BeginRecording();
			cmdBuffer->SetViewport(0, 0, resolution, resolution);
			cmdBuffer->BindPipeline(copyPipeline.Ptr());
			cmdBuffer->BindDescriptorSet(0, copyDescSet.Ptr());
			cmdBuffer->BindVertexBuffer(sharedRes->fullScreenQuadVertBuffer.Ptr(), 0);
			cmdBuffer->Draw(0, 4);
			cmdBuffer->EndRecording();
			hw->ExecuteCommandBuffers(fb.Ptr(), MakeArrayView(cmdBuffer), nullptr);
			hw->Wait();
		}
		return rs;
	}
}
