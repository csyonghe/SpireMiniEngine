#include "CatmullSpline.h"

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"

namespace GameEngine
{
    using namespace CoreLib::IO;

    Vec2 CatmullSpline::InterpolatePosition(const Vec2 & P0, const Vec2 & P1, const Vec2 & P2, const Vec2 & P3, float u)
    {
        return Vec2::Create(0.f);
    }

    void CatmullSpline::SetKeyPoints(const List<Vec2>& points)
    {
        KeyPoints = points;
    }

    void CatmullSpline::GetCurvePoints()
    {
        return;
    }

    void CatmullSpline::SaveToStream(CoreLib::IO::Stream * stream)
    {
        BinaryWriter writer(stream);

        writer.Write(KeyPoints.Count());
        writer.Write(CurvePoints.Count());
        for (auto & point : KeyPoints)
        {
            writer.Write(point);
        }
        for (auto & point : CurvePoints)
        {
            writer.Write(point);
        }
        writer.ReleaseStream();
    }

    void CatmullSpline::LoadFromStream(CoreLib::IO::Stream * stream)
    {
        BinaryReader reader(stream);

        int numKeyPoints = 0;
        int numCurvePoints = 0;
        reader.Read(numKeyPoints);
        reader.Read(numCurvePoints);
        KeyPoints.SetSize(numKeyPoints);
        CurvePoints.SetSize(numCurvePoints);

        for (int i = 0; i < numKeyPoints; i++)
        {
            reader.Read(KeyPoints[i]);
        }
        for (int i = 0; i < numCurvePoints; i++)
        {
            reader.Read(CurvePoints[i]);
        }
        reader.ReleaseStream();
    }

    void CatmullSpline::SaveToFile(const CoreLib::String & filename)
    {
        RefPtr<FileStream> stream = new FileStream(filename, FileMode::Create);
        SaveToStream(stream.Ptr());
        stream->Close();
    }

    void CatmullSpline::LoadFromFile(const CoreLib::String & filename)
    {
        RefPtr<FileStream> stream = new FileStream(filename, FileMode::Open);
        LoadFromStream(stream.Ptr());
        stream->Close();
    }
}