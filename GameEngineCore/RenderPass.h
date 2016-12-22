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
		HardwareRenderer * hwRenderer = nullptr;
		CoreLib::RefPtr<RenderTargetLayout> renderTargetLayout;
		virtual void Create() = 0;
	public:
		void Init(RendererSharedResource * pSharedRes);
		RenderTargetLayout * GetRenderTargetLayout()
		{
			return renderTargetLayout.Ptr();
		}
		void SetId(int id)
		{
			renderPassId = id;
		}
		
		virtual char * GetName() = 0;
	};
}

#endif