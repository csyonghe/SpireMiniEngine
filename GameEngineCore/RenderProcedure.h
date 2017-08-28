#ifndef GAME_ENGINE_RENDER_PROCEDURE_H
#define GAME_ENGINE_RENDER_PROCEDURE_H

#include "RenderContext.h"

namespace GameEngine
{
	class RendererService;

	struct RenderProcedureParameters
	{
		Renderer * renderer;
		RenderStat * renderStats;
		View view;
		Level * level;
		RendererService * rendererService;
	};

	struct FrameRenderTask
	{
		CoreLib::List<CoreLib::RefPtr<RenderTask>> subTasks;
		SharedModuleInstances sharedModuleInstances;
		void Clear();
	};

	class IRenderProcedure : public CoreLib::RefObject
	{
	public:
		virtual void Init(Renderer * renderer, ViewResource * pViewRes) = 0;
		virtual void UpdateSharedResourceBinding() = 0;
		virtual void Run(FrameRenderTask & task, const RenderProcedureParameters & params) = 0;
		virtual RenderTarget* GetOutput() = 0;
	};

	IRenderProcedure * CreateStandardRenderProcedure(bool toneMapping);
}

#endif