#ifndef GAME_ENGINE_WORLD_RENDER_PASS_H
#define GAME_ENGINE_WORLD_RENDER_PASS_H

#include "RenderPass.h"

namespace GameEngine
{
	class WorldRenderPass : public RenderPass
	{
	protected:
		CoreLib::RefPtr<ModuleInstance> moduleInstance;
		CoreLib::Array<CoreLib::RefPtr<CommandBuffer>, 32> commandBufferPool;
		int poolAllocPtr = 0;
		CommandBuffer * AllocCommandBuffer();
		virtual CoreLib::String GetEntryPointShader() = 0;
		virtual void SetPipelineStates(PipelineBuilder * /*pb*/) {}
	public:
		void ResetInstancePool()
		{
			poolAllocPtr = 0;
		}
		RenderPassInstance CreateInstance(RenderOutput * output);
		virtual CoreLib::RefPtr<PipelineClass> CreatePipelineStateObject(SceneResource * sceneRes, Material* material, Mesh* mesh, DrawableType drawableType);
	};
}

#endif
