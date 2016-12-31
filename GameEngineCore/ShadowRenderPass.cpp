#include "WorldRenderPass.h"
#include "HardwareRenderer.h"

namespace GameEngine
{
	using namespace CoreLib;

	class ShadowRenderPass : public WorldRenderPass
	{
	private:
		const char * shaderSrc = R"(
			template shader ShadowPass(passParams:ForwardBasePassParams, geometryModule:IMaterialGeometry, materialModule:IMaterialPattern, animationModule) targets StandardPipeline
			{
				public using VertexAttributes;
				public using passParams;
				public using animationModule;
				public using TangentSpaceTransform;
				public using geometryModule;
				public using VertexTransform;
				public using materialModule;
				out @Fragment vec3 ocolor = vec3(1.0);
			};
		)";
	protected:
		virtual void SetPipelineStates(FixedFunctionPipelineStates & states) override
		{
			states.DepthCompareFunc = CompareFunc::Less;
			states.EnablePolygonOffset = true;
			states.PolygonOffsetUnits = 10.0f;
			states.PolygonOffsetFactor = 2.0f;
		}
	public:
		virtual const char * GetShaderSource() override
		{
			return shaderSrc;
		}
		virtual char * GetName() override
		{
			return "ShadowPass";
		}
		virtual RenderTargetLayout * CreateRenderTargetLayout() override
		{
			return hwRenderer->CreateRenderTargetLayout(MakeArrayView(TextureUsage::DepthAttachment));
		}
	};

	WorldRenderPass * CreateShadowRenderPass()
	{
		return new ShadowRenderPass();
	}
}

