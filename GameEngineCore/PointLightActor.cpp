#include "PointLightActor.h"
#include "MeshBuilder.h"

namespace GameEngine
{
	Mesh PointLightActor::CreateGizmoMesh()
	{
		MeshBuilder mb;
		mb.AddCone(10.0f, 40.0f, 15);
		return mb.ToMesh();
	}
}
