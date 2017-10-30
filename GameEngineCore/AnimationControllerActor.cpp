#include "AnimationControllerActor.h"
#include "Engine.h"
#include "MeshBuilder.h"

namespace GameEngine
{
    void AnimationControllerActor::TimeChanged()
    {
        EvalAnimation(Time.GetValue());
    }
    void AnimationControllerActor::IsPlayingChanged()
    {
    }
    SkeletalMeshActor * AnimationControllerActor::GetTargetActor(int id)
    {
        if (id < TargetActors->Count())
        {
            auto rs = level->FindActor((*TargetActors)[id]);
            return dynamic_cast<SkeletalMeshActor*>(rs);
        }
        else
        {
            return nullptr;
        }
    }
    void AnimationControllerActor::OnLoad()
    {
        MeshBuilder mb;
        mb.AddPyramid(40.0f, 40.0f, 40.0f);
        SetGizmoCount(1);
        SetGizmoMesh(0, mb.ToMesh(), GizmoStyle::Editor);
        GizmoActor::OnLoad();
        IsPlaying.OnChanged.Bind(this, &AnimationControllerActor::IsPlayingChanged);
        Time.OnChanged.Bind(this, &AnimationControllerActor::TimeChanged);
    }
    void AnimationControllerActor::Tick()
    {
        Actor::Tick();
        if (IsPlaying.GetValue())
        {
            Time = Time + Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
        }
    }
}
