#ifndef GAME_ENGINE_WORLD_RENDER_PASS_H
#define GAME_ENGINE_WORLD_RENDER_PASS_H

#include "RenderPass.h"

namespace GameEngine
{
	class WorldRenderPass : public RenderPass
	{
	protected:
		virtual void Create() override;
		virtual CoreLib::String GetEntryPointShader() = 0;
	public:
		virtual CoreLib::RefPtr<PipelineInstance> CreatePipelineStateObject(Material* material, Mesh* mesh, const DrawableSharedUniformBuffer & uniforms, DrawableType drawableType);
	};
}

#endif
