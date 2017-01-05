#include "WorldRenderPass.h"
#include "Material.h"
#include "Mesh.h"
#include "CoreLib/LibIO.h"
#include "Spire/Spire.h"

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
	class ForwardBaseRenderPass : public WorldRenderPass
	{
	private:
		const char * shaderSrc = R"(
			template shader ForwardBase(passParams : ForwardBasePassParams, 
                                        lightingModule, 
                                        geometryModule : IMaterialGeometry, 
                                        materialModule : IMaterialPattern, 
                                        animationModule) targets StandardPipeline
			{
				public using VertexAttributes;
				public using passParams;
				public using animationModule;
				public using TangentSpaceTransform;
				public using geometryModule;
				public using VertexTransform;
				public using materialModule;
				vec3 lightParam = vec3(roughness, metallic, specular);
				using lighting = lightingModule(TangentSpaceToWorldSpace(vec3(normal.x, -normal.y, normal.z)));
				public out @Fragment vec4 outputColor = vec4(lighting.result, 1.0);
			};
		)";
	public:
		const char * GetShaderSource() override
		{
			return shaderSrc;
		}
		RenderTargetLayout * CreateRenderTargetLayout() override
		{
			return hwRenderer->CreateRenderTargetLayout(MakeArray(
				AttachmentLayout(TextureUsage::ColorAttachment, StorageFormat::RGBA_8),
				AttachmentLayout(TextureUsage::DepthAttachment, StorageFormat::Depth32)).GetArrayView());
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