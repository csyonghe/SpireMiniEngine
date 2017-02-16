#ifndef GAME_ENGINE_RENDERER_SERIVCE_H
#define GAME_ENGINE_RENDERER_SERIVCE_H

#include "Drawable.h"

namespace GameEngine
{
	class RendererService : public CoreLib::Object
	{
	public:
		virtual CoreLib::RefPtr<Drawable> CreateStaticDrawable(Mesh * mesh, Material * material, bool cacheMesh = true) = 0;
		virtual CoreLib::RefPtr<Drawable> CreateSkeletalDrawable(Mesh * mesh, Skeleton * skeleton, Material * material, bool cacheMesh = true) = 0;
	};

}

#endif