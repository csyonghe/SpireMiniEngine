#ifndef GAME_ENGINE_FRUSTUM_CULLING_H
#define GAME_ENGINE_FRUSTUM_CULLING_H

#include "CoreLib/Graphics/ViewFrustum.h"
#include "CoreLib/Graphics/BBox.h"

namespace GameEngine
{
	struct CullFrustum
	{
	private:
		void FromVerts(CoreLib::ArrayView<VectorMath::Vec3> points);
	public:
		VectorMath::Vec4 Planes[6];
		bool IsBoxInFrustum(CoreLib::Graphics::BBox box);

		CullFrustum(CoreLib::Graphics::ViewFrustum f);
		CullFrustum(CoreLib::Graphics::Matrix4 invViewProj);

		CullFrustum() = default;
	};
}
#endif
