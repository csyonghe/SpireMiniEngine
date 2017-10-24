#include "WorldRenderPass.h"
#include "HardwareRenderer.h"

namespace GameEngine
{
    using namespace CoreLib;

    class CustomDepthRenderPass : public WorldRenderPass
    {
    private:
        const char * shaderSrc = R"(
			template shader CustomDepthPass(
                passParams:ForwardBasePassParams, 
                geometryModule:IMaterialGeometry, 
                materialModule:IMaterialPattern, 
                animationModule, 
                vertexAttribModule) targets StandardPipeline
			{
				public using vertexAttribModule;
				public using passParams;
				public using animationModule;
				public using TangentSpaceTransform;
				public using geometryModule;
				public using VertexTransform;
				public using materialModule;
				out @Fragment vec3 ocolor { if (opacity < 0.8f) discard; return vec3(1.0); }
			};
		)";
    protected:
        virtual void SetPipelineStates(FixedFunctionPipelineStates & states) override
        {
            states.DepthCompareFunc = CompareFunc::LessEqual;
            states.EnablePolygonOffset = false;
        }
    public:
        virtual const char * GetShaderSource() override
        {
            return shaderSrc;
        }
        virtual char * GetName() override
        {
            return "CustomDepthPass";
        }
        RenderTargetLayout * CreateRenderTargetLayout() override
        {
            return hwRenderer->CreateRenderTargetLayout(MakeArray(
                AttachmentLayout(TextureUsage::DepthAttachment, DepthBufferFormat)).GetArrayView());
        }
    };

    WorldRenderPass * CreateCustomDepthRenderPass()
    {
        return new CustomDepthRenderPass();
    }
}

