#ifndef GAME_ENGINE_STANDARD_VIEW_UNIFORMS_H
#define GAME_ENGINE_STANDARD_VIEW_UNIFORMS_H

#include "CoreLib/VectorMath.h"

namespace GameEngine
{
	class StandardViewUniforms
	{
	public:
		VectorMath::Matrix4 ViewTransform, ViewProjectionTransform, InvViewTransform, InvViewProjTransform;
		VectorMath::Vec3 CameraPos;
		float Time;
	};
}

#endif