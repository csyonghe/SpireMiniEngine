#ifndef GAME_ENGINE_LIGHT_PROBE_RENDERER_H
#define GAME_ENGINE_LIGHT_PROBE_RENDERER_H

#include "RenderContext.h"
#include "ViewResource.h"

namespace GameEngine
{
	class LightProbeRenderer : public CoreLib::RefObject
	{
	private:
		Renderer * renderer;
		RendererService * renderService;
		IRenderProcedure* renderProc;
		ViewResource * viewRes = nullptr;
	public:
		LightProbeRenderer(Renderer * renderer, RendererService * renderService, IRenderProcedure * pRenderProc, ViewResource * pViewRes);
		CoreLib::RefPtr<TextureCube> RenderLightProbe(Level * level, VectorMath::Vec3 position);
	};
}

#endif