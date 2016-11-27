#ifndef CATMULL_SPLINE_H
#define CATMULL_SPLINE_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Stream.h"

namespace GameEngine
{
    using namespace VectorMath;
    class CatmullSpline
    {
    public:
        List<Vec2> KeyPoints;
        List<Vec2> CurvePoints;

        Vec2 InterpolatePosition(const Vec2 & P0, const Vec2 & P1, const Vec2 & P2, const Vec2 & P3, float u);
        void SetKeyPoints(const List<Vec2> & points);
        void GetCurvePoints();
        void SaveToStream(CoreLib::IO::Stream * stream);
        void LoadFromStream(CoreLib::IO::Stream * stream);
        void SaveToFile(const CoreLib::String & filename);
        void LoadFromFile(const CoreLib::String & filename);
    };
}

#endif
