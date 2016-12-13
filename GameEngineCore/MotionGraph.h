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
        int IdInsequence = 0;
        Vec3 Velocity = Vec3::Create(0.f);
        float YawAngularVelocity = 0.f;
        float LeftFootToRootDistance = 0.f;     // used to replant foot on the ground
        float RightFootToRootDistance = 0.f;
        ContactLabel Contact = ContactLabel::BothFeetOnFloor;
        EnumerableHashSet<int> ChildrenIds;

        static inline String GetContactName(ContactLabel l)
        {
            switch (l)
            {
            case ContactLabel::LeftFootOnFloor: return String("LeftFootOnFloor");
                break;
            case ContactLabel::RightFootOnFloor: return String("RightFootOnFloor");
                break;
            case ContactLabel::BothFeetOnFloor: return String("BothFeetOnFloor");
                break;
            case ContactLabel::InAir: return String("InAir");
                break;
            default:
                return "";
            }
        }
    };

    class SGState : public RefObject
    {
    public:
        MGState* MGStatePtr = nullptr;
        int StateId = 0;
        int TransitionGap = 0;
        Vec3 Pos = Vec3::Create(0.f);
        float Yaw = 0.f;
        float TimePassed = 0.f;
        float DistancePassed = 0.f;
    };

    class RSGStateKey
    {
    public:
        int mgStateId;
        float yaw;
        Vec3 pos;

        int GetHashCode()
        {
            int yawQ = (int)(yaw * 200.0f);
            int posX = (int)(pos.x * 10000.0f);
            int posZ = (int)(pos.z * 10000.0f);

            return (mgStateId << 18) ^ (yawQ ^ posX ^ posZ);
        }

        bool operator == (const RSGStateKey & other)
        {
            return mgStateId == other.mgStateId && (int)(yaw * 200.0f) == (int)(other.yaw * 200.0f) &&
                (int)(pos.x * 10000.0f) == (int)(other.pos.x*10000.0f) &&
                (int)(pos.z * 10000.0f) == (int)(other.pos.z*10000.0f);
        }
    };

    class RSGState : public RefObject
    {
    public:
        RSGState* Parent = nullptr;
        List<RefPtr<SGState>> SGStateList;
        float Cost = 0.f;
        float G = 0.f;
        bool isDeleted = false;
        bool operator<(const RSGState & b) const 
        {
            return Cost > b.Cost;
        }

        RSGStateKey GetKey()
        {
            auto state = SGStateList.Last();
            RSGStateKey key;
            key.pos = state->Pos;
            key.yaw = state->Yaw;
            key.mgStateId = state->StateId;
            return key;
        }
    };

    class IndexPair
    {
    public:
        int Id1 = 0, Id2 = 0;
        IndexPair(int id1, int id2) : Id1(id1), Id2(id2) {}
        IndexPair() = default;

        int GetHashCode()
        {
            return (Id1 << 16) ^ Id2;
        }
        bool operator == (const IndexPair & other)
        {
            return Id1 == other.Id1 && Id2 == other.Id2;
        }
    };

    struct StateTransitionInfo
    {
        List<int> ShortestPath;
        VectorMath::Vec3 DeltaPos = VectorMath::Vec3::Create(0.0f);
        float DeltaYaw = 0.f;
    };

    class MotionGraph
    {
    public:
        CoreLib::List<MGState> States;
        CoreLib::EnumerableDictionary<IndexPair, StateTransitionInfo> TransitionDictionary;
        float Speed;
        float Duration;

        void SaveToStream(CoreLib::IO::Stream * stream);
        void LoadFromStream(CoreLib::IO::Stream * stream);
        void SaveToFile(const CoreLib::String & filename);
        void LoadFromFile(const CoreLib::String & filename);
    };
}

#endif
