#ifndef GAME_ENGINE_LIGHT_PROBE_RENDERER_H
#define GAME_ENGINE_LIGHT_PROBE_RENDERER_H

#include "RenderContext.h"
#include "ViewResource.h"
#include "RenderProcedure.h"

namespace GameEngine
{
	class IRenderProcedure;
	class Renderer;

	class LightProbeRenderer : public CoreLib::RefObject
	{
	private:
		Renderer * renderer;
		RendererService * renderService;
		IRenderProcedure* renderProc;
		ViewResource * viewRes = nullptr;
		CoreLib::RefPtr<TextureCube> tempEnv;
		FrameRenderTask task;
	public:
		LightProbeRenderer(Renderer * renderer, RendererService * renderService, IRenderProcedure * pRenderProc, ViewResource * pViewRes);
		void RenderLightProbe(TextureCubeArray* dest, int id, Level * level, VectorMath::Vec3 position);
	};
}

#endif