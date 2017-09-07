#ifndef GAME_ENGINE_WORLD_RENDER_PASS_H
#define GAME_ENGINE_WORLD_RENDER_PASS_H

#include "RenderPass.h"

namespace GameEngine
{
	class WorldRenderPass : public RenderPass
	{
	private:
		FixedFunctionPipelineStates fixedFunctionStates;
		SpireShader * shader = nullptr;

	protected:
		CoreLib::List<CoreLib::RefPtr<AsyncCommandBuffer>> commandBufferPool;
		int poolAllocPtr = 0;
		virtual const char * GetShaderSource() = 0;
		virtual RenderTargetLayout * CreateRenderTargetLayout() = 0;
		virtual void SetPipelineStates(FixedFunctionPipelineStates & state)
		{
			state.BlendMode = BlendMode::Replace;
			state.DepthCompareFunc = CompareFunc::Less;
		}
		virtual void Create() override;
	public:
		~WorldRenderPass();
		void ResetInstancePool()
		{
			poolAllocPtr = 0;
		}
		virtual void Bind();
		AsyncCommandBuffer * AllocCommandBuffer();
		CoreLib::RefPtr<WorldPassRenderTask> CreateInstance(RenderOutput * output, bool clearOutput);
	};
}

#endif
