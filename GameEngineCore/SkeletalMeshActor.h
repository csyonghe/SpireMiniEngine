#ifndef GAME_ENGINE_SKELETAL_ANIMATED_ACTOR_H
#define GAME_ENGINE_SKELETAL_ANIMATED_ACTOR_H

#include "Actor.h"
#include "RendererService.h"
#include "InputDispatcher.h"

namespace GameEngine
{
	class SkeletalMeshActor : public Actor
	{
	private:
		Pose mNextPose;
		RefPtr<Drawable> mDrawable;
		float mStartTime = 0.0f;
		float mCurrTime = 0.f;
		float mGapTime = 0.f;
		float mPauseTime = 0.0f;
		int   mPlaybackN = 0;
		bool  mPause = false;
		SimpleAnimationSynthesizeParam mParam;
	
	public:
		CoreLib::RefPtr<AnimationSynthesizer> mAnimation;
		Mesh * mMesh = nullptr;
		Skeleton * mSkeleton = nullptr;
		SkeletalAnimation * mSimpleAnimation = nullptr;
		RetargetFile * mRetargetFile = nullptr;
		CoreLib::String mMeshName, mSkeletonName, mSimpleAnimationName;

	protected:
		bool Pause(const CoreLib::String & name, ActionInput value);
		bool PlayBack(const CoreLib::String & name, ActionInput value);
		bool PlayForward(const CoreLib::String & name, ActionInput value);
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid) override;
	
	public:
		Material * MaterialInstance = nullptr;
		virtual void Tick() override;
		Pose & GetCurrentPose()
		{
			return mNextPose;
		}
		virtual void GetDrawables(const GetDrawablesParameter & params) override;
		virtual void SetLocalTransform(const VectorMath::Matrix4 & val) override;
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Drawable;
		}
		virtual CoreLib::String GetTypeName() override
		{
			return "SkeletalMesh";
		}
		virtual void OnLoad() override;
	};
}

#endif