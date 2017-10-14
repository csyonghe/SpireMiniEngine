#ifndef GAME_ENGINE_SKELETAL_ANIMATED_ACTOR_H
#define GAME_ENGINE_SKELETAL_ANIMATED_ACTOR_H

#include "Actor.h"
#include "RendererService.h"
#include "Model.h"
#include "AnimationSynthesizer.h"

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
		SkeletalAnimation * simpleAnimation = nullptr;
	protected:
		void UpdateBounds();
	public:
		PROPERTY(CoreLib::String, ModelFileName);
		PROPERTY(CoreLib::String, RetargetFileName);
		PROPERTY(CoreLib::String, AnimationFileName);
	public:
		CoreLib::RefPtr<AnimationSynthesizer> Animation;

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