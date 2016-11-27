#ifndef MOTION_GRAPH_H
#define MOTION_GRAPH_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Stream.h"
#include "Skeleton.h"

namespace GameEngine
{
    using namespace VectorMath;

    enum class ContactLabel
    {
        LeftFootOnFloor,
        RightFootOnFloor,
        BothFeetOnFloor,
        InAir
    };

    class MGState
    {
    public:
        Pose Pose;
        int Sequence = 0;
        Vec3 Velocity = Vec3::Create(0.f);
        float YawAngularVelocity = 0.f;
        ContactLabel Contact = ContactLabel::BothFeetOnFloor;
        EnumerableHashSet<int> ChildrenIds;
    };

    class SGState : public RefObject
    {
    public:
        SGState* Parent = nullptr;
        MGState* MGStatePtr = nullptr;
        int StateId = 0;
        int TransitionGap = 0;
        Vec2 Pos = Vec2::Create(0.f);  
        float Cost = 0.f;
        float Yaw = 0.f;
        float TimePassed = 0.f;
        float DistancePassed = 0.f;

        bool operator<(const SGState & b) const {
            return Cost > b.Cost;
        }
    };

    class MotionGraph
    {
    public:
        CoreLib::List<MGState> States;
        float Speed;
        float Duration;

        void SaveToStream(CoreLib::IO::Stream * stream);
        void LoadFromStream(CoreLib::IO::Stream * stream);
        void SaveToFile(const CoreLib::String & filename);
        void LoadFromFile(const CoreLib::String & filename);
    };
}

#endif
