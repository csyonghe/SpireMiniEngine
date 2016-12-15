#ifndef GAME_ENGINE_WORLD_RENDER_PASS_H
#define GAME_ENGINE_WORLD_RENDER_PASS_H

#include "RenderPass.h"

namespace GameEngine
{
	class WorldRenderPass : public RenderPass
	{
	protected:
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
		RenderPassInstance CreateInstance(RenderOutput * output, void * viewUniformData, int viewUniformSize);
		virtual CoreLib::RefPtr<PipelineInstance> CreatePipelineStateObject(Material* material, Mesh* mesh, const DrawableSharedUniformBuffer & uniforms, DrawableType drawableType);
	};
}

#endif
