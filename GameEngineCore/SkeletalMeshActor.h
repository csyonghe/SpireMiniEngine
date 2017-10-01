#ifndef GAME_ENGINE_SKELETAL_ANIMATED_ACTOR_H
#define GAME_ENGINE_SKELETAL_ANIMATED_ACTOR_H

#include "Actor.h"
#include "RendererService.h"
#include "Model.h"

namespace GameEngine
{
	class RetargetFile;

	class SkeletalMeshActor : public Actor
	{
	private:
		Pose nextPose;
		CoreLib::RefPtr<ModelPhysicsInstance> physInstance;
		ModelDrawableInstance modelInstance;
		float startTime = 0.0f;
		Model * model = nullptr;
		RetargetFile * retargetFile = nullptr;
	protected:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid) override;
	public:
		CoreLib::RefPtr<AnimationSynthesizer> Animation;
		SkeletalAnimation * SimpleAnimation = nullptr;

		CoreLib::String ModelFileName, RetargetFileName, SimpleAnimationName;
		virtual void Tick() override;
		Model * GetModel()
		{
			return model;
		}
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
		virtual void OnUnload() override;
	};
}

#endif