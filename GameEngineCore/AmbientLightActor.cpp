#include "AmbientLightActor.h"
#include "MeshBuilder.h"

namespace GameEngine
{
    Mesh AmbientLightActor::CreateGizmoMesh()
    {
        MeshBuilder mb;
        mb.AddCylinder(20.0f, 40.0f, 20);
        return mb.ToMesh();
    }
}
