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
				[Binding: "0"]
				public using ForwardBasePassParams;
				[Binding: "3"]
				public using ANIMATION;
				public using TangentSpaceTransform;
				public using MaterialGeometry;
				public using VertexTransform;
				[Binding: "2"]
				public using MaterialPattern;
				vec3 lightParam = vec3(roughness, metallic, specular);
				[Binding: "1"]
				using lighting = Lighting(TangentSpaceToWorldSpace(normal));
				public out @Fragment vec4 outputColor = vec4(lighting.result, 1.0);
			};
		)";
	public:
		virtual void Create() override
		{
			renderTargetLayout = hwRenderer->CreateRenderTargetLayout(MakeArray(TextureUsage::ColorAttachment, TextureUsage::DepthAttachment).GetArrayView());
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