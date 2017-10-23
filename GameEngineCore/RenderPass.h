#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "RenderContext.h"

namespace GameEngine
{
	class Renderer;
	class RenderPass : public CoreLib::Object
	{
	protected:
		RendererSharedResource * sharedRes = nullptr;
		HardwareRenderer * hwRenderer = nullptr;
		CoreLib::RefPtr<RenderTargetLayout> renderTargetLayout;
		virtual void Create(Renderer * renderer) = 0;
	public:
		void Init(Renderer * renderer);
		RenderTargetLayout * GetRenderTargetLayout()
		{
			return renderTargetLayout.Ptr();
		}
		virtual int GetShaderId() = 0;
		virtual char * GetName() = 0;
	};
}

#endif