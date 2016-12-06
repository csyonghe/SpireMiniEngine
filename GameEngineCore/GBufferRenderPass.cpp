#include "WorldRenderPass.h"
#include "Material.h"
#include "Mesh.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
	class GBufferRenderPass : public WorldRenderPass
	{
	private:
		String shaderEntryPoint = R"(
			shader GBufferPass : StandardPipeline
			{
				public using VertexAttributes;
				public using SystemUniforms;
				public using ANIMATION;
				public using TangentSpaceTransform;
				public using MaterialGeometry;
				public using VertexTransform;
				public using MaterialPattern;
				vec3 lightParam = vec3(roughness, metallic, specular);
				public out @Fragment vec3 outputAlbedo = albedo;
				public out @Fragment vec3 outputPbr = lightParam;
				public out @Fragment vec3 outputNormal = TangentSpaceToWorldSpace(normal) * 0.5 + 0.5;
			};
		)";
	protected:
		RefPtr<RenderTarget> baseColorBuffer, pbrBuffer, normalBuffer, depthBuffer;
	public:
		virtual void AcquireRenderTargets() override
		{
			Array<TextureUsage, 8> bindings;
			bindings.Add(TextureUsage::ColorAttachment);
			bindings.Add(TextureUsage::ColorAttachment);
			bindings.Add(TextureUsage::ColorAttachment);
			bindings.Add(TextureUsage::DepthAttachment);
			renderTargetLayout = hwRenderer->CreateRenderTargetLayout(bindings.GetArrayView());

			baseColorBuffer = sharedRes->ProvideRenderTarget("baseColorBuffer", StorageFormat::RGBA_8);
			depthBuffer = sharedRes->ProvideOrModifyRenderTarget("depthBuffer", StorageFormat::Depth24Stencil8);
			pbrBuffer = sharedRes->ProvideRenderTarget("pbrBuffer", StorageFormat::RGBA_8);
			normalBuffer = sharedRes->ProvideRenderTarget("normalBuffer", StorageFormat::RGB10_A2);
		}
		virtual void UpdateFrameBuffer() override
		{
			if (!baseColorBuffer->Texture) return;

			RenderAttachments renderAttachments;
			renderAttachments.SetAttachment(0, baseColorBuffer->Texture.Ptr());
			renderAttachments.SetAttachment(1, pbrBuffer->Texture.Ptr());
			renderAttachments.SetAttachment(2, normalBuffer->Texture.Ptr());
			renderAttachments.SetAttachment(3, depthBuffer->Texture.Ptr());

			frameBuffer = renderTargetLayout->CreateFrameBuffer(renderAttachments);

			clearCommandBuffer->BeginRecording(renderTargetLayout.Ptr(), frameBuffer.Ptr());

			RenderAttachments clearRenderAttachments;
			clearRenderAttachments.SetAttachment(0, baseColorBuffer->Texture.Ptr());
			clearRenderAttachments.SetAttachment(1, pbrBuffer->Texture.Ptr());
			clearRenderAttachments.SetAttachment(2, normalBuffer->Texture.Ptr());
			clearRenderAttachments.SetAttachment(3, depthBuffer->Texture.Ptr());
			clearCommandBuffer->ClearAttachments(clearRenderAttachments);
			clearCommandBuffer->EndRecording();
		}
		virtual String GetEntryPointShader() override
		{
			return shaderEntryPoint;
		}
		virtual char * GetName() override
		{
			return "DeferredBase";
		}
	};

	WorldRenderPass * CreateGBufferRenderPass()
	{
		return new GBufferRenderPass();
	}
}