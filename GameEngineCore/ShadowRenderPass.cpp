#include "WorldRenderPass.h"
#include "HardwareRenderer.h"

namespace GameEngine
{
	using namespace CoreLib;

	const char * shadowShaderSrc = R"(
		shader ShadowPass : StandardPipeline
		{
			public using VertexAttributes;
			public using SystemUniforms;
			public using ANIMATION;
			public using TangentSpaceTransform;
			public using MaterialGeometry;
			public using VertexTransform;
			// public using MaterialPattern;
		};
	)";
	class ShadowRenderPass : public WorldRenderPass
	{
	private:
		CoreLib::String shaderSrc = R"(
			shader ShadowPass targets StandardPipeline
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
				out @Fragment vec3 ocolor = vec3(1.0);
			};
		)";
	protected:
		virtual void SetPipelineStates(PipelineBuilder * pb)
		{
			pb->FixedFunctionStates.EnablePolygonOffset = true;
			pb->FixedFunctionStates.PolygonOffsetUnits = 10.0f;
			pb->FixedFunctionStates.PolygonOffsetFactor = 2.0f;
		}
	public:
		virtual CoreLib::String GetEntryPointShader()
		{
			return shaderSrc;
		}
		virtual char * GetName() override
		{
			return "ShadowPass";
		}
		virtual void Create() override
		{
			renderTargetLayout = hwRenderer->CreateRenderTargetLayout(MakeArrayView(TextureUsage::DepthAttachment));
		}
	};

	WorldRenderPass * CreateShadowRenderPass()
	{
		return new ShadowRenderPass();
	}
}

