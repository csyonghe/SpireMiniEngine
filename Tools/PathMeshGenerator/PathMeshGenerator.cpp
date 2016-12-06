#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Stream.h"
#include "CatmullSpline.h"
#include "GameEngineCore/Mesh.h"

namespace GameEngine
{
    namespace Tools
    {
        Mesh generatePathMesh(const CatmullSpline & catmullSpline, float width)
        {
            Mesh result;
            result.SetVertexFormat(MeshVertexFormat(0, 1, true, false));
            int numPoint = catmullSpline.KeyPoints.Count();
            result.AllocVertexBuffer(numPoint * 2);
            float halfW = width / 2.0f;
            for (int i = 0; i < numPoint; i++)
            {
                Vec3 tangent = Vec3::Create(0.f);
                if (i == 0)
                    tangent = catmullSpline.KeyPoints[i + 1].Pos - catmullSpline.KeyPoints[i].Pos;
                else if (i == numPoint - 1)
                    tangent = catmullSpline.KeyPoints[i].Pos - catmullSpline.KeyPoints[i - 1].Pos;
                else if(numPoint > 2)
                    tangent = (catmullSpline.KeyPoints[i + 1].Pos - catmullSpline.KeyPoints[i].Pos) * 0.5f;

                tangent = tangent.Normalize();

                Vec3 binorm = Vec3::Cross(tangent, Vec3::Create(0.0f, 1.0f, 0.0f)).Normalize();
                Vec3 v0 = catmullSpline.KeyPoints[i].Pos + binorm * halfW;
                Vec3 v1 = catmullSpline.KeyPoints[i].Pos - binorm * halfW;
                Vec2 uv0 = Vec2::Create(i / float(numPoint - 1), 0.0f);
                Vec2 uv1 = Vec2::Create(i / float(numPoint - 1), 1.0f);
                Quaternion tangentFrame = Quaternion::FromCoordinates(tangent, Vec3::Create(0.0f, 1.0f, 0.0f), binorm);
                result.SetVertexPosition(i * 2, v0);
                result.SetVertexUV(i * 2, 0, uv0);
                result.SetVertexTangentFrame(i * 2, tangentFrame);

                result.SetVertexPosition(i * 2 + 1, v1);
                result.SetVertexUV(i * 2 + 1, 0, uv1);
                result.SetVertexTangentFrame(i * 2 + 1, tangentFrame);
            }
            for (int i = 0; i < numPoint - 1; i++)
            {
                result.Indices.Add(i * 2);
                result.Indices.Add((i + 1) * 2);
                result.Indices.Add(i * 2 + 1);

                result.Indices.Add(i * 2 + 1);
                result.Indices.Add(i * 2 + 2);
                result.Indices.Add(i * 2 + 3);
            }
            return result;
        }
    }
}


using namespace GameEngine;

int wmain(int argc, wchar_t** argv)
{
    if (argc < 1)
    {
        printf("Usage: PathMeshGenerator [path] \n");
        return -1;
    }

    CatmullSpline path;
    List<Vec3> points;
    points.Add(Vec3::Create(0.0f));
    points.Add(Vec3::Create(50.0f, 0.0f, 0.0f));
    points.Add(Vec3::Create(100.0f, 0.0f, 0.0f));
    points.Add(Vec3::Create(100.0f, 0.0f, -50.0f));
    points.Add(Vec3::Create(100.0f, 0.0f, -100.0f));
    path.SetKeyPoints(points);

    //String inputFileName = String::FromWString(argv[1]);
    //String outputFileName = CoreLib::IO::Path::ReplaceExt(inputFileName, "mesh");

    //Mesh mesh = Tools::generatePathMesh(path, 5.0f);
    //mesh.SaveToFile(outputFileName);

    String inputFileName = String::FromWString(argv[1]);
    String outputFileName = CoreLib::IO::Path::ReplaceExt(inputFileName, "path");
    path.SaveToFile(outputFileName);

    return 0;
}