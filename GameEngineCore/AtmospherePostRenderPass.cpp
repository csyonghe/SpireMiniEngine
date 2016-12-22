#include "PostRenderPass.h"
#include "Mesh.h"
#include "Engine.h"
#include "CoreLib/LibIO.h"
#include "Atmosphere.h"
#include <assert.h>

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
	using namespace VectorMath;

	class AtmospherePostRenderPass : public PostRenderPass
	{
	protected:
		RefPtr<RenderTarget> colorBuffer, depthBuffer, colorOutBuffer;
		RefPtr<Buffer> parameterBuffer;
		
		RefPtr<Texture2D> transmittanceTex, irradianceTex;
		RefPtr<Texture3D> inscatterTex;

		RefPtr<DescriptorSet> atmosphereDesc;
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

			// initialize parameter buffer with default params
			AtmosphereParameters defaultParams;
			defaultParams.SunDir = Vec3::Create(1.0f, 1.0f, 0.5f).Normalize();
			parameterBuffer->SetData(&defaultParams, sizeof(defaultParams));
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
			pipelineBuilder->SetDebugName("atmosphere");

			atmosphereDesc = hwRenderer->CreateDescriptorSet(descLayouts[0].Ptr());
		}
		virtual void UpdatePipelineBinding(SharedModuleInstances sharedModules, DescriptorSetBindings & binding, RenderAttachments & attachments) override
		{
			if (!colorBuffer->Texture)
				return;
			binding.Bind(0, atmosphereDesc.Ptr());
			binding.Bind(1, sharedModules.View->Descriptors.Ptr());

			atmosphereDesc->BeginUpdate();
			atmosphereDesc->Update(0, parameterBuffer.Ptr());
			atmosphereDesc->Update(1, colorBuffer->Texture.Ptr());
			atmosphereDesc->Update(2, depthBuffer->Texture.Ptr());
			atmosphereDesc->Update(3, transmittanceTex.Ptr());
			atmosphereDesc->Update(4, irradianceTex.Ptr());
			atmosphereDesc->Update(5, inscatterTex.Ptr());
			atmosphereDesc->Update(6, sharedRes->linearSampler.Ptr());
			atmosphereDesc->Update(7, sharedRes->nearestSampler.Ptr());
			atmosphereDesc->EndUpdate();

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
		virtual void SetParameters(void * data, int count) override
		{
			assert(count == sizeof(AtmosphereParameters));
			parameterBuffer->SetData(data, count);
		}
	};

	PostRenderPass * CreateAtmospherePostRenderPass()
	{
		return new AtmospherePostRenderPass();
	}
}