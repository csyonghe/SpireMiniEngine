#ifndef PATH_MESH_GENERATOR_H
#define PATH_MESH_GENERATOR_H

#include "CoreLib/Basic.h"
#include "CatmullSpline.h"

namespace GameEngine
{
    namespace Tools
    {
        void generatePathMesh(const CatmullSpline & catmullSpline);
    }
}

#endif