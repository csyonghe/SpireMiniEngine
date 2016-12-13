#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "Common.h"
#include "RenderContext.h"

namespace GameEngine
{
	class RenderPass : public CoreLib::Object
	{
	protected:
		int renderPassId = -1;
		RendererSharedResource * sharedRes = nullptr;
		SceneResource * sceneRes = nullptr;
		HardwareRenderer * hwRenderer = nullptr;
		CoreLib::RefPtr<RenderTargetLayout> renderTargetLayout;
		CoreLib::Array<CoreLib::RefPtr<CommandBuffer>, 32> commandBufferPool;
		int poolAllocPtr = 0;
		CommandBuffer * AllocCommandBuffer();
		virtual void Create() = 0;
	public:
		void Init(RendererSharedResource * pSharedRes, SceneResource * pSceneRes);
		RenderTargetLayout * GetRenderTargetLayout()
		{
			return renderTargetLayout.Ptr();
		}
		void SetId(int id)
		{
			renderPassId = id;
		}
		void ResetInstancePool()
		{
			poolAllocPtr = 0;
		}
		RenderPassInstance CreateInstance(RenderOutput * output, void * viewUniformData, int viewUniformSize);
		virtual char * GetName() = 0;
	};
}

#endif