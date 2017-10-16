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
		CoreLib::RefPtr<ModelPhysicsInstance> physInstance, errorPhysInstance;
		ModelDrawableInstance modelInstance, errorModelInstance;
		float startTime = 0.0f;
		bool disableRetargetFile = false;
		Model * model = nullptr;
		RetargetFile * retargetFile = nullptr;
		SkeletalAnimation * simpleAnimation = nullptr;
		Skeleton * skeleton = nullptr;
	protected:
		void UpdateBounds();
		void UpdateStates();
		void LocalTransform_Changing(VectorMath::Matrix4 & newTransform);
		void ModelFileName_Changing(CoreLib::String & newFileName);
		void RetargetFileName_Changing(CoreLib::String & newFileName);
		void AnimationFileName_Changing(CoreLib::String & newFileName);
		void SkeletonFileName_Changing(CoreLib::String & newFileName);
		void PreviewFrame_Changing(float & newFrame);
	public:
		PROPERTY_ATTRIB(CoreLib::String, ModelFileName, "resource(Mesh, model)");
		PROPERTY_ATTRIB(CoreLib::String, RetargetFileName, "resource(Animation, retarget)");
		PROPERTY_ATTRIB(CoreLib::String, AnimationFileName, "resource(Animation, anim)");
		PROPERTY_ATTRIB(CoreLib::String, SkeletonFileName, "resource(Animation, skeleton)");
		PROPERTY_DEF(float, PreviewFrame, 0.0f);
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