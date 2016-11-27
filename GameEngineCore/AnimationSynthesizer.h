#ifndef ANIMATION_SYNTHESIZER_H
#define ANIMATION_SYNTHESIZER_H

#include "Skeleton.h"
#include "MotionGraph.h"
#include "CoreLib/LibMath.h"
#include "CatmullSpline.h"
#include <queue>
#include <stack>
#include <ctime>

namespace GameEngine
{
	class AnimationSynthesizer : public CoreLib::Object
	{
    protected:
        void toQuaternion(VectorMath::Quaternion& q, float x, float y, float z);
        void toEulerianAngle(const VectorMath::Quaternion& q, float& x, float& y, float& z);
        void changeYawAngle(Pose& p, float yaw);
	
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
        BoneTransformation lastStateRootTransform, nextStateRootTransform;
        int transitionGap = 0;
        int minTransitionGap = 60;
        int lastStateId = 0;
        int nextStateId = 0;
        float lastStateTime = 0.f;
        float yaw = 0.f;
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

    class SearchGraphAnimationSynthesizer : public AnimationSynthesizer
    {
    private:
        Skeleton * skeleton = nullptr;
        MotionGraph * motionGraph = nullptr;
        CatmullSpline userPath;
        List<RefPtr<SGState>> path;
        int   lastStateId = -1;
        int   nextStateId = 0;

        int   currentSegmentId = 0;
        float lastStateTime = 0.f;
        float goalThreshold = 2.0f;
        int   minTransitionGap = 60;

    public:
        SearchGraphAnimationSynthesizer(Skeleton * pSkeleton, MotionGraph * pMotionGraph)
            : skeleton(pSkeleton), motionGraph(pMotionGraph) 
        {
            // TODO: set catmull spline user input
            List<Vec2> points;
            points.Add(Vec2::Create(0.0f, 0.0f));
            points.Add(Vec2::Create(100.0f, 0.0f));
            userPath.SetKeyPoints(points);

            printf("start calculating optimal path.\n");
            clock_t start = clock();
            GetOptimalPath(currentSegmentId, currentSegmentId+1);
            clock_t end = clock();
            double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
            printf("%lf seconds.\n", elapsed_secs);
        }

        virtual void GetPose(Pose & p, float time) override;
        void  GetOptimalPath(int start, int end);
        float CalculateCost(const SGState & state, int end);
    };
}

#endif