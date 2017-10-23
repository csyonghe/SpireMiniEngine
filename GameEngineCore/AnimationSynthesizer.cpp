#include "AnimationSynthesizer.h"
#include <queue>

using namespace CoreLib;

namespace GameEngine
{
    using namespace VectorMath;

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
			if (anim->Channels[i].BoneId != - 1)
			{
				p.Transforms[anim->Channels[i].BoneId] = anim->Channels[i].Sample(animTime);
			}
		}
	}
}