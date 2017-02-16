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
		const char * shaderEntryPoint = R"(
			template shader GBufferPass(passParams:ForwardBasePassParams, geometryModule:IMaterialGeometry, materialModule:IMaterialPattern, animationModule) targets StandardPipeline
			{
				public using VertexAttributes;
				public using passParams;
				public using animationModule;
				public using TangentSpaceTransform;
				public using geometryModule;
				public using VertexTransform;
				public using materialModule;
				public out @Fragment vec4 outputAlbedo
				{	 
					if (opacity < 0.01f) discard;
					return vec4(albedo, opacity);
				}
				public out @Fragment vec4 outputPbr = vec4(roughness, metallic, specular, ao);
				public out @Fragment vec4 outputNormal = vec4(TangentSpaceToWorldSpace(vec3(normal.x, -normal.y, normal.z)) * 0.5 + 0.5, (isDoubleSided ? 1.0 : 0.0));
			};
		)";
	protected:
		virtual char * GetName() override
		{
			return "GBuffer";
		}
		const char * GetShaderSource() override
		{
			return shaderEntryPoint;
		}
		RenderTargetLayout * CreateRenderTargetLayout() override
		{
			return hwRenderer->CreateRenderTargetLayout(MakeArray(
				AttachmentLayout(TextureUsage::ColorAttachment, StorageFormat::RGBA_8),
				AttachmentLayout(TextureUsage::ColorAttachment, StorageFormat::RGBA_8),
				AttachmentLayout(TextureUsage::ColorAttachment, StorageFormat::RGB10_A2),
				AttachmentLayout(TextureUsage::DepthAttachment, DepthBufferFormat)).GetArrayView());
		}
	};

	WorldRenderPass * CreateGBufferRenderPass()
	{
		return new GBufferRenderPass();
	}
}