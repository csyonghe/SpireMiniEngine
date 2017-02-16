#include "Drawable.h"
#include "RenderContext.h"

namespace GameEngine
{
	Drawable::Drawable(SceneResource * sceneRes)
	{
		scene = sceneRes;
		Bounds.Min = VectorMath::Vec3::Create(-1e9f);
		Bounds.Max = VectorMath::Vec3::Create(1e9f);
		pipelineCache.SetSize(pipelineCache.GetCapacity());
		for (auto & p : pipelineCache)
			p = nullptr;
		transformModule = new ModuleInstance();
	}
	Drawable::~Drawable()
	{
		delete transformModule;
	}
}