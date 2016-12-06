#include "CatmullSpline.h"

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Tokenizer.h"

namespace GameEngine
{
    using namespace CoreLib::IO;

    Vec3 CatmullSpline::GetInterpolatedPosition(float t)
    {
        // TODO: use straight lines currently
        if (KeyPoints.Count() < 2)
            printf("Error: no enough key points");
        t = Math::Clamp(t, 0.f, 1.f);
         
        int index = 0;
        for (index = 0; index < KeyPoints.Count()-1; index++)
        {
            if (KeyPoints[index + 1].T > t)
                break;
        }
        float alpha = (t - KeyPoints[index].T) / (KeyPoints[index+1].T - KeyPoints[index].T);
        Vec3 result = KeyPoints[index].Pos * (1 - alpha) + KeyPoints[index + 1].Pos * alpha;
        return result;
    }

    void CatmullSpline::SetKeyPoints(const List<Vec3>& points)
    {
        if (points.Count() < 2)
            printf("Error: no enough key points");

        float distance = 0.f;
        for (int i = 0; i < points.Count(); i++)
        {
            KeyPoint p;
            p.Pos = points[i];
            if (i > 0)
            {
                distance += (points[i] - points[i - 1]).Length();
            }
            KeyPoints.Add(_Move(p));
        }

        KeyPoints[0].T = 0.f;
        for (int i = 1; i < points.Count(); i++)
        {
            KeyPoints[i].T = KeyPoints[i - 1].T + (points[i] - points[i - 1]).Length() / distance;
        }
    }

    void CatmullSpline::SaveToStream(CoreLib::IO::Stream * stream)
    {
        StreamWriter writer(stream);
        for (auto & point : KeyPoints)
        {
            writer << point.Pos.x << "," << point.Pos.y << "," << point.Pos.z << ",\n";
        }
        writer.ReleaseStream();
    }

    void CatmullSpline::LoadFromStream(CoreLib::IO::Stream * stream)
    {
        StreamReader reader(stream);

        CoreLib::Text::TokenReader parser(reader.ReadToEnd());
        List<Vec3> points;
        while (!parser.IsEnd())
        {
            Vec3 pos;
            pos.x = parser.ReadFloat();
            if (parser.LookAhead(",")) parser.Read(",");
            pos.y = parser.ReadFloat();
            if (parser.LookAhead(",")) parser.Read(",");
            pos.z = parser.ReadFloat();
            if (parser.LookAhead(",")) parser.Read(",");
            points.Add(pos);
        }
        SetKeyPoints(points);
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