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

	void SkeletalMeshActor::UpdateStates()
	{
		if (this->simpleAnimation && model)
			Animation = new SimpleAnimationSynthesizer(skeleton?skeleton:model->GetSkeleton(), this->simpleAnimation);
		if (model)
			physInstance = model->CreatePhysicsInstance(level->GetPhysicsScene(), this, nullptr);
		Tick();
	}

	void SkeletalMeshActor::LocalTransform_Changing(VectorMath::Matrix4 & newTransform)
	{
		if (physInstance)
			physInstance->SetTransform(newTransform, nextPose, retargetFile);
		if (errorPhysInstance)
			errorPhysInstance->SetTransform(newTransform);
	}

	void SkeletalMeshActor::ModelFileName_Changing(CoreLib::String & newFileName)
	{
		model = level->LoadModel(newFileName);
		if (!model)
			newFileName = "";
		modelInstance.Drawables.Clear();
		nextPose.Transforms.Clear();
		UpdateStates();
	}

	void SkeletalMeshActor::RetargetFileName_Changing(CoreLib::String & newFileName)
	{
		retargetFile = level->LoadRetargetFile(newFileName);
		if (!retargetFile)
			newFileName = "";
		UpdateStates();
	}

	void SkeletalMeshActor::AnimationFileName_Changing(CoreLib::String & newFileName)
	{
		simpleAnimation = level->LoadSkeletalAnimation(newFileName);
		if (!simpleAnimation)
			newFileName = "";
		UpdateStates();
	}

	void SkeletalMeshActor::SkeletonFileName_Changing(CoreLib::String & newFileName)
	{
		skeleton = level->LoadSkeleton(newFileName);
		if (!skeleton)
			newFileName = "";
		UpdateStates();
	}

	void SkeletalMeshActor::PreviewFrame_Changing(float & newValue)
	{
		previewFrameTime = newValue;
		Tick();
	}

	void GameEngine::SkeletalMeshActor::Tick()
	{
		auto time = Engine::Instance()->GetTime();
		if (Engine::Instance()->GetEngineMode() == EngineMode::Editor)
		{
			time = previewFrameTime;
		}
		if (Animation)
			Animation->GetPose(nextPose, time);
		auto isPoseCompatible = [this]()
		{
			if (!model)
				return false;
			if (!model->GetSkeleton())
				return false;
			if (retargetFile)
			{
				if (retargetFile->RetargetedInversePose.Count() != model->GetSkeleton()->Bones.Count())
					return false;
				if (nextPose.Transforms.Count() < retargetFile->MaxAnimationBoneId + 1)
					return false;
				return true;
			}
			else if (nextPose.Transforms.Count() != model->GetSkeleton()->Bones.Count())
				return false;
			return true;
		};
		disableRetargetFile = false;
		if (!Animation || !isPoseCompatible())
		{
			nextPose.Transforms.Clear();
			if (model)
			{
				auto srcSkeleton = model->GetSkeleton();
				nextPose.Transforms.SetSize(srcSkeleton->Bones.Count());
				for (int i = 0; i < nextPose.Transforms.Count(); i++)
					nextPose.Transforms[i] = srcSkeleton->Bones[i].BindPose;
			}
			disableRetargetFile = true;
		}
		if (physInstance)
		{
			physInstance->SetTransform(*LocalTransform, nextPose, disableRetargetFile ? nullptr : retargetFile);
		}
		if (nextPose.Transforms.Count() == 0 && !errorPhysInstance)
		{
			errorPhysInstance = level->LoadErrorModel()->CreatePhysicsInstance(level->GetPhysicsScene(), this, nullptr);
		}
		else
		{
			errorPhysInstance = nullptr;
		}
		UpdateBounds();
	}

	void SkeletalMeshActor::GetDrawables(const GetDrawablesParameter & params)
	{
		if (model && modelInstance.IsEmpty())
			modelInstance = model->GetDrawableInstance(params);
		if (nextPose.Transforms.Count() == 0)
		{
			if (errorModelInstance.IsEmpty())
				errorModelInstance = level->LoadErrorModel()->GetDrawableInstance(params);
			errorModelInstance.UpdateTransformUniform(*LocalTransform);
			for (auto & drawable : errorModelInstance.Drawables)
			{
				drawable->CastShadow = CastShadow.GetValue();
				TransformBBox(drawable->Bounds, *LocalTransform, level->LoadErrorModel()->GetBounds());
				AddDrawable(params, drawable.Ptr(), drawable->Bounds);
			}
			return;
		}
		
		modelInstance.UpdateTransformUniform(*LocalTransform, nextPose, disableRetargetFile ? nullptr : retargetFile);
        AddDrawable(params, &modelInstance);
	}

	void SkeletalMeshActor::OnLoad()
	{
		model = level->LoadModel(*ModelFileName);
		if (RetargetFileName.GetValue().Length())
			retargetFile = level->LoadRetargetFile(*RetargetFileName);
		simpleAnimation = level->LoadSkeletalAnimation(*AnimationFileName);
		if (SkeletonFileName.GetValue().Length())
			skeleton = level->LoadSkeleton(*SkeletonFileName);
		LocalTransform.OnChanging.Bind(this, &SkeletalMeshActor::LocalTransform_Changing);
		UpdateStates();

		ModelFileName.OnChanging.Bind(this, &SkeletalMeshActor::ModelFileName_Changing);
		RetargetFileName.OnChanging.Bind(this, &SkeletalMeshActor::RetargetFileName_Changing);
		AnimationFileName.OnChanging.Bind(this, &SkeletalMeshActor::AnimationFileName_Changing);
		SkeletonFileName.OnChanging.Bind(this, &SkeletalMeshActor::SkeletonFileName_Changing);
		PreviewFrame.OnChanging.Bind(this, &SkeletalMeshActor::PreviewFrame_Changing);

		previewFrameTime = PreviewFrame.GetValue();
	}

	void SkeletalMeshActor::OnUnload()
	{
		if (physInstance)
			physInstance->RemoveFromScene();
	}

}