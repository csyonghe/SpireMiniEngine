#include "PostRenderPass.h"
#include "Mesh.h"
#include "Engine.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
	using namespace VectorMath;

	struct AtmosphereParameters
	{
		Vec3 SunDir; float padding = 0.0f;
	};
	class AtmospherePostRenderPass : public PostRenderPass
	{
	protected:
		RefPtr<RenderTarget> colorBuffer, depthBuffer, colorOutBuffer;
		RefPtr<Buffer> parameterBuffer;
		AtmosphereParameters params;
		RefPtr<Texture2D> transmittanceTex, irradianceTex;
		RefPtr<Texture3D> inscatterTex;
		bool isValid = true;
	public:
		virtual void Create() override
		{
			PostRenderPass::Create();
			parameterBuffer = hwRenderer->CreateBuffer(BufferUsage::UniformBuffer);
			String irradianceDataFile = Engine::Instance()->FindFile("Atmosphere/irradiance.raw", ResourceType::Material);
			String inscatterDataFile = Engine::Instance()->FindFile("Atmosphere/inscatter.raw", ResourceType::Material);
			String transmittanceDataFile = Engine::Instance()->FindFile("Atmosphere/transmittance.raw", ResourceType::Material);
			if (irradianceDataFile.Length() == 0 || inscatterDataFile.Length() == 0 || transmittanceDataFile.Length() == 0)
			{
				Print("missing atmosphere precompute data.\n");
				isValid = false;
				return;
			}
			else
			{
				// load precomputed textures
				{
					BinaryReader reader(new FileStream(irradianceDataFile));
					List<float> irradianceData;
					irradianceData.SetSize(16 * 64 * 3);
					reader.Read(irradianceData.Buffer(), irradianceData.Count());
					irradianceTex = hwRenderer->CreateTexture2D(TextureUsage::Sampled);
					irradianceTex->SetData(StorageFormat::RGB_F16, 64, 16, 1, DataType::Float3, irradianceData.Buffer());
				}
				{
					BinaryReader reader(new FileStream(inscatterDataFile));
					List<float> inscatterData;
					const int res = 64;
					const int nr = res / 2;
					const int nv = res * 2;
					const int nb = res / 2;
					const int na = 8;
					inscatterData.SetSize(nr * nv * nb * na * 4);
					reader.Read(inscatterData.Buffer(), inscatterData.Count());
					inscatterTex = hwRenderer->CreateTexture3D(TextureUsage::Sampled, na*nb, nv, nr, 1, StorageFormat::RGBA_F16);
					inscatterTex->SetData(0, 0, 0, 0, na*nb, nv, nr, DataType::Float4, inscatterData.Buffer());
				}
				{
					BinaryReader reader(new FileStream(transmittanceDataFile));
					List<float> transmittanceData;
					transmittanceData.SetSize(256 * 64 * 3);
					reader.Read(transmittanceData.Buffer(), transmittanceData.Count());
					transmittanceTex = hwRenderer->CreateTexture2D(TextureUsage::Sampled);
					transmittanceTex->SetData(StorageFormat::RGB_F16, 0, 256, 64, 1, DataType::Float3, transmittanceData.Buffer());
				}
			}

			params.SunDir = Vec3::Create(1.0f, 1.0f, 0.5f).Normalize();
			parameterBuffer->SetData(&params, sizeof(params));
		}
		virtual void AcquireRenderTargets() override
		{
			colorBuffer = sharedRes->LoadSharedRenderTarget("litColor", StorageFormat::RGBA_8);
			depthBuffer = sharedRes->LoadSharedRenderTarget("depthBuffer", StorageFormat::Depth24Stencil8);
			colorOutBuffer = sharedRes->LoadSharedRenderTarget("litAtmosphereColor", StorageFormat::RGBA_8);
		}
		virtual void SetupPipelineBindingLayout(PipelineBuilder * pipelineBuilder, List<TextureUsage> & renderTargets) override
		{
			renderTargets.Add(TextureUsage::ColorAttachment);

			pipelineBuilder->SetBindingLayout(0, BindingType::UniformBuffer);
			pipelineBuilder->SetBindingLayout(1, BindingType::UniformBuffer);

			int offset = sharedRes->GetTextureBindingStart();
			pipelineBuilder->SetBindingLayout(offset + 0, BindingType::Texture); // litColor
			pipelineBuilder->SetBindingLayout(offset + 1, BindingType::Texture); // depthBuffer
			pipelineBuilder->SetBindingLayout(offset + 13, BindingType::Texture); // transmittanceTex
			pipelineBuilder->SetBindingLayout(offset + 14, BindingType::Texture); // irradianceTex
			pipelineBuilder->SetBindingLayout(offset + 15, BindingType::Texture); // inscatterTex

		}
		virtual void UpdatePipelineBinding(PipelineBinding & binding, RenderAttachments & attachments) override
		{
			if (!colorBuffer->Texture)
				return;

			binding.BindUniformBuffer(0, parameterBuffer.Ptr());
			binding.BindUniformBuffer(1, sharedRes->viewUniformBuffer.Ptr());
			int offset = sharedRes->GetTextureBindingStart();
			binding.BindTexture(offset + 0, colorBuffer->Texture.Ptr(), sharedRes->nearestSampler.Ptr());
			binding.BindTexture(offset + 1, depthBuffer->Texture.Ptr(), sharedRes->nearestSampler.Ptr());
			binding.BindTexture(offset + 13, transmittanceTex.Ptr(), sharedRes->linearSampler.Ptr());
			binding.BindTexture(offset + 14, irradianceTex.Ptr(), sharedRes->linearSampler.Ptr());
			binding.BindTexture(offset + 15, inscatterTex.Ptr(), sharedRes->linearSampler.Ptr());
			attachments.SetAttachment(0, colorOutBuffer->Texture.Ptr());
		}
		virtual String GetShaderFileName() override
		{
			return "Atmosphere.shader";
		}
		virtual char * GetName() override
		{
			return "Atmosphere";
		}
	};

	PostRenderPass * CreateAtmospherePostRenderPass()
	{
		return new AtmospherePostRenderPass();
	}
}