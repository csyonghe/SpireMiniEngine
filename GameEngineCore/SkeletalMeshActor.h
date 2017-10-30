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
		bool disableRetargetFile = false;
		Model * model = nullptr;
		RetargetFile * retargetFile = nullptr;
	protected:
		void UpdateBounds();
		void UpdateStates();
		void LocalTransform_Changing(VectorMath::Matrix4 & newTransform);
		void ModelFileName_Changing(CoreLib::String & newFileName);
		void RetargetFileName_Changing(CoreLib::String & newFileName);
	public:
		PROPERTY_ATTRIB(CoreLib::String, ModelFileName, "resource(Mesh, model)");
		PROPERTY_ATTRIB(CoreLib::String, RetargetFileName, "resource(Animation, retarget)");
	public:
		virtual void Tick() override;
		Model * GetModel()
		{
			return model;
		}
        void SetPose(const Pose & p);
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