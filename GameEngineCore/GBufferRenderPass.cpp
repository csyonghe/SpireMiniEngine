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
				vec3 lightParam = vec3(roughness, metallic, specular);
				public out @Fragment vec3 outputAlbedo = albedo;
				public out @Fragment vec3 outputPbr = lightParam;
				public out @Fragment vec3 outputNormal = TangentSpaceToWorldSpace(vec3(normal.x, -normal.y, normal.z)) * 0.5 + 0.5;
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