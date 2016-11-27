#include "AnimationSynthesizer.h"
#include <queue>

using namespace CoreLib;

namespace GameEngine
{
    using namespace VectorMath;

	int BinarySearchForKeyFrame(List<AnimationKeyFrame> & keyFrames, float time)
	{
		int begin = 0;
		int end = keyFrames.Count();
		while (begin < end)
		{
			int mid = (begin + end) >> 1;
			if (keyFrames[mid].Time > time)
				end = mid;
			else if (keyFrames[mid].Time == time)
				return mid;
			else
				begin = mid + 1;
		}
		if (begin >= keyFrames.Count())
			begin = keyFrames.Count() - 1;
		if (keyFrames[begin].Time > time)
			begin--;
		if (begin < 0)
			begin = 0;
		return begin;
	}

    void AnimationSynthesizer::toQuaternion(VectorMath::Quaternion & q, float x, float y, float z)
    {
        Matrix4 rotx, roty, rotz;
        Matrix4::RotationX(rotx, x);
        Matrix4::RotationY(roty, y);
        Matrix4::RotationZ(rotz, z);
        Matrix4::Multiply(rotz, rotz, roty);
        Matrix4::Multiply(rotx, rotx, rotz);
        
        q = Quaternion::FromMatrix(rotx.GetMatrix3());
        //q.y = 0.7f;
        //Matrix4 r = q.ToMatrix4();

        //Vec3 vx = Vec3::Create(1.f, 0.f, 0.f);
        //Vec3 vy = q.Transform(vx);
        //
        //Vec4 t = Vec4::Create(1.f, 0.f, 0.f, 0.f);
        //Vec4 vz = rotx.Transform(t);

        
        //float xt = 0, yt = 0, zt = 0;
        //toEulerianAngle(q, xt, yt, zt);
        //float a = yt;
    }

    void AnimationSynthesizer::toEulerianAngle(const VectorMath::Quaternion & q, float & x, float & y, float & z)
    {
        y = -atan2(2 * q.y * q.w - 2 * q.x * q.z, 1 - 2 * q.y * q.y - 2 * q.z * q.z);
        x = -atan2(2 * q.x * q.w - 2 * q.y * q.z, 1 - 2 * q.x * q.x - 2 * q.z * q.z);
        z = -asin(2 * q.x * q.y + 2 * q.z * q.w);

        //Matrix4 rotx, roty, rotz;
        //Matrix4::RotationX(rotx, x);
        //Matrix4::RotationY(roty, y);
        //Matrix4::RotationZ(rotz, z);
        //Matrix4::Multiply(rotz, rotz, roty);
        //Matrix4::Multiply(rotx, rotx, rotz);
        //auto q1 = Quaternion::FromMatrix(rotx.GetMatrix3());
    }

    void AnimationSynthesizer::changeYawAngle(Pose& p, float yaw)
    {
        float x = 0, y = 0, z = 0;
        toEulerianAngle(p.Transforms[0].Rotation, x, y, z);
        toQuaternion(p.Transforms[0].Rotation, x, yaw, z);
    }

	void SimpleAnimationSynthesizer::GetPose(Pose & p, float time)
	{
		p.Transforms.SetSize(skeleton->Bones.Count());
		float animTime = fmod(time * anim->Speed, anim->Duration);
        for (int i = 0; i < skeleton->Bones.Count(); i++)
            p.Transforms[i] = skeleton->Bones[i].BindPose;
		for (int i = 0; i < anim->Channels.Count(); i++)
		{
			if (anim->Channels[i].BoneId == -1)
				skeleton->BoneMapping.TryGetValue(anim->Channels[i].BoneName, anim->Channels[i].BoneId);
			if (anim->Channels[i].BoneId != -1)
			{
				int frame0 = BinarySearchForKeyFrame(anim->Channels[i].KeyFrames, animTime);
				int frame1 = frame0 + 1;
				float t = 0.0f;
				if (frame0 < anim->Channels[i].KeyFrames.Count() - 1)
				{
					t = (animTime - anim->Channels[i].KeyFrames[frame0].Time) / (anim->Channels[i].KeyFrames[frame1].Time - anim->Channels[i].KeyFrames[frame0].Time);
				}
				else
				{
					float b = (anim->Duration - anim->Channels[i].KeyFrames[frame0].Time);
					if (b <= 1e-4)
						t = 0.0f;
					else 
						t = (animTime - anim->Channels[i].KeyFrames[frame0].Time) / b;
					frame1 = 0;
				}
                auto & f0 = anim->Channels[i].KeyFrames[frame0];
				auto & f1 = anim->Channels[i].KeyFrames[frame1];
                
                p.Transforms[anim->Channels[i].BoneId] = BoneTransformation::Lerp(f0.Transform, f1.Transform, t); 
			}
		}
	}

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

    void MotionGraphAnimationSynthesizer::GetPose(Pose& p, float time)
    {
        time = time * motionGraph->Speed;
        if (nextState == nullptr)
        {
            int startId = random.Next(0, startCandidates.Count());
            lastState = &motionGraph->States[startId];
            lastStateTime = time;
            lastStateId = 0;
            lastStateRootTransform = lastState->Pose.Transforms[lastStateId];
            lastStateRootTransform.Translation = Vec3::Create(0.f);

            nextStateId = 1;
            nextState = &motionGraph->States[nextStateId];
            nextStateRootTransform = nextState->Pose.Transforms[0];
            nextStateRootTransform.Translation.x = lastState->Velocity.x + lastState->Pose.Transforms[0].Translation.x;
            nextStateRootTransform.Translation.z = lastState->Velocity.z + lastState->Pose.Transforms[0].Translation.z;

            float x = 0, y = 0, z = 0;
            yaw = 0.f;
            toEulerianAngle(lastState->Pose.Transforms[0].Rotation, x, y, z);
            toQuaternion(lastStateRootTransform.Rotation, x, yaw, z);
            toQuaternion(nextStateRootTransform.Rotation, x, yaw + nextState->YawAngularVelocity, z);
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
                VectorMath::Vec3 velocity = lastStateRootTransform.Rotation.ToMatrix3().Transform(lastState->Velocity);

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
                    nextState = &motionGraph->States[lastStateId + 1];
                    nextStateId = lastStateId + 1;
                    transitionGap++;
                }
                // if there is not other transitions to go, continue with its child
                else if (lastState->ChildrenIds.Count() == 1 &&
                    motionGraph->States[lastState->ChildrenIds.First()].Sequence == motionGraph->States[lastStateId].Sequence)
                {
                    nextStateId = lastState->ChildrenIds.First();
                    nextState = &motionGraph->States[nextStateId];
                    //transitionGap = 0;
                }
                // do transition
                else
                {
                    int randomNum = random.Next(0, lastState->ChildrenIds.Count());
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
                    printf("transfer %d->%d\n", lastState->Sequence, motionGraph->States[index].Sequence);
                    nextState = &motionGraph->States[index];
                    nextStateId = index;
                    transitionGap = 0;
                }

                nextStateRootTransform.Rotation = nextState->Pose.Transforms[0].Rotation;
                nextStateRootTransform.Scale = nextState->Pose.Transforms[0].Scale;

                float x = 0, y = 0, z = 0;
                yaw += nextState->YawAngularVelocity;
                toEulerianAngle(nextStateRootTransform.Rotation, x, y, z);
                toQuaternion(nextStateRootTransform.Rotation, x, yaw, z);

                nextStateRootTransform.Translation.x += velocity.x;
                nextStateRootTransform.Translation.z += velocity.z;
                nextStateRootTransform.Translation.y = nextState->Pose.Transforms[0].Translation.y;

                // velocity = Vec3::Create(0.f);
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
        //    if ((lastState->Contact == ContactLabel::LeftFootOnFloor || lastState->Contact == ContactLabel::BothFeetOnFloor) &&
        //        (nextState->Contact == ContactLabel::RightFootOnFloor || nextState->Contact == ContactLabel::InAir))
        //    {
        //        p.Transforms[7].Translation = lastState->Pose.Transforms[7].Translation;
        //        p.Transforms[8].Translation = lastState->Pose.Transforms[8].Translation;
        //    }
        //    else if ((lastState->Contact == ContactLabel::RightFootOnFloor || lastState->Contact == ContactLabel::BothFeetOnFloor) &&
        //        (nextState->Contact == ContactLabel::LeftFootOnFloor || nextState->Contact == ContactLabel::InAir))
        //    {
        //        p.Transforms[3].Translation = lastState->Pose.Transforms[3].Translation;
        //        p.Transforms[4].Translation = lastState->Pose.Transforms[4].Translation;
        //    }
    }

    template <class T, class S, class C>
    void clearpq(std::priority_queue<T, S, C>& q) {
        std::priority_queue<T, S, C> empty;
        std::swap(q, empty);
    }

    float EuclidDistance(Vec2 a, Vec2 b)
    {
        return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
    }

    float Norm(Vec3 a)
    {
        return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    }

    void SearchGraphAnimationSynthesizer::GetPose(Pose & p, float time)
    {
        time = time * motionGraph->Speed;
        if (lastStateId == -1)
        {
            lastStateId = 0;
            nextStateId = 1;
            lastStateTime = time;
        }
        else if((time - lastStateTime) > motionGraph->Duration)
        {
            lastStateId = nextStateId;
            nextStateId = lastStateId + 1;
            //nextStateId = static_cast<int>((time - lastStateTime) / motionGraph->Duration);
            lastStateTime = time;
        }
        
        if (lastStateId + 1 >= path.Count()) {
            lastStateId = 0;
            nextStateId = 1;
            lastStateTime = time;
        }
        else
        {
            BoneTransformation lastTransformation, nextTransformation;
            lastTransformation = path[lastStateId]->MGStatePtr->Pose.Transforms[0];
            lastTransformation.Translation.x = path[lastStateId]->Pos.x;
            lastTransformation.Translation.z = path[lastStateId]->Pos.y;
            float x = 0, y = 0, z = 0;
            toEulerianAngle(lastTransformation.Rotation, x, y, z);
            toQuaternion(lastTransformation.Rotation, x, path[lastStateId]->Yaw, z);

            nextTransformation = path[nextStateId]->MGStatePtr->Pose.Transforms[0];
            nextTransformation.Translation.x = path[nextStateId]->Pos.x;
            nextTransformation.Translation.z = path[nextStateId]->Pos.y;
            toEulerianAngle(nextTransformation.Rotation, x, y, z);
            toQuaternion(nextTransformation.Rotation, x, path[nextStateId]->Yaw, z);

            p.Transforms.SetSize(skeleton->Bones.Count());
            float animTime = fmod(time - lastStateTime, motionGraph->Duration);
            float t = animTime / motionGraph->Duration;
            p.Transforms[0] = BoneTransformation::Lerp(lastTransformation, nextTransformation, t);
            for (int i = 1; i < skeleton->Bones.Count(); i++)
            {
                p.Transforms[i] = BoneTransformation::Lerp(path[lastStateId]->MGStatePtr->Pose.Transforms[i],
                    path[nextStateId]->MGStatePtr->Pose.Transforms[i], t);
            }
        }
    }

    void SearchGraphAnimationSynthesizer::GetOptimalPath(int start, int end)
    {
        auto comparator = [](const RefPtr<SGState> & s0, const RefPtr<SGState> & s1)
        {
            return s0->Cost > s1->Cost;
        };
        std::priority_queue<RefPtr<SGState>, std::vector<RefPtr<SGState>>, decltype(comparator)> openList(comparator);
        List<RefPtr<SGState>> expandedList;
        
        RefPtr<SGState> startNode = new SGState();
        startNode->StateId = 0;
        startNode->MGStatePtr = &(motionGraph->States[0]);
        startNode->Yaw = 0.f;
        startNode->Parent = nullptr;
        startNode->TimePassed = 0.f;
        startNode->DistancePassed = 0.f;
        startNode->Cost = CalculateCost(*startNode, end);
        openList.push(startNode);

        while (true)
        {
            if (openList.size() == 0)
                break;
            
            RefPtr<SGState> expandedNode = openList.top();
            openList.pop();
            expandedList.Add(expandedNode);

            float distance = EuclidDistance(expandedNode->Pos, userPath.KeyPoints[end]);
            printf("distance: %f\n", distance);
            if (distance < goalThreshold)
                break;

            if (expandedNode->TransitionGap < minTransitionGap)
            {
                if (expandedNode->StateId != motionGraph->States.Count() - 1 &&
                    motionGraph->States[expandedNode->StateId + 1].Sequence == expandedNode->MGStatePtr->Sequence)
                {
                    RefPtr<SGState> node = new SGState();
                    node->StateId = expandedNode->StateId + 1;
                    node->MGStatePtr = &(motionGraph->States[node->StateId]);
                    node->Yaw = expandedNode->Yaw + expandedNode->MGStatePtr->YawAngularVelocity;

                    Matrix4 rotY;
                    Matrix4::RotationY(rotY, expandedNode->Yaw);
                    VectorMath::Vec3 velocity = rotY.TransformNormal(expandedNode->MGStatePtr->Velocity);
                    node->Pos.x = expandedNode->Pos.x + velocity.x;
                    node->Pos.y = expandedNode->Pos.y + velocity.z;
                    node->Parent = expandedNode.Ptr();
                    node->DistancePassed = expandedNode->DistancePassed + Norm(expandedNode->MGStatePtr->Velocity);
                    node->Cost = CalculateCost(*node, end);
                    node->TransitionGap = expandedNode->TransitionGap + 1;
                    openList.push(node);
                }
            }
            else
            {
                for (auto child : expandedNode->MGStatePtr->ChildrenIds)
                {
                    RefPtr<SGState> node = new SGState();

                    node->StateId = child;
                    node->MGStatePtr = &(motionGraph->States[child]);
                    node->Yaw = expandedNode->Yaw + expandedNode->MGStatePtr->YawAngularVelocity;

                    Matrix4 rotY;
                    Matrix4::RotationY(rotY, expandedNode->Yaw);
                    VectorMath::Vec3 velocity = rotY.TransformNormal(expandedNode->MGStatePtr->Velocity);
                    node->Pos.x = expandedNode->Pos.x + velocity.x;
                    node->Pos.y = expandedNode->Pos.y + velocity.z;
                    node->Parent = expandedNode.Ptr();
                    node->DistancePassed = expandedNode->DistancePassed + Norm(expandedNode->MGStatePtr->Velocity);
                    node->Cost = CalculateCost(*node, end);
                    if (node->StateId == expandedNode->StateId + 1 &&
                        node->MGStatePtr->Sequence == expandedNode->MGStatePtr->Sequence)
                    {
                        node->TransitionGap = expandedNode->TransitionGap + 1;
                    }
                    else
                    {
                        node->TransitionGap = 0;
                    }
                    openList.push(node);
                }
            }
        }

        // extract optimal list
        List<RefPtr<SGState>> optimalPath;
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

    float SearchGraphAnimationSynthesizer::CalculateCost(const SGState & state, int end)
    {
        //float g = state.TimePassed + state.DistancePassed;
        //Vec2 newPos = state.Pos;
        //newPos.x += state.MGStatePtr->Velocity.x;
        //newPos.y += state.MGStatePtr->Velocity.z;
        //float heuristic = EuclidDistance(newPos, userPath.KeyPoints[end]);

        //Vec2 newPos = state.Pos;
        //newPos.x += state.MGStatePtr->Velocity.x;
        //newPos.y += state.MGStatePtr->Velocity.z;
        //float v = state.DistancePassed / (state.TimePassed + 1);
        //float t = EuclidDistance(newPos, userPath.KeyPoints[end]) / (v+1);
        
        //printf("cost: %f + %f = %f\n", state.TimePassed, t, state.TimePassed + t);
        float g = state.DistancePassed;
        float h = EuclidDistance(state.Pos, userPath.KeyPoints[end]);
        return g + h;
    }
}