#include "AnimationSynthesizer.h"

using namespace CoreLib;

namespace GameEngine
{
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
                
                p.Transforms[anim->Channels[i].BoneId] = BoneTransformation::Lerp(f0.Transform, f1.Transform, t); //BoneTransformation(); //
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
            //int startId = random.Next(0, startCandidates.Count());
            lastState = &motionGraph->States[startCandidates[startId]];
            lastStateTime = time;
            int childNum = lastState->ChildrenIds.Count();
            int child = random.Next(0, childNum);
            nextState = &motionGraph->States[lastState->ChildrenIds[child]];
            startId = (startId+1) % startCandidates.Count();
        }
        else 
        {
            while (time - lastStateTime > motionGraph->Duration)
            {
                preventJumpCounter++;
                rootLocation += lastState->Velocities[0];
                lastState = nextState;
                lastStateTime = lastStateTime + motionGraph->Duration;
                int childNum = lastState->ChildrenIds.Count();
                if (childNum == 0)
                {
                    nextState = nullptr;
                    rootLocation.SetZero();
                    GetPose(p, time);
                    return;
                }
                int child = random.Next(0, childNum);
                //if (child > 0)
                //{
                //    if (preventJumpCounter < 10)
                //        child = 0;
                //    else
                //        preventJumpCounter = 0;
                //}
                if (child > 1)
                {
                    printf("transfer %d->%d\n", lastState->Sequence, motionGraph->States[lastState->ChildrenIds[child]].Sequence);
                }
                nextState = &motionGraph->States[lastState->ChildrenIds[child]];
                nextState->Pose.Transforms[0].Translation = lastState->Velocities[0] + lastState->Pose.Transforms[0].Translation;
            }
        }

        p.Transforms.SetSize(skeleton->Bones.Count());
        float animTime = fmod(time-lastStateTime, motionGraph->Duration);
        float t = animTime / motionGraph->Duration;
       
        for (int i = 0; i < skeleton->Bones.Count(); i++)
        {
            p.Transforms[i] = BoneTransformation::Lerp(lastState->Pose.Transforms[i], nextState->Pose.Transforms[i], t);
        }  
        auto offset = lastState->Velocities[0] * t + rootLocation;
        p.Transforms[0].Translation.x = offset.x;
        p.Transforms[0].Translation.z = offset.z;
    }
}