#include "MotionGraphAnimationSynthesizer.h"

namespace GameEngine
{
    void MotionGraphAnimationSynthesizer::GetStartCandidates()
    {
        int lastSeq = -1;
        for (int i = 0; i < motionGraph->States.Count(); i++)
        {
            if (motionGraph->States[i].Sequence != lastSeq)
            {
                startCandidates.Add(i);
                lastSeq = motionGraph->States[i].Sequence;
            }
        }
    }
    MotionGraphAnimationSynthesizer::MotionGraphAnimationSynthesizer(Skeleton * pSkeleton, MotionGraph * pMotionGraph)
        : skeleton(pSkeleton), motionGraph(pMotionGraph), random(3571)
    {
        GetStartCandidates();
    }
    void MotionGraphAnimationSynthesizer::GetPose(Pose & p, float time)
    {
        time = time * motionGraph->Speed;
        if (nextState == nullptr)
        {
            yaw = 0.f;

            int startId = random.Next(0, startCandidates.Count());
            lastState = &motionGraph->States[startId];
            lastStateTime = time;
            lastStateId = 0;
            lastStateRootTransform = lastState->Pose.Transforms[lastStateId];

            nextStateId = 1;
            nextState = &motionGraph->States[nextStateId];
            nextStateRootTransform = nextState->Pose.Transforms[0];
            nextStateRootTransform.Translation = lastState->Velocity + lastState->Pose.Transforms[0].Translation;
            nextStateRootTransform.Translation.y = nextState->Pose.Transforms[0].Translation.y;
            nextStateRootTransform.SetYawAngle(yaw + nextState->YawAngularVelocity);
            transitionGap++;
        }
        else
        {
            while (time - lastStateTime > motionGraph->Duration)
            {
                lastState = nextState;
                lastStateTime = lastStateTime + motionGraph->Duration;
                lastStateId = nextStateId;
                lastStateRootTransform = nextStateRootTransform;
                yaw += lastState->YawAngularVelocity;

                if (lastState->ChildrenIds.Count() == 0)    // dead end
                {
                    printf("Dead end still exists. \n");
                    nextState = nullptr;
                    GetPose(p, time);
                    return;
                }

                // if the transitionGap is smaller than threshold, 
                // and this frame is not the final one of a clip,
                // than do not transit, i.e. continue along this clip
                if (transitionGap < minTransitionGap &&
                    lastStateId != motionGraph->States.Count() - 1 &&
                    motionGraph->States[lastStateId + 1].Sequence == motionGraph->States[lastStateId].Sequence)
                {
                    
                    nextStateId = lastState->ChildrenIds.First();
                    nextState = &motionGraph->States[nextStateId];
                    transitionGap++;
                }
                // if there is not other transitions to go, continue with its child
                else if (lastState->ChildrenIds.Count() == 1)
                {
                    nextStateId = lastState->ChildrenIds.First();
                    nextState = &motionGraph->States[nextStateId];
                }
                // do transition
                else
                {
                    int randomNum = random.Next(1, lastState->ChildrenIds.Count());
                    int count = 0;
                    int index = 0;
                    for (auto child : lastState->ChildrenIds)
                    {
                        if (count++ == randomNum)
                        {
                            index = child;
                            break;
                        }
                    }
                    nextState = &motionGraph->States[index];
                    nextStateId = index;
                    Print("Transfer from (state %d, sequence %d) to (state %d, sequence %d)\n",
                        lastStateId, lastState->Sequence,
                        nextStateId, nextState->Sequence);

                    String msg = String("Contact from ") + MGState::GetContactName(lastState->Contact) +
                        String(" to ") + MGState::GetContactName(nextState->Contact) +
                        String("\n");
                    Print(msg.Buffer());
                    transitionGap = 0;
                }

                Quaternion rotation = motionGraph->States[nextStateId - 1].Pose.Transforms[0].Rotation;
                rotation.SetYawAngle(rotation, yaw);
                VectorMath::Vec3 velocity = rotation.ToMatrix3().Transform(nextState->Velocity);
                //Print("y velocity: %f\n", velocity.y);

                //Matrix4 roty;
                //Matrix4::RotationY(roty, yaw);
                //Vec3 velocity = roty.TransformNormal(nextState->Velocity);
                nextStateRootTransform.Rotation = nextState->Pose.Transforms[0].Rotation;
                nextStateRootTransform.Scale = nextState->Pose.Transforms[0].Scale;

                nextStateRootTransform.SetYawAngle(yaw + nextState->YawAngularVelocity);
                nextStateRootTransform.Translation += velocity;
                nextStateRootTransform.Translation.y = nextState->Pose.Transforms[0].Translation.y;

                //if (nextState->Contact == ContactLabel::LeftFootOnFloor)
                //    nextStateRootTransform.Translation.y = floorHeight + nextState->LeftFootToRootDistance;
                //else if (nextState->Contact == ContactLabel::RightFootOnFloor)
                //    nextStateRootTransform.Translation.y = floorHeight + nextState->RightFootToRootDistance;
                //else if (nextState->Contact == ContactLabel::BothFeetOnFloor)
                //    nextStateRootTransform.Translation.y = floorHeight + Math::Min(nextState->LeftFootToRootDistance, nextState->RightFootToRootDistance);
            }
        }

        p.Transforms.SetSize(skeleton->Bones.Count());
        float animTime = fmod(time - lastStateTime, motionGraph->Duration);
        float t = animTime / motionGraph->Duration;
        p.Transforms[0] = BoneTransformation::Lerp(lastStateRootTransform, nextStateRootTransform, t);
        for (int i = 1; i < skeleton->Bones.Count(); i++)
        {
            p.Transforms[i] = BoneTransformation::Lerp(lastState->Pose.Transforms[i], nextState->Pose.Transforms[i], t);
        }

        //// re-root feet
        //// case 1: right foot down/up, left foot does not move
        //if ((lastState->Contact == ContactLabel::LeftFootOnFloor && nextState->Contact == ContactLabel::BothFeetOnFloor) ||
        //    (lastState->Contact == ContactLabel::BothFeetOnFloor && nextState->Contact == ContactLabel::RightFootOnFloor))
        //{
        //    p.Transforms[3].Translation = lastState->Pose.Transforms[3].Translation;
        //    p.Transforms[4].Translation = lastState->Pose.Transforms[4].Translation;
        //}
        //// case 2: left foot down/up, right foot does not move
        //else if ((lastState->Contact == ContactLabel::RightFootOnFloor && nextState->Contact == ContactLabel::BothFeetOnFloor) ||
        //    (lastState->Contact == ContactLabel::BothFeetOnFloor && nextState->Contact == ContactLabel::LeftFootOnFloor))
        //{
        //    p.Transforms[7].Translation = lastState->Pose.Transforms[7].Translation;
        //    p.Transforms[8].Translation = lastState->Pose.Transforms[8].Translation;
        //}
    }
}