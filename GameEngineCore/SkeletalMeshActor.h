#ifndef GAME_ENGINE_SKELETAL_ANIMATED_ACTOR_H
#define GAME_ENGINE_SKELETAL_ANIMATED_ACTOR_H

#include "Actor.h"
#include "RendererService.h"

namespace GameEngine
{
	class SkeletalMeshActor : public Actor
	{
	private:
		Pose nextPose;
		RefPtr<Drawable> drawable;
		float startTime = 0.0f;
	protected:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid) override;
	public:
		CoreLib::RefPtr<AnimationSynthesizer> Animation;
		Mesh * Mesh = nullptr;
		Skeleton * Skeleton = nullptr;
		SkeletalAnimation * SimpleAnimation = nullptr;

		CoreLib::String MeshName, SkeletonName, SimpleAnimationName;
		Material * MaterialInstance = nullptr;
		virtual void Tick() override;
		Pose & GetCurrentPose()
		{
			return nextPose;
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