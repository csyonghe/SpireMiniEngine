#include "PostRenderPass.h"
#include "Mesh.h"
#include "Engine.h"
#include "CoreLib/LibIO.h"
#include "OutlinePassParameters.h"
#include <assert.h>

using namespace CoreLib;
using namespace CoreLib::IO;

namespace GameEngine
{
    using namespace VectorMath;

    class OutlinePostRenderPass : public PostRenderPass
    {
    protected:
        RefPtr<RenderTarget> colorBuffer, depthBuffer, colorOutBuffer;
        RefPtr<Buffer> parameterBuffer;
        RefPtr<DescriptorSet> outlinePassDesc;
        bool isValid = true;
        const float outlineWidth = 5.0f;
        virtual void Resized() override
        {
            PostRenderPass::Resized();
            OutlinePassParameters defaultParams;
            defaultParams.pixelSize = Vec2::Create(1.0f / colorBuffer->Width, 1.0f / colorBuffer->Height);
            defaultParams.Width = outlineWidth;
            parameterBuffer->SetData(&defaultParams, sizeof(defaultParams));
        }
    public:
        virtual void Create(Renderer * renderer) override
        {
            PostRenderPass::Create(renderer);
            parameterBuffer = hwRenderer->CreateBuffer(BufferUsage::UniformBuffer, sizeof(OutlinePassParameters));
            // initialize parameter buffer with default params
            OutlinePassParameters defaultParams;
            defaultParams.pixelSize = Vec2::Create(1.0f / 1024.0f);
            defaultParams.Width = outlineWidth;
            parameterBuffer->SetData(&defaultParams, sizeof(defaultParams));
        }
        virtual void AcquireRenderTargets() override
        {
            colorBuffer = viewRes->LoadSharedRenderTarget(sources[0].Name, sources[0].Format);
            depthBuffer = viewRes->LoadSharedRenderTarget(sources[1].Name, sources[1].Format);
            colorOutBuffer = viewRes->LoadSharedRenderTarget(sources[2].Name, sources[2].Format);
        }
        virtual void SetupPipelineBindingLayout(PipelineBuilder * pipelineBuilder, List<AttachmentLayout> & renderTargets) override
        {
            renderTargets.Add(AttachmentLayout(TextureUsage::ColorAttachment, StorageFormat::RGBA_8));
            pipelineBuilder->SetDebugName("outline");

            outlinePassDesc = hwRenderer->CreateDescriptorSet(descLayouts[0].Ptr());
        }
        virtual void UpdateDescriptorSetBinding(SharedModuleInstances sharedModules, DescriptorSetBindings & binding) override
        {
            binding.Bind(0, outlinePassDesc.Ptr());
            binding.Bind(1, sharedModules.View->GetCurrentDescriptorSet());
        }
        virtual void UpdateRenderAttachments(RenderAttachments & attachments) override
        {
            if (!colorBuffer->Texture)
                return;

            outlinePassDesc->BeginUpdate();
            outlinePassDesc->Update(0, parameterBuffer.Ptr());
            outlinePassDesc->Update(1, colorBuffer->Texture.Ptr(), TextureAspect::Color);
            outlinePassDesc->Update(2, depthBuffer->Texture.Ptr(), TextureAspect::Depth);
            outlinePassDesc->Update(3, sharedRes->linearClampedSampler.Ptr());
            outlinePassDesc->EndUpdate();

            attachments.SetAttachment(0, colorOutBuffer->Texture.Ptr());
        }
        virtual String GetShaderFileName() override
        {
            return "Outline.shader";
        }
        virtual char * GetName() override
        {
            return "Outline";
        }
        virtual void SetParameters(void * data, int count) override
        {
            assert(count == sizeof(OutlinePassParameters));
            parameterBuffer->SetData(data, count);
        }
    public:
        OutlinePostRenderPass(ViewResource * viewRes)
            : PostRenderPass(viewRes)
        {}
    };

    PostRenderPass * CreateOutlinePostRenderPass(ViewResource * viewRes)
    {
        return new OutlinePostRenderPass(viewRes);
    }
}