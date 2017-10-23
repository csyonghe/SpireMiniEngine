#ifndef CATMULL_SPLINE_H
#define CATMULL_SPLINE_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Stream.h"

namespace GameEngine
{
    using namespace VectorMath;

    class KeyPoint
    {
    public:
        Vec3 Pos;
        float T;
    };

    class CatmullSpline
    {
    public:
        List<KeyPoint> KeyPoints;
        Vec3 GetInterpolatedPosition(float t);
        void SetKeyPoints(const List<Vec3> & points);

        void SaveToStream(CoreLib::IO::Stream * stream);
        void LoadFromStream(CoreLib::IO::Stream * stream);
        void SaveToFile(const CoreLib::String & filename);
        void LoadFromFile(const CoreLib::String & filename);
    };
}

#endif
