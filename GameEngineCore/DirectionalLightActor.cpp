#include "DirectionalLightActor.h"
#include "MeshBuilder.h"

namespace GameEngine
{
	Mesh DirectionalLightActor::CreateGizmoMesh()
	{
		MeshBuilder mb;
		mb.PushScale(VectorMath::Vec3::Create(2.0f));
		mb.PushRotation(VectorMath::Vec3::Create(0.0f, 0.0f, 1.0f), CoreLib::Math::Pi);
		mb.PushTranslation(VectorMath::Vec3::Create(0.0f, 20.0f, 0.0f));
		mb.AddCone(5.0f, 10.0f, 12);
		mb.PopTransform();
		mb.AddCylinder(2.0f, 20.0f, 12);
		return mb.ToMesh();
	}
}
