#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "Common.h"
#include "RenderContext.h"

namespace GameEngine
{
	class RenderPass : public CoreLib::Object
	{
	protected:
		RendererSharedResource * sharedRes = nullptr;
		SceneResource * sceneRes = nullptr;
		HardwareRenderer * hwRenderer = nullptr;
		CoreLib::RefPtr<FrameBuffer> frameBuffer;
		CoreLib::RefPtr<RenderTargetLayout> renderTargetLayout;
		CoreLib::RefPtr<CommandBuffer> clearCommandBuffer;

		virtual void Create();
		virtual void AcquireRenderTargets() = 0;
	public:
		void Init(RendererSharedResource * pSharedRes, SceneResource * pSceneRes);
		virtual void UpdateFrameBuffer() = 0;
		FrameBuffer * GetFrameBuffer()
		{
			return frameBuffer.Ptr();
		}
		RenderTargetLayout * GetRenderTargetLayout()
		{
			return renderTargetLayout.Ptr();
		}
		CommandBuffer * GetClearCommandBuffer()
		{
			return clearCommandBuffer.Ptr();
		}
		virtual char * GetName() = 0;
	};
}

#endif