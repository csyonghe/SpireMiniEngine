#ifndef MOTION_GRAPH_ANIMATION_SYNTHESIZER_H
#define MOTION_GRAPH_ANIMATION_SYNTHESIZER_H

#include "AnimationSynthesizer.h"
#include "Engine.h"

namespace GameEngine
{
    class MotionGraphAnimationSynthesizer : public AnimationSynthesizer
    {
    private:
        Skeleton * skeleton = nullptr;
        MotionGraph * motionGraph = nullptr;
        MGState* lastState = nullptr;
        MGState* nextState = nullptr;
        BoneTransformation lastStateRootTransform, nextStateRootTransform;
        int transitionGap = 0;
        int minTransitionGap = 60;
        int lastStateId = 0;
        int nextStateId = 0;
        float lastStateTime = 0.f;
        float yaw = 0.f;
        float floorHeight = 0.f;
        CoreLib::Random random;
        CoreLib::List<int> startCandidates;

        void GetStartCandidates();

    public:
        MotionGraphAnimationSynthesizer(Skeleton * pSkeleton, MotionGraph * pMotionGraph);
        virtual void GetPose(Pose & p, float time) override;
    };
}

#endif
