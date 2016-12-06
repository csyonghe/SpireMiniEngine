#ifndef MOTION_GRAPH_ACTOR_H
#define MOTION_GRAPH_ACTOR_H

#include "Actor.h"
#include "MotionGraph.h"
#include "MotionGraphAnimationSynthesizer.h"


namespace GameEngine
{
    class MotionGraphMeshActor : public Actor
    {
    private:
        Pose nextPose;
        RefPtr<Drawable> drawable;
        RefPtr<Drawable> pathIndicatorDrawable;
        RefPtr<CatmullSpline> path;
        Material diskIndicatorMaterial;
        float startTime = 0.0f;
        

    protected:
        virtual bool ParseField(Level * level, CoreLib::Text::TokenReader & parser, bool &isInvalid) override;
    public:
        CoreLib::RefPtr<AnimationSynthesizer> Animation;
        Mesh * Mesh = nullptr;
        Skeleton * Skeleton = nullptr;
        MotionGraph * MotionGraph = nullptr;

        CoreLib::String MeshName, SkeletonName, MotionGraphName;
        Material * MaterialInstance = nullptr;
        virtual void Tick() override;
        Pose & GetCurrentPose()
        {
            return nextPose;
        }
        virtual void GetDrawables(RendererService * renderService) override;
        virtual EngineActorType GetEngineType() override
        {
            return EngineActorType::Drawable;
        }
        virtual CoreLib::String GetTypeName() override
        {
            return "MotionGraphMesh";
        }
        virtual void OnLoad() override;
    };


}

#endif