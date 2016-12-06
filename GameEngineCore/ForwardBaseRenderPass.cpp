#include "WorldRenderPass.h"
#include "Material.h"
#include "Mesh.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
	class ForwardBaseRenderPass : public WorldRenderPass
	{
	private:
		String shaderEntryPoint = R"(
			shader ForwardBase : StandardPipeline
			{
				public using VertexAttributes;
				public using SystemUniforms;
				public using ANIMATION;
				public using TangentSpaceTransform;
				public using MaterialGeometry;
				public using VertexTransform;
				public using MaterialPattern;
				vec3 lightParam = vec3(roughness, metallic, specular);
				using lighting = Lighting(TangentSpaceToWorldSpace(normal));
				public out @Fragment vec4 outputColor = vec4(lighting.result, 1.0);
			};
		)";
	protected:
		RefPtr<RenderTarget> colorBuffer, depthBuffer;
	public:
		virtual void AcquireRenderTargets() override
		{
			Array<TextureUsage, 2> bindings;
			bindings.Add(TextureUsage::ColorAttachment);
			bindings.Add(TextureUsage::DepthAttachment);
			renderTargetLayout = hwRenderer->CreateRenderTargetLayout(bindings.GetArrayView());

			colorBuffer = sharedRes->ProvideRenderTarget("litColor", StorageFormat::RGBA_8);
			depthBuffer = sharedRes->ProvideOrModifyRenderTarget("depthBuffer", StorageFormat::Depth24Stencil8);
		}
		virtual void UpdateFrameBuffer() override
		{
			if (!colorBuffer->Texture) return;

			RenderAttachments renderAttachments;
			renderAttachments.SetAttachment(0, colorBuffer->Texture.Ptr());
			renderAttachments.SetAttachment(1, depthBuffer->Texture.Ptr());
			frameBuffer = renderTargetLayout->CreateFrameBuffer(renderAttachments);

			clearCommandBuffer->BeginRecording(renderTargetLayout.Ptr(), frameBuffer.Ptr());
			clearCommandBuffer->ClearAttachments(renderAttachments);
			clearCommandBuffer->EndRecording();
		}
		virtual String GetEntryPointShader() override
		{
			return shaderEntryPoint;
		}
		virtual char * GetName() override
		{
			return "ForwardBase";
		}
	};

	WorldRenderPass * CreateForwardBaseRenderPass()
	{
		return new ForwardBaseRenderPass();
	}
}