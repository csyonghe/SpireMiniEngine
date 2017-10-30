#include "AnimationControllerActor.h"
#include "SimpleAnimationControllerActor.h"
#include "Level.h"

namespace GameEngine
{
    void SimpleAnimationControllerActor::UpdateStates()
    {
        if (this->simpleAnimation && this->skeleton)
            simpleSynthesizer = new SimpleAnimationSynthesizer(skeleton, simpleAnimation);
        Tick();
    }
    void SimpleAnimationControllerActor::AnimationFileName_Changing(CoreLib::String & newFileName)
    {
        simpleAnimation = level->LoadSkeletalAnimation(newFileName);
        if (!simpleAnimation)
            newFileName = "";
        UpdateStates();
    }

    void SimpleAnimationControllerActor::SkeletonFileName_Changing(CoreLib::String & newFileName)
    {
        skeleton = level->LoadSkeleton(newFileName);
        if (!skeleton)
            newFileName = "";
        UpdateStates();
    }
    void SimpleAnimationControllerActor::EvalAnimation(float time)
    {
        if (simpleSynthesizer)
        {
            Pose pose;
            simpleSynthesizer->GetPose(pose, time);
            for (int i = 0; i < TargetActors->Count(); i++)
                if (auto target = GetTargetActor(i))
                    target->SetPose(pose);
        }
    }
    void SimpleAnimationControllerActor::OnLoad()
    {
        AnimationControllerActor::OnLoad();
        if (AnimationFile.GetValue().Length())
            simpleAnimation = level->LoadSkeletalAnimation(*AnimationFile);
        if (SkeletonFile.GetValue().Length())
            skeleton = level->LoadSkeleton(*SkeletonFile);
        UpdateStates();
        AnimationFile.OnChanging.Bind(this, &SimpleAnimationControllerActor::AnimationFileName_Changing);
        SkeletonFile.OnChanging.Bind(this, &SimpleAnimationControllerActor::SkeletonFileName_Changing);
    }
}
