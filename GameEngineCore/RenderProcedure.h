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
	private:
		int frameId = 0;
		CoreLib::List<CoreLib::RefPtr<RenderTask>> subTasks[3];
	public:
		SharedModuleInstances sharedModuleInstances;
		void Clear()
		{
			subTasks[frameId].Clear();
		}
		void NewFrame() 
		{
			frameId++;
			frameId %= 3;
			subTasks[frameId].Clear();
		}
		void AddTask(const CoreLib::RefPtr<RenderTask>& task)
		{
			subTasks[frameId].Add(task);
		}
		CoreLib::List<CoreLib::RefPtr<RenderTask>> & GetTasks()
		{
			return subTasks[frameId];
		}
	};

	class IRenderProcedure : public CoreLib::RefObject
	{
	public:
		virtual void Init(Renderer * renderer, ViewResource * pViewRes) = 0;
		virtual void UpdateSharedResourceBinding() = 0;
		virtual void Run(FrameRenderTask & task, const RenderProcedureParameters & params) = 0;
		virtual RenderTarget* GetOutput() = 0;
	};

	IRenderProcedure * CreateStandardRenderProcedure(bool toneMapping, bool useEnvMap);
}

#endif