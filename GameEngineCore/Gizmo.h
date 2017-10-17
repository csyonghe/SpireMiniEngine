#ifndef GAME_ENGINE_GIZMO_H
#define GAME_ENGINE_GIZMO_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"

namespace GameEngine
{
	class Mesh;
	class GetDrawablesParameter;

	class Gizmo : public CoreLib::RefObject
	{
	public:
		void SetMesh(const Mesh & mesh);
		void SetTransform(const VectorMath::Matrix4 & transform);
		void GetDrawables(const GetDrawablesParameter & params);
	};

}

#endif