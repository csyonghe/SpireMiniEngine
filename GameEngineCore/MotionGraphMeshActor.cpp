#include "MotionGraphMeshActor.h"
#include "Engine.h"
#include "MeshBuilder.h"
#include "AnimationSynthesizer.h"
#include <ppl.h>

namespace GameEngine
{
    class SearchGraphAnimationSynthesizer : public AnimationSynthesizer
    {
    private:
        Skeleton * skeleton = nullptr;
        MotionGraph * motionGraph = nullptr;
        CatmullSpline & userPath;
        List<RefPtr<RSGState>> path;
        List<int> parentCount;
        BoneTransformation lastTransformation;
        BoneTransformation nextTransformation;
        int   lastStateId = -1;     // id in path
        int   nextStateId = 0;
        int   lastNodeId = 0;       // id in RSGState->SGStateList
        int   nextNodeId = 0;

        int   currentSegmentId = 0;
        int   minTransitionGap = 10000;
        int   counter = 0;
        float lastStateTime = 0.f;
        float goalThreshold = 30.0f;
        float maxDeltaPos = 80.0f;
       
    private:
        void getMgStateParentCount()
        {
            int numStates = motionGraph->States.Count();

            for (int i = 0; i < numStates; i++)
                parentCount.Add(0);

            for (int i = 0; i < numStates; i++)
            {
                for (auto child : motionGraph->States[i].ChildrenIds)
                {
                    parentCount[child] += 1;
                }
            }
        }
        bool hasAchievedGoal(Vec3 pos, Vec3 goal)
        {
            auto d = (goal - pos);
            d.y = 0.0f;
            float distance = d.Length();
            if (distance < goalThreshold)
                return true;

            return false;
        }
        bool hasNoResult(Vec3 pos, Vec3 goal, float threshold)
        {
            float distance = (goal - pos).Length();
#ifndef DEBUG
            //printf("Distance: %f\n", distance);
#endif
            if (distance > threshold)
                return true;

            return false;
        }
        bool hasExpanded(const List<RefPtr<RSGState>> & closeList, const SGState* statePtr)
        {
            for (auto p : closeList)
            {
                auto sgPtr = p->SGStateList.Last();
                if (sgPtr->StateId == statePtr->StateId &&
                    sgPtr->Yaw == statePtr->Yaw &&
                    sgPtr->Pos == statePtr->Pos)
                    return true;
            }
            return false;
        }
        bool isTransitionState(int i)
        {
            if (parentCount.Count() == 0)
                getMgStateParentCount();

            if (motionGraph->States[i].ChildrenIds.Count() > 1 ||
                parentCount[i] > 1)
                return true;
            else
                return false;
        }

    public:
        SearchGraphAnimationSynthesizer(Skeleton * pSkeleton, MotionGraph * pMotionGraph, CatmullSpline & path)
            : skeleton(pSkeleton), motionGraph(pMotionGraph), userPath(path)
        {
            printf("start calculating optimal path.\n");
            clock_t start = clock();
            for (int i = 0; i < userPath.KeyPoints.Count(); i++)
            //for(int i = 0; i < 2; i++)
            {
                currentSegmentId = i;
                GetOptimalPathWithPrecomputeAndParallel(currentSegmentId, currentSegmentId + 1);
                printf("----------------------------------------------\n");
                printf("GetOptimalPath from %d to %d\n", currentSegmentId, currentSegmentId + 1);
                printf("----------------------------------------------\n");
            }
            
            //GetOptimalPath(currentSegmentId, currentSegmentId + 1);
            clock_t end = clock();
            double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
            printf("%lf seconds.\n", elapsed_secs);
        }
        virtual void GetPose(Pose & p, float time) override
        {
            time = time * motionGraph->Speed;
            if (path.Count() == 0)
            {
                p = motionGraph->States[0].Pose;
                return;
            }

            if (lastStateId == -1 || (time - lastStateTime) > motionGraph->Duration)
            {
                lastStateId = nextStateId;
                lastNodeId = nextNodeId;
                if (lastNodeId + 1 < path[lastStateId]->SGStateList.Count())
                {
                    nextNodeId = lastNodeId + 1;
                }
                else
                {
                    nextStateId = lastStateId + 1;
                    nextNodeId = 0;
                }
                lastStateTime = time;

                if (nextStateId >= path.Count()) {
                    lastStateId = 0;
                    lastNodeId = 0;
                    if (lastNodeId + 1 < path[0]->SGStateList.Count())
                    {
                        nextStateId = 0;
                        nextNodeId = 1;
                    }
                    else
                    {
                        nextStateId = 1;
                        nextNodeId = 0;
                    }
                }
                else
                {
                    auto lastSG = path[lastStateId]->SGStateList[lastNodeId];
                    lastTransformation = lastSG->MGStatePtr->Pose.Transforms[0];
                    lastTransformation.Translation.x = lastSG->Pos.x;
                    lastTransformation.Translation.z = lastSG->Pos.z;
                    lastTransformation.SetYawAngle(lastSG->Yaw);

                    auto nextSG = path[nextStateId]->SGStateList[nextNodeId];
                    nextTransformation = nextSG->MGStatePtr->Pose.Transforms[0];
                    nextTransformation.Translation.x = nextSG->Pos.x;
                    nextTransformation.Translation.z = nextSG->Pos.z;
                    nextTransformation.SetYawAngle(nextSG->Yaw);

                    if (nextSG->StateId != lastSG->MGStatePtr->ChildrenIds.First())
                        Print("Transfer from (state %d, sequence %d) to (state %d, sequence %d)\n",
                            lastSG->StateId, lastSG->MGStatePtr->Sequence,
                            nextSG->StateId, nextSG->MGStatePtr->Sequence);
                }
            }

            p.Transforms.SetSize(skeleton->Bones.Count());
            float animTime = fmod(time - lastStateTime, motionGraph->Duration);
            float t = animTime / motionGraph->Duration;
            p.Transforms[0] = BoneTransformation::Lerp(lastTransformation, nextTransformation, t);
            for (int i = 1; i < skeleton->Bones.Count(); i++)
            {
                auto lastMG = path[lastStateId]->SGStateList[lastNodeId]->MGStatePtr;
                auto nextMG = path[nextStateId]->SGStateList[nextNodeId]->MGStatePtr;
                p.Transforms[i] = BoneTransformation::Lerp(lastMG->Pose.Transforms[i], nextMG->Pose.Transforms[i], t);
            }
        }
        void CompactStates(RSGState* rsgState, const Vec3 & goalPos, float overThreshold)
        {
            while (true)
            {
                auto lastSG = rsgState->SGStateList.Last();

                RefPtr<SGState> state = new SGState();
                state->StateId = lastSG->MGStatePtr->ChildrenIds.First();
                state->MGStatePtr = &(motionGraph->States[state->StateId]);

                //Quaternion rotation = lastState->MGStatePtr->Pose.Transforms[0].Rotation;
                //Quaternion::SetYawAngle(rotation, lastState->Yaw);
                //Vec3 velocity = rotation.Transform(state->MGStatePtr->Velocity);
                Matrix4 roty;
                Matrix4::RotationY(roty, lastSG->Yaw);
                Vec3 velocity = roty.TransformNormal(state->MGStatePtr->Velocity);

                state->Pos = lastSG->Pos + velocity;

                // hack
                state->Pos.y = state->MGStatePtr->Pose.Transforms[0].Translation.y;

                state->Yaw = lastSG->Yaw + state->MGStatePtr->YawAngularVelocity;
                state->TimePassed = lastSG->TimePassed + 1;
                state->DistancePassed = lastSG->DistancePassed + velocity.Length();
                state->TransitionGap = 0;

                rsgState->SGStateList.Add(state);
                if (isTransitionState(state->StateId) ||
                    rsgState->SGStateList.Count() > minTransitionGap ||
                    hasAchievedGoal(state->Pos, goalPos) ||
                    hasNoResult(state->Pos, goalPos, overThreshold))
                    break;
            }
        }
        void GetOptimalPathWithPrecompute(int start, int end)
        {
            auto comparator = [](const RefPtr<RSGState> & s0, const RefPtr<RSGState> & s1)
            {
                return s0->Cost > s1->Cost;
            };
            std::priority_queue<RefPtr<RSGState>, std::vector<RefPtr<RSGState>>, decltype(comparator)> openList(comparator);
            List<RefPtr<RSGState>> expandedList;

            RefPtr<RSGState> startNode = new RSGState();
            Vec3 goalPos = userPath.KeyPoints[end].Pos;
            float overThreshold = 2.0f * (userPath.KeyPoints[end].Pos - userPath.KeyPoints[start].Pos).Length();
            if (path.Count() == 0)
            {
                Vec3 tangent = userPath.KeyPoints[start + 1].Pos - userPath.KeyPoints[start].Pos;
                tangent = tangent.Normalize();
                float theta = acosf(tangent.x);
                if (tangent.z > 0)
                    theta = -theta;

                RefPtr<SGState> state = new SGState();
                state->Pos = userPath.KeyPoints[start].Pos;
                state->StateId = 0;
                state->MGStatePtr = &(motionGraph->States[0]);
                state->Yaw = theta;
                state->TimePassed = 0.f;
                state->DistancePassed = 0.f;
                state->TransitionGap = 0;
                startNode->SGStateList.Add(state);
            }
            else
            {
                auto lastEnd = path[path.Count() - 1]->SGStateList.Last();

                RefPtr<SGState> state = new SGState();
                state->Pos = lastEnd->Pos;
                state->StateId = lastEnd->StateId;
                state->MGStatePtr = lastEnd->MGStatePtr;
                state->Yaw = lastEnd->Yaw;
                state->TimePassed = 0.f;
                state->DistancePassed = 0.f;
                state->TransitionGap = 0;
                startNode->SGStateList.Add(state);
            }
            openList.push(startNode);

            CompactStates(startNode.Ptr(), goalPos, overThreshold);
            startNode->Cost = CalculateCost(*startNode.Ptr(), end);

            while (true)
            {
                if (openList.size() == 0)
                    break;

                RefPtr<RSGState> expandedRSG = openList.top();
                openList.pop();

                //printf("sequence %d, id %d\n", expandedRSG->SGStateList.Last()->MGStatePtr->Sequence, expandedRSG->SGStateList.Last()->MGStatePtr->IdInsequence);
                //printf("distance: %f\n", (expandedRSG->SGStateList.Last()->Pos - goalPos).Length());
                //printf("yaw: %f\n\n", expandedRSG->SGStateList.Last()->Yaw);

                auto expandedSG = expandedRSG->SGStateList.Last();
                if (hasAchievedGoal(expandedSG->Pos, goalPos) ||
                    hasNoResult(expandedSG->Pos, goalPos, overThreshold))
                    break;
                
                int i = expandedSG->StateId;

                for (int j = 0; j < motionGraph->States.Count(); j++)
                {
                    if (! isTransitionState(j))
                        continue;

                    StateTransitionInfo info;
                    if (motionGraph->TransitionDictionary.TryGetValue(IndexPair(i, j), info))
                    {
                        if (info.DeltaPos.Length() > maxDeltaPos)
                            continue;

                        RefPtr<RSGState> rsgState = new RSGState();
                        if (info.ShortestPath.Count() < 2)
                            printf("Error: info.ShortestPath.Count() < 2");
                        for (int k = 1; k < info.ShortestPath.Count(); k++)
                        {
                            RefPtr<SGState> sgState = new SGState();
                            int index = info.ShortestPath[k];
                            auto state = motionGraph->States[index];
                            SGState* lastSGState = nullptr;
                            if (rsgState->SGStateList.Count() == 0)
                                lastSGState = expandedSG.Ptr();
                            else
                                lastSGState = rsgState->SGStateList.Last().Ptr();

                            float yaw = lastSGState->Yaw;
                            Quaternion rotation = motionGraph->States[index - 1].Pose.Transforms[0].Rotation;
                            Quaternion::SetYawAngle(rotation, yaw);
                            Vec3 velocity = rotation.Transform(state.Velocity);
                            sgState->Pos = lastSGState->Pos + velocity;
                            sgState->Yaw = yaw + state.YawAngularVelocity;
                            sgState->StateId = index;
                            sgState->TimePassed = lastSGState->TimePassed + 1;
                            sgState->DistancePassed = lastSGState->DistancePassed + velocity.Length();
                            sgState->MGStatePtr = &motionGraph->States[index];
                            sgState->TransitionGap = 0;
                            rsgState->SGStateList.Add(sgState);
                            if (hasAchievedGoal(sgState->Pos, goalPos) ||
                                hasNoResult(sgState->Pos, goalPos, overThreshold))
                                break;
                        }
                        rsgState->Parent = expandedRSG.Ptr();
                        rsgState->Cost = CalculateCost(*rsgState.Ptr(), end);
                        //rsgState->Cost = (rsgState->SGStateList.Last()->Pos - goalPos).Length();
                        if (hasExpanded(expandedList, rsgState->SGStateList.Last().Ptr()))
                            continue;
                        openList.push(rsgState);
                        expandedList.Add(rsgState);
                    }
                }
            }

            // extract optimal Path
            List<RefPtr<RSGState>> optimalPath;
            if (expandedList.Count() == 0)
            {
                printf("No expand list");
                return;
            }

            auto p = expandedList.Last();
            while (p)
            {
                optimalPath.Add(p);
                p = p->Parent;
            }

            for (int i = optimalPath.Count() - 1; i >= 0; i--)
            {
                path.Add(optimalPath[i]);
            }
        }
        void GetOptimalPath(int start, int end)
        {
            auto comparator = [](const RefPtr<RSGState> & s0, const RefPtr<RSGState> & s1)
            {
                return s0->Cost > s1->Cost;
            };
            std::priority_queue<RefPtr<RSGState>, std::vector<RefPtr<RSGState>>, decltype(comparator)> openList(comparator);
            List<RefPtr<RSGState>> expandedList;

            float overThreshold = 2.0f * (userPath.KeyPoints[end].Pos - userPath.KeyPoints[start].Pos).Length();
            RefPtr<RSGState> startNode = new RSGState();
            if (path.Count() == 0)
            {
                Vec3 tangent = userPath.KeyPoints[start + 1].Pos - userPath.KeyPoints[start].Pos;
                tangent = tangent.Normalize();
                float theta = acosf(tangent.x);
                if (tangent.z > 0)
                    theta = -theta;

                RefPtr<SGState> state = new SGState();
                state->Pos = userPath.KeyPoints[start].Pos;
                state->StateId = 0;
                state->MGStatePtr = &(motionGraph->States[0]);
                //state->Yaw = theta;
                state->Yaw = 0.f;
                state->TimePassed = 0.f;
                state->DistancePassed = 0.f;
                state->TransitionGap = 0;
                startNode->SGStateList.Add(state);
            }
            else
            {
                auto lastEnd = path[path.Count() - 1]->SGStateList.Last();

                RefPtr<SGState> state = new SGState();
                state->Pos = lastEnd->Pos;
                state->StateId = lastEnd->StateId;
                state->MGStatePtr = lastEnd->MGStatePtr;
                state->Yaw = lastEnd->Yaw;
                state->TimePassed = 0.f;
                state->DistancePassed = 0.f;
                state->TransitionGap = 0;
                startNode->SGStateList.Add(state);
            }
            CompactStates(startNode.Ptr(), userPath.KeyPoints[end].Pos, overThreshold);
            startNode->Parent = nullptr;
            startNode->Cost = CalculateCost(*startNode, end);
            openList.push(startNode);
#ifdef DEBUG
            printf("Open List add: -----------------------------\n");
            printf("Cost: %f ", startNode->Cost);
            for (int i = 0; i < startNode->SGStateList.Count(); i++)
            {
                printf("%d, ", startNode->SGStateList[i]->StateId);
            }
            printf("\n");
#endif

            while (true)
            {
                if (openList.size() == 0)
                    break;

                RefPtr<RSGState> expandedRSG = openList.top();
                openList.pop();
                expandedList.Add(expandedRSG);

                if (hasAchievedGoal(expandedRSG->SGStateList.Last()->Pos, userPath.KeyPoints[end].Pos))
                    break;

                float distance = (expandedRSG->SGStateList.Last()->Pos - userPath.KeyPoints[end].Pos).Length();
                printf("Distance: %f\n", distance);

                auto expandedSG = expandedRSG->SGStateList.Last();

                printf("(Sequence %d, Id %d) , Pos (%f, %f)\n", 
                    expandedSG->MGStatePtr->Sequence, expandedSG->MGStatePtr->IdInsequence,
                    expandedSG->Pos.x, expandedSG->Pos.z);

                auto childrenList = expandedSG->MGStatePtr->ChildrenIds;
                for (auto child : childrenList)
                {
                    RefPtr<RSGState> rsgNode = new RSGState();
                    rsgNode->Parent = expandedRSG.Ptr();

                    RefPtr<SGState> node = new SGState();
                    node->StateId = child;
                    node->MGStatePtr = &(motionGraph->States[child]);
                    node->Yaw = expandedSG->Yaw + node->MGStatePtr->YawAngularVelocity;

                    //Quaternion rotation = expandedSG->MGStatePtr->Pose.Transforms[0].Rotation;
                    //Quaternion::SetYawAngle(rotation, expandedSG->Yaw);
                    //Vec3 velocity = rotation.Transform(node->MGStatePtr->Velocity);
                    Matrix4 roty;
                    Matrix4::RotationY(roty, expandedSG->Yaw);
                    Vec3 velocity = roty.TransformNormal(node->MGStatePtr->Velocity);

                    node->Pos = expandedSG->Pos + velocity;

                    // hack
                    node->Pos.y = node->MGStatePtr->Pose.Transforms[0].Translation.y;

                    node->DistancePassed = expandedSG->DistancePassed + velocity.Length();
                    node->TimePassed = expandedSG->TimePassed + 1;
                    node->TransitionGap = 0;

                    rsgNode->SGStateList.Add(node);
                    CompactStates(rsgNode.Ptr(), userPath.KeyPoints[end].Pos, overThreshold);
                    if (hasExpanded(expandedList, rsgNode->SGStateList.Last().Ptr()))
                        continue;

                    rsgNode->Cost = CalculateCost(*rsgNode, end);
                    openList.push(rsgNode);

#ifdef DEBUG
                    printf("Cost: %f ", rsgNode->Cost);
                    for (int i = 0; i < rsgNode->SGStateList.Count(); i++)
                    {
                        printf("%d, ", rsgNode->SGStateList[i]->StateId);
                    }
                    printf("\n");
#endif

                }
            }

            // extract optimal list
            List<RefPtr<RSGState>> optimalPath;
            if (expandedList.Count() == 0)
            {
                printf("No expand list");
                return;
            }

            auto p = expandedList.Last();
            while (p)
            {
                optimalPath.Add(p);
                p = p->Parent;
            }

            for (int i = optimalPath.Count() - 1; i >= 0; i--)
            {
                path.Add(optimalPath[i]);
            }

#ifdef DEBUG
            // print optimal path
            printf("Optimal Path: -----------------------------\n");
            for (int i = optimalPath.Count() - 1; i >= 0; i--)
            {
                for (int j = 0; j < optimalPath[i]->SGStateList.Count(); j++)
                {
                    printf("%d, ", optimalPath[i]->SGStateList[j]->StateId);
                }
                printf("\n");
            }

            // print expanded list
            printf("Expanded List: (stateId, cost) -----------------------------\n");
            for (int i = 0; i < expandedList.Count(); i++)
            {
                printf("Cost: %f ", expandedList[i]->Cost);
                for (int j = 0; j < expandedList[i]->SGStateList.Count(); j++)
                {
                    printf("%d, ", expandedList[i]->SGStateList[j]->StateId);
                }
                printf("\n");
            }
#endif
        }
        void GetOptimalPathWithPrecomputeAndParallel(int start, int end)
        {
            auto comparator = [](const RefPtr<RSGState> & s0, const RefPtr<RSGState> & s1)
            {
                return s0->Cost > s1->Cost;
            };
            std::priority_queue<RefPtr<RSGState>, std::vector<RefPtr<RSGState>>, decltype(comparator)> openList(comparator);
            
            EnumerableDictionary<RSGStateKey, RefPtr<RSGState>> closeDictionary;
            EnumerableDictionary<RSGStateKey, RefPtr<RSGState>> openDictionary;

            RefPtr<RSGState> startNode = new RSGState();
            Vec3 goalPos = userPath.KeyPoints[end].Pos;
            float overThreshold = 2.0f * (userPath.KeyPoints[end].Pos - userPath.KeyPoints[start].Pos).Length();
            if (path.Count() == 0)
            {
                Vec3 tangent = userPath.KeyPoints[start + 1].Pos - userPath.KeyPoints[start].Pos;
                tangent = tangent.Normalize();
                float theta = acosf(tangent.x);
                if (tangent.z > 0)
                    theta = -theta;

                RefPtr<SGState> state = new SGState();
                state->Pos = userPath.KeyPoints[start].Pos;
                state->StateId = 0;
                state->MGStatePtr = &(motionGraph->States[0]);
                state->Yaw = theta;
                state->TimePassed = 0.f;
                state->DistancePassed = 0.f;
                state->TransitionGap = 0;
                startNode->SGStateList.Add(state);
            }
            else
            {
                auto lastEnd = path[path.Count() - 1]->SGStateList.Last();

                RefPtr<SGState> state = new SGState();
                state->Pos = lastEnd->Pos;
                state->StateId = lastEnd->StateId;
                state->MGStatePtr = lastEnd->MGStatePtr;
                state->Yaw = lastEnd->Yaw;
                state->TimePassed = 0.f;
                state->DistancePassed = 0.f;
                state->TransitionGap = 0;
                startNode->SGStateList.Add(state);
            }
            openList.push(startNode);
            openDictionary[startNode->GetKey()] = startNode;

            CompactStates(startNode.Ptr(), goalPos, overThreshold);
            CalculateFGCost(startNode.Ptr(), end);

            while (true)
            {
                if (openList.size() == 0)
                    break;

                RefPtr<RSGState> expandedRSG = openList.top();
                openList.pop();
                openDictionary.Remove(expandedRSG->GetKey());
                if (expandedRSG->isDeleted)
                    continue;

                closeDictionary[expandedRSG->GetKey()] = expandedRSG;

                auto expandedSG = expandedRSG->SGStateList.Last();
                if (hasAchievedGoal(expandedSG->Pos, goalPos) ||
                    hasNoResult(expandedSG->Pos, goalPos, overThreshold))
                    break;

                int i = expandedSG->StateId;

                for (int j = 0; j < motionGraph->States.Count(); j++)
                {
                    if (!isTransitionState(j))
                        continue;

                    StateTransitionInfo info;
                    if (motionGraph->TransitionDictionary.TryGetValue(IndexPair(i, j), info))
                    {
                        if (info.DeltaPos.Length() > maxDeltaPos)
                            continue;

                        RefPtr<RSGState> rsgState = new RSGState();
                        if (info.ShortestPath.Count() < 2)
                            printf("Error: info.ShortestPath.Count() < 2");
                        for (int k = 1; k < info.ShortestPath.Count(); k++)
                        {
                            RefPtr<SGState> sgState = new SGState();
                            int index = info.ShortestPath[k];
                            auto state = motionGraph->States[index];
                            SGState* lastSGState = nullptr;
                            if (rsgState->SGStateList.Count() == 0)
                                lastSGState = expandedSG.Ptr();
                            else
                                lastSGState = rsgState->SGStateList.Last().Ptr();

                            float yaw = lastSGState->Yaw;
                            Quaternion rotation = motionGraph->States[index - 1].Pose.Transforms[0].Rotation;
                            Quaternion::SetYawAngle(rotation, yaw);
                            Vec3 velocity = rotation.Transform(state.Velocity);
                            sgState->Pos = lastSGState->Pos + velocity;
                            sgState->Yaw = yaw + state.YawAngularVelocity;
                            sgState->StateId = index;
                            sgState->TimePassed = lastSGState->TimePassed + 1;
                            sgState->DistancePassed = lastSGState->DistancePassed + velocity.Length();
                            sgState->MGStatePtr = &motionGraph->States[index];
                            sgState->TransitionGap = 0;
                            rsgState->SGStateList.Add(sgState);
                            if (hasAchievedGoal(sgState->Pos, goalPos) ||
                                hasNoResult(sgState->Pos, goalPos, overThreshold))
                                break;
                        }
                        rsgState->Parent = expandedRSG.Ptr();
                        CalculateFGCost(rsgState.Ptr(), end);
                        if (closeDictionary.ContainsKey(rsgState->GetKey()))
                            continue;

                        RefPtr<RSGState> existingState;
                        if (openDictionary.TryGetValue(rsgState->GetKey(), existingState))
                        {
                            if (rsgState->G < existingState->G)
                            {
                                existingState->isDeleted = true;
                            }
                        }
                        
                        openList.push(rsgState);
                        openDictionary[rsgState->GetKey()] = rsgState;
                    }
                }
            }

            // extract optimal Path
            List<RefPtr<RSGState>> optimalPath;

            auto p = closeDictionary.Last().Value;
            while (p)
            {
                optimalPath.Add(p);
                p = p->Parent;
            }

            for (int i = optimalPath.Count() - 1; i >= 0; i--)
            {
                path.Add(optimalPath[i]);
            }
        }
        void CalculateFGCost(RSGState* statePtr, int end)
        {
            auto sgState = statePtr->SGStateList.Last();

            float walk_speed = 70.0f;
            float turn_speed = Math::Pi * 0.5f / 1.5f;
            Vec3 tangent = userPath.KeyPoints[end].Pos - sgState->Pos;
            tangent.y = 0.0f;
            tangent = tangent.Normalize();
            float theta = acosf(tangent.x);
            if (tangent.z > 0)
                theta = -theta;

            float g = sgState->DistancePassed / walk_speed;
            Vec3 dp = (userPath.KeyPoints[end].Pos - sgState->Pos);
            dp.y = 0.0f;
            float h = (dp.Length() / walk_speed + abs(theta - sgState->Yaw) / turn_speed) * 120.0f;
            statePtr->G = g;
            statePtr->Cost = g + h;
        }
        float CalculateCost(const RSGState & state, int end)
        {
            auto sgState = state.SGStateList.Last();

            //float g = sgState->DistancePassed;
            //float h = (userPath.KeyPoints[end].Pos - sgState->Pos).Length();
            float walk_speed = 70.0f;
            float turn_speed = Math::Pi * 0.5f / 1.5f;
            //float turn_speed = Math::Pi;
            Vec3 tangent = userPath.KeyPoints[end].Pos - sgState->Pos;
            tangent.y = 0.0f;
            tangent = tangent.Normalize();
            float theta = acosf(tangent.x);
            if (tangent.z > 0)
                theta = -theta;

            float g = sgState->DistancePassed / walk_speed;
            Vec3 dp = (userPath.KeyPoints[end].Pos - sgState->Pos);
            dp.y = 0.0f;
            float h = (dp.Length() / walk_speed + abs(theta - sgState->Yaw) / turn_speed) * 120.0f;
            return g + h;
        }
    };

    bool MotionGraphMeshActor::ParseField(Level * level, CoreLib::Text::TokenReader & parser, bool &isInvalid)
    {
        if (Actor::ParseField(level, parser, isInvalid))
            return true;
        if (parser.LookAhead("mesh"))
        {
            parser.ReadToken();
            MeshName = parser.ReadStringLiteral();
            Mesh = level->LoadMesh(MeshName);
            if (!Mesh)
                isInvalid = false;
            return true;
        }
        if (parser.LookAhead("material"))
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
                if (!MaterialInstance)
                    isInvalid = true;
            }
            return true;
        }
        if (parser.LookAhead("Skeleton"))
        {
            parser.ReadToken();
            SkeletonName = parser.ReadStringLiteral();
            Skeleton = level->LoadSkeleton(SkeletonName);
            if (!Skeleton)
                isInvalid = true;
            return true;
        }
        if (parser.LookAhead("MotionGraph"))
        {
            parser.ReadToken();
            MotionGraphName = parser.ReadStringLiteral();
            MotionGraph = level->LoadMotionGraph(MotionGraphName);
            if (!MotionGraph)
                isInvalid = true;
            return true;
        }
        if (parser.LookAhead("Path"))
        {
            parser.ReadToken();
            if (parser.NextToken().Type == CoreLib::Text::TokenType::StringLiterial)
            {
                String fileName = parser.ReadStringLiteral();
                auto fullFileName = Engine::Instance()->FindFile(fileName, ResourceType::Mesh);
                try
                {
                    if (fullFileName.Length())
                    {
                        path = new CatmullSpline();
                        path->LoadFromFile(fullFileName);
                    }
                }
                catch (Exception)
                {
                    isInvalid = true;
                }
            }
            else if (parser.LookAhead("{"))
            {
                parser.Read("{");
                path = new CatmullSpline();
                List<Vec3> points;
                while (!parser.IsEnd() && !parser.LookAhead("}"))
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
                parser.Read("}");
                path->SetKeyPoints(points);
            }
            else
                return false;
            return true;
        }
        return false;
    }

    void MotionGraphMeshActor::Tick()
    {
        auto time = Engine::Instance()->GetTime();
        //static float time = 0.0f;
        //time += 0.001f;
        if (Animation)
            Animation->GetPose(nextPose, time);
    }

    void MotionGraphMeshActor::GetDrawables(const GetDrawablesParameter & params)
    {
        if (!drawable)
            drawable = params.rendererService->CreateSkeletalDrawable(Mesh, Skeleton, MaterialInstance);
        if (!pathIndicatorDrawable && path)
        {
            diskIndicatorMaterial = *Engine::Instance()->GetLevel()->LoadMaterial("LineIndicator.material");
            //diskIndicatorMaterial.SetVariable("solidColor", DynamicVariable(Vec3::Create(0.7f, 0.7f, 0.0f)));
            MeshBuilder mb;
            mb.AddDisk(path->KeyPoints.Last().Pos + Vec3::Create(0.0f, 1.0f, 0.0f), Vec3::Create(0.f, 1.f, 0.f), 27.0f, 24.0f, 32);
            for (int i = 0; i < path->KeyPoints.Count() - 1; i++)
            {
                mb.AddLine(path->KeyPoints[i].Pos + Vec3::Create(0.0f, 1.0f, 0.0f),
                    path->KeyPoints[i + 1].Pos + Vec3::Create(0.0f, 1.0f, 0.0f), 
                    Vec3::Create(0.0f, 1.0f, 0.0f), 3.0f);
            }
            
            auto mesh = mb.ToMesh();
            pathIndicatorDrawable = params.rendererService->CreateStaticDrawable(&mesh, &diskIndicatorMaterial);
        }

        drawable->UpdateTransformUniform(localTransform, nextPose);
        Matrix4 scaleMatrix;
        Matrix4::Scale(scaleMatrix, 2.0f, 2.0f, 2.0f);
        pathIndicatorDrawable->UpdateTransformUniform(scaleMatrix);
        params.sink->AddDrawable(drawable.Ptr());
        if (pathIndicatorDrawable)
            params.sink->AddDrawable(pathIndicatorDrawable.Ptr());
    }

    void MotionGraphMeshActor::OnLoad()
    {
        if (this->MotionGraph && path)
        {
            Animation = new SearchGraphAnimationSynthesizer(Skeleton, this->MotionGraph, *path);
            //Animation = new MotionGraphAnimationSynthesizer(Skeleton, this->MotionGraph);
        }
        Tick();
    }
}