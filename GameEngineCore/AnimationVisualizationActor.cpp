#include "AnimationVisualizationActor.h"
#include "Engine.h"

namespace GameEngine
{
    bool AnimationVisualizationActor::ParseField(CoreLib::String fieldName, CoreLib::Text::TokenReader & parser)
    {
        if (Actor::ParseField(fieldName, parser))
            return true;
        if (fieldName == "mesh")
        {
            MeshName = parser.ReadStringLiteral();
            Mesh = level->LoadMesh(MeshName);
            return true;
        }
        if (fieldName == "material")
        {
            if (parser.NextToken(1).Content == "{")
            {
                MaterialInstance = level->CreateNewMaterial();
                MaterialInstance->Parse(parser);
            }
            else
            {
                parser.ReadToken();
                auto materialName = parser.ReadStringLiteral();
                MaterialInstance = level->LoadMaterial(materialName);
            }
            return true;
        }
        if (fieldName == "Skeleton")
        {
            parser.ReadToken();
            SkeletonName = parser.ReadStringLiteral();
            Skeleton = level->LoadSkeleton(SkeletonName);
            return true;
        }
        return false;
    }

    void AnimationVisualizationActor::Tick()
    {
        if (Graph)
            nextPose = Graph->States[frameId].Pose;
    }

    void AnimationVisualizationActor::GetDrawables(const GetDrawablesParameter & params)
    {
        if (!drawable)
            drawable = params.rendererService->CreateSkeletalDrawable(Mesh, 0, Skeleton, MaterialInstance);
        drawable->UpdateTransformUniform(*LocalTransform, nextPose);
        params.sink->AddDrawable(drawable.Ptr());
    }

    void AnimationVisualizationActor::OnLoad()
    {
        Tick();
    }
}
