#ifndef GAME_ENGINE_ANIMATION_CONTROLLER_ACTOR
#define GAME_ENGINE_ANIMATION_CONTROLLER_ACTOR

#include "SkeletalMeshActor.h"
#include "GizmoActor.h"

namespace GameEngine
{
    class AnimationControllerActor : public GizmoActor
    {
    private:
        void TimeChanged();
        void IsPlayingChanged();
    protected:
        void UpdateStates();
        SkeletalMeshActor * GetTargetActor(int id);
        virtual void EvalAnimation(float time) = 0;
    public:
        PROPERTY_DEF(float, Time, 0.0f);
        PROPERTY_DEF(bool, IsPlaying, true);
        PROPERTY_ATTRIB(CoreLib::List<CoreLib::String>, TargetActors, "ActorList(SkeletalMesh)");
        virtual EngineActorType GetEngineType()
        {
            return EngineActorType::Util;
        }
        virtual void OnLoad();
        virtual void Tick();
    };
}

#endif