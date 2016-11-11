#ifndef ANIMATION_SYNTHESIZER_H
#define ANIMATION_SYNTHESIZER_H

#include "Skeleton.h"
#include "MotionGraph.h"
#include "CoreLib/LibMath.h"


namespace GameEngine
{
	class AnimationSynthesizer : public CoreLib::Object
	{
	public:
		virtual void GetPose(Pose & p, float time) = 0;
	};

	class SimpleAnimationSynthesizer : public AnimationSynthesizer
	{
	private:
		Skeleton * skeleton = nullptr;
		SkeletalAnimation * anim = nullptr;
	public:
		SimpleAnimationSynthesizer() = default;
		SimpleAnimationSynthesizer(Skeleton * pSkeleton, SkeletalAnimation * pAnim)
			: skeleton(pSkeleton), anim(pAnim)
		{}
		void SetSource(Skeleton * pSkeleton, SkeletalAnimation * pAnim)
		{
			this->skeleton = pSkeleton;
			this->anim = pAnim;
		}
		virtual void GetPose(Pose & p, float time) override;
	};

    class MotionGraphAnimationSynthesizer : public AnimationSynthesizer
    {
    private:
        Skeleton * skeleton = nullptr;
        MotionGraph * motionGraph = nullptr;
        MGState* lastState = nullptr;
        MGState* nextState = nullptr;
        int startId = 0;
        int preventJumpCounter = 0;
        float lastStateTime = 0.f;
        VectorMath::Vec3 rootLocation = VectorMath::Vec3::Create(0.f, 0.f, 0.f);
        CoreLib::Random random;
        CoreLib::List<int> startCandidates;
        void GetStartCandidates();

    public:
        MotionGraphAnimationSynthesizer(Skeleton * pSkeleton, MotionGraph * pMotionGraph)
            : skeleton(pSkeleton), motionGraph(pMotionGraph), random(3571)
        {
            GetStartCandidates();
        }
        virtual void GetPose(Pose & p, float time) override;
    };

}

#endif