#include "SkeletalMeshActor.h"
#include "Engine.h"
#include "Skeleton.h"

namespace GameEngine
{
	void SkeletalMeshActor::UpdateBounds()
	{
		if (physInstance)
		{
			Bounds.Init();
			for (auto & obj : physInstance->objects)
				Bounds.Union(obj->GetBounds());
		}
	}

	void GameEngine::SkeletalMeshActor::Tick()
	{
		auto time = Engine::Instance()->GetTime();
		if (Animation)
			Animation->GetPose(nextPose, time);
		if (physInstance)
		{
			physInstance->SetTransform(*LocalTransform, nextPose, retargetFile);
		}
		UpdateBounds();
	}

	void SkeletalMeshActor::GetDrawables(const GetDrawablesParameter & params)
	{
		if (modelInstance.IsEmpty())
			modelInstance = model->GetDrawableInstance(params);
		modelInstance.UpdateTransformUniform(*LocalTransform, nextPose, retargetFile);
		
		auto insertDrawable = [&](Drawable * d)
		{
			d->CastShadow = CastShadow;
			d->Bounds = Bounds;
			params.sink->AddDrawable(d);
		};
		for (auto & drawable : modelInstance.Drawables)
			insertDrawable(drawable.Ptr());
	}

	void SkeletalMeshActor::OnLoad()
	{
		model = level->LoadModel(*ModelFileName);
		if (RetargetFileName.GetValue().Length())
			retargetFile = level->LoadRetargetFile(*RetargetFileName);
		simpleAnimation = level->LoadSkeletalAnimation(*AnimationFileName);

		if (this->simpleAnimation && model)
			Animation = new SimpleAnimationSynthesizer(model->GetSkeleton(), this->simpleAnimation);
		physInstance = model->CreatePhysicsInstance(level->GetPhysicsScene(), this, nullptr);
		Tick();
	}

	void SkeletalMeshActor::OnUnload()
	{
		physInstance->RemoveFromScene();
	}

	void SkeletalMeshActor::SetLocalTransform(const VectorMath::Matrix4 & val)
	{
		Actor::SetLocalTransform(val);
		if (physInstance)
			physInstance->SetTransform(*LocalTransform, nextPose, retargetFile);
	}
}