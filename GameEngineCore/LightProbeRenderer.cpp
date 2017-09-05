#include "LightProbeRenderer.h"
#include "Engine.h"


namespace GameEngine
{
	using namespace CoreLib;

	class Renderer;

	LightProbeRenderer::LightProbeRenderer(Renderer * prenderer, RendererService * prenderService, IRenderProcedure * pRenderProc, ViewResource * pViewRes)
	{
		renderer = prenderer;
		renderService = prenderService;
		renderProc = pRenderProc;
		viewRes = pViewRes;
		tempEnv = prenderer->GetHardwareRenderer()->CreateTextureCube(TextureUsage::SampledColorAttachment, EnvMapSize, Math::Log2Ceil(EnvMapSize) + 1, StorageFormat::RGBA_F16);
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
	const char * prefilterShader = R"(
		module PrefilterParams
		{
			public param vec4 origin;
			public param vec4 s;
			public param vec4 t;
			public param vec4 r;
			public param float roughness;
			public param TextureCube envMap;
			public param SamplerState nearestSampler;
		}

		float radicalInverse_VdC(uint bits)
		{
			 bits = (bits << 16) | (bits >> 16);
			 bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
			 bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
			 bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
			 bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
			 return float(bits) * 2.3283064365386963e-10; // / 0x100000000
		 }

		 vec2 hammersley(uint i, uint N)
		 {
			 return vec2(float(i)/float(N), radicalInverse_VdC(i));
		 }

		vec3 importanceSampleGGX(vec2 xi, float roughness, vec3 N)
		{
			float a = roughness * roughness;
			float phi = 2 * 3.1415926f * xi.x;
			float cosTheta = sqrt( (1-xi.y)/(1+(a*a-1)*xi.y));
			float sinTheta = sqrt(1-cosTheta*cosTheta);
			vec3 H = vec3(sinTheta * cos(phi), sinTheta*sin(phi), cosTheta);
			vec3 upVector = abs(N.z) < 0.9 ? vec3(0,0,1) : vec3(1, 0 , 0);
			vec3 tangentX = normalize(cross(upVector, N));
			vec3 tangentY = cross(N, tangentX);
			return tangentX * H.x + tangentY * H.y + N * H.z;
		}

		shader Prefilter
		{
			[Binding: "0"]
			public using PrefilterParams;
			@rootVert vec2 vert_pos;
			@rootVert vec2 vert_uv;
			public vec4 projCoord = vec4(vert_pos, 0.0, 1.0);
			public out @fs vec3 color
			{
				vec3 coord = origin.xyz + s.xyz * vert_uv.x * 2.0 + t.xyz * vert_uv.y * 2.0;
				vec3 N = coord;
				vec3 V = coord;
				vec3 prefilteredColor = vec3(0.0);
				int numSamples = 1024;
				float totalWeight = 0.001;
				for (int i = 0; i < numSamples; i++)
				{
					vec2 xi = hammersley(uint(i), uint(numSamples));
					vec3 h = importanceSampleGGX(xi, roughness, N);
					vec3 L = h * (2.0*dot(V, h)) - V;
					float NoL = clamp(dot(N,L), 0.0f, 1.0f);
					if (NoL > 0)
					{
						prefilteredColor += envMap.Sample(nearestSampler, L).xyz * NoL;
						totalWeight += NoL;
					}            
				}
				return prefilteredColor / totalWeight;
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
		spSetCodeGenTarget(ctx, hw->GetSpireTarget());
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

	struct PrefilterUniform
	{
		Vec4 origin;
		Vec4 s, t, r;
		float roughness;
	};

	void LightProbeRenderer::RenderLightProbe(TextureCubeArray* dest, int id, Level * level, VectorMath::Vec3 position)
	{
		HardwareRenderer * hw = renderer->GetHardwareRenderer();
		int resolution = EnvMapSize;
		int numLevels = Math::Log2Ceil(resolution) + 1;
		RenderStat stat;
		ImageTransferRenderTask t1(MakeArrayView(dynamic_cast<Texture*>(tempEnv.Ptr())), ArrayView<Texture*>());
		t1.Execute(hw, stat);

		ImageTransferRenderTask t3(MakeArrayView(dynamic_cast<Texture*>(dest)), ArrayView<Texture*>());
		t3.Execute(hw, stat);

		viewRes->Resize(resolution, resolution);
		FrameRenderTask task;
		RenderProcedureParameters params;
		params.level = level;
		params.renderer = renderer;
		params.rendererService = renderService;
		params.renderStats = &stat;
		params.view.FOV = 90.1f;
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
		RefPtr<DescriptorSetLayout> copyPassLayout = hw->CreateDescriptorSetLayout(MakeArray(
			DescriptorLayout(StageFlags::sfGraphics, 0, BindingType::Texture, 0),
			DescriptorLayout(StageFlags::sfGraphics, 1, BindingType::Sampler, renderer->GetHardwareRenderer()->GetSpireTarget()==SPIRE_GLSL?0:1)).GetArrayView());
		pb->SetBindingLayout(copyPassLayout.Ptr());
		auto copyShaderSet = CompileShader(pb.Ptr(), hw, copyPixelShader);
		RefPtr<RenderTargetLayout> copyRTLayout = hw->CreateRenderTargetLayout(MakeArrayView(AttachmentLayout(TextureUsage::ColorAttachment, StorageFormat::RGBA_F16)));
		RefPtr<Pipeline> copyPipeline = pb->ToPipeline(copyRTLayout.Ptr());
		RefPtr<DescriptorSet> copyDescSet = hw->CreateDescriptorSet(copyPassLayout.Ptr());
		RefPtr<FrameBuffer> frameBuffers[6];
		List<RefPtr<CommandBuffer>> commandBuffers;

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
			task.Clear();
			renderProc->Run(task, params);
			hw->EndDataTransfer();
			for (auto & pass : task.GetTasks())
				pass->Execute(hw, sharedRes->renderStats);

			copyDescSet->BeginUpdate();
			copyDescSet->Update(0, renderProc->GetOutput()->Texture.Ptr(), TextureAspect::Color);
			copyDescSet->Update(1, sharedRes->nearestSampler.Ptr());
			copyDescSet->EndUpdate();
			RefPtr<FrameBuffer> fb0, fb1;
			// copy to level 0 of tempEnv
			{
				RenderAttachments attachments;
				attachments.SetAttachment(0, tempEnv.Ptr(), (TextureCubeFace)f, 0);
				fb0 = copyRTLayout->CreateFrameBuffer(attachments);
				frameBuffers[f] = fb0;
				auto cmdBuffer = hw->CreateCommandBuffer();
				cmdBuffer->BeginRecording(fb0.Ptr());
				cmdBuffer->SetViewport(0, 0, resolution, resolution);
				cmdBuffer->BindPipeline(copyPipeline.Ptr());
				cmdBuffer->BindDescriptorSet(0, copyDescSet.Ptr());
				cmdBuffer->BindVertexBuffer(sharedRes->fullScreenQuadVertBuffer.Ptr(), 0);
				cmdBuffer->Draw(0, 4);
				cmdBuffer->EndRecording();
				commandBuffers.Add(cmdBuffer);
				hw->ExecuteRenderPass(fb0.Ptr(), MakeArrayView(cmdBuffer), nullptr);
			}
			// copy to level 0 of result
			{
				RenderAttachments attachments;
				attachments.SetAttachment(0, tempEnv.Ptr(), (TextureCubeFace)f, 0);
				fb1 = copyRTLayout->CreateFrameBuffer(attachments);
				frameBuffers[f] = fb1;
				auto cmdBuffer = hw->CreateCommandBuffer();
				cmdBuffer->BeginRecording(fb1.Ptr());
				cmdBuffer->SetViewport(0, 0, resolution, resolution);
				cmdBuffer->BindPipeline(copyPipeline.Ptr());
				cmdBuffer->BindDescriptorSet(0, copyDescSet.Ptr());
				cmdBuffer->BindVertexBuffer(sharedRes->fullScreenQuadVertBuffer.Ptr(), 0);
				cmdBuffer->Draw(0, 4);
				cmdBuffer->EndRecording();
				commandBuffers.Add(cmdBuffer);
				hw->ExecuteRenderPass(fb1.Ptr(), MakeArrayView(cmdBuffer), nullptr);
			}
			hw->Wait();
		}
		ImageTransferRenderTask t2(ArrayView<Texture*>(), MakeArrayView(dynamic_cast<Texture*>(tempEnv.Ptr())));
		t2.Execute(hw, stat);

		// prefilter
		RefPtr<PipelineBuilder> pb2 = hw->CreatePipelineBuilder();
		pb2->FixedFunctionStates.PrimitiveTopology = PrimitiveType::TriangleFans;
		pb2->SetVertexLayout(quadVert);
		RefPtr<DescriptorSetLayout> prefilterPassLayout = hw->CreateDescriptorSetLayout(MakeArray(
			DescriptorLayout(StageFlags::sfGraphics, 0, BindingType::UniformBuffer, 0),
			DescriptorLayout(StageFlags::sfGraphics, 1, BindingType::Texture, renderer->GetHardwareRenderer()->GetSpireTarget() == SPIRE_GLSL ? 0 : 1),
			DescriptorLayout(StageFlags::sfGraphics, 2, BindingType::Sampler, renderer->GetHardwareRenderer()->GetSpireTarget() == SPIRE_GLSL ? 0 : 2)).GetArrayView());
		RefPtr<Buffer> uniformBuffer = hw->CreateMappedBuffer(BufferUsage::UniformBuffer, sizeof(PrefilterUniform));
		pb2->SetBindingLayout(prefilterPassLayout.Ptr());
		auto prefilterShaderSet = CompileShader(pb2.Ptr(), hw, prefilterShader);
		RefPtr<RenderTargetLayout> prefilterRTLayout = hw->CreateRenderTargetLayout(MakeArrayView(AttachmentLayout(TextureUsage::ColorAttachment, StorageFormat::RGBA_F16)));
		RefPtr<Pipeline> prefilterPipeline = pb2->ToPipeline(prefilterRTLayout.Ptr());
		RefPtr<DescriptorSet> prefilterDescSet = hw->CreateDescriptorSet(prefilterPassLayout.Ptr());
		prefilterDescSet->BeginUpdate();
		prefilterDescSet->Update(0, uniformBuffer.Ptr());
		prefilterDescSet->Update(1, tempEnv.Ptr(), TextureAspect::Color);
		prefilterDescSet->Update(2, sharedRes->nearestSampler.Ptr());
		prefilterDescSet->EndUpdate();

		
		for (int f = 0; f < 6; f++)
		{
			PrefilterUniform prefilterParams;
			switch (f)
			{
			case 0:
				prefilterParams.origin = Vec4::Create(1.0f, 1.0f, 1.0f, 0.0f);
				prefilterParams.r = Vec4::Create(1.0f, 0.0f, 0.0f, 0.0f);
				prefilterParams.s = Vec4::Create(0.0f, 0.0f, -1.0f, 0.0f);
				prefilterParams.t = Vec4::Create(0.0f, -1.0f, 0.0f, 0.0f);
				break;
			case 1:
				prefilterParams.origin = Vec4::Create(-1.0f, 1.0f, -1.0f, 0.0f);
				prefilterParams.r = Vec4::Create(1.0f, 0.0f, 0.0f, 0.0f);
				prefilterParams.s = Vec4::Create(0.0f, 0.0f, 1.0f, 0.0f);
				prefilterParams.t = Vec4::Create(0.0f, -1.0f, 0.0f, 0.0f);
				break;
			case 2:
				prefilterParams.origin = Vec4::Create(-1.0f, 1.0f, -1.0f, 0.0f);
				prefilterParams.r = Vec4::Create(0.0f, 1.0f, 0.0f, 0.0f);
				prefilterParams.s = Vec4::Create(1.0f, 0.0f, 0.0f, 0.0f);
				prefilterParams.t = Vec4::Create(0.0f, 0.0f, 1.0f, 0.0f);
				break;
			case 3:
				prefilterParams.origin = Vec4::Create(-1.0f, -1.0f, 1.0f, 0.0f);
				prefilterParams.r = Vec4::Create(0.0f, -1.0f, 0.0f, 0.0f);
				prefilterParams.s = Vec4::Create(1.0f, 0.0f, 0.0f, 0.0f);
				prefilterParams.t = Vec4::Create(0.0f, 0.0f, -1.0f, 0.0f);
				break;
			case 4:
				prefilterParams.origin = Vec4::Create(-1.0f, 1.0f, 1.0f, 0.0f);
				prefilterParams.r = Vec4::Create(0.0f, 0.0f, 1.0f, 0.0f);
				prefilterParams.s = Vec4::Create(1.0f, 0.0f, 0.0f, 0.0f);
				prefilterParams.t = Vec4::Create(0.0f, -1.0f, 0.0f, 0.0f);
				break;
			case 5:
				prefilterParams.origin = Vec4::Create(1.0f, 1.0f, -1.0f, 0.0f);
				prefilterParams.r = Vec4::Create(0.0f, 0.0f, -1.0f, 0.0f);
				prefilterParams.s = Vec4::Create(-1.0f, 0.0f, 0.0f, 0.0f);
				prefilterParams.t = Vec4::Create(0.0f, -1.0f, 0.0f, 0.0f);
				break;
			}
			for (int l = 1; l < numLevels; l++)
			{
				prefilterParams.roughness = (l / (float)(numLevels - 1));
				hw->BeginDataTransfer();
				uniformBuffer->SetData(&prefilterParams, sizeof(prefilterParams));
				hw->EndDataTransfer();

				RenderAttachments attachments;
				attachments.SetAttachment(0, dest, id, (TextureCubeFace)f, l);
				RefPtr<FrameBuffer> fb = prefilterRTLayout->CreateFrameBuffer(attachments);
				auto cmdBuffer = hw->CreateCommandBuffer();
				cmdBuffer->BeginRecording(fb.Ptr());
				cmdBuffer->SetViewport(0, 0, resolution >> l, resolution >> l);
				cmdBuffer->BindPipeline(prefilterPipeline.Ptr());
				cmdBuffer->BindDescriptorSet(0, prefilterDescSet.Ptr());
				cmdBuffer->BindVertexBuffer(sharedRes->fullScreenQuadVertBuffer.Ptr(), 0);
				cmdBuffer->Draw(0, 4);
				cmdBuffer->EndRecording();
				commandBuffers.Add(cmdBuffer);
				hw->ExecuteRenderPass(fb.Ptr(), cmdBuffer, nullptr);
				hw->Wait();
			}
		}
		ImageTransferRenderTask t4(ArrayView<Texture*>(), MakeArrayView(dynamic_cast<Texture*>(dest)));
		t4.Execute(hw, stat);
		hw->Wait();
	}
}
