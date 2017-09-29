#include "SkeletalMeshActor.h"
#include "Engine.h"
#include "Skeleton.h"

namespace GameEngine
{
	bool SkeletalMeshActor::ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("model"))
		{
			parser.ReadToken();
			ModelFileName = parser.ReadStringLiteral();
			model = level->LoadModel(ModelFileName);
			if (!model)
				isInvalid = true;
			else
				Bounds = model->GetBounds();
			return true;
		}
		if (parser.LookAhead("RetargetFile"))
		{
			parser.ReadToken();
			RetargetFileName = parser.ReadStringLiteral();
			retargetFile = level->LoadRetargetFile(RetargetFileName);
		}
		if (parser.LookAhead("SimpleAnimation"))
		{
			parser.ReadToken();
			SimpleAnimationName = parser.ReadStringLiteral();
			SimpleAnimation = level->LoadSkeletalAnimation(SimpleAnimationName);
			if (!SimpleAnimation)
				isInvalid = true;
			return true;
		}
		return false;
	}

	void GameEngine::SkeletalMeshActor::Tick()
	{
		auto time = Engine::Instance()->GetTime();
		if (Animation)
			Animation->GetPose(nextPose, time);
	}

	void SkeletalMeshActor::GetDrawables(const GetDrawablesParameter & params)
	{
		if (modelInstance.IsEmpty())
			modelInstance = model->GetDrawableInstance(params);
		modelInstance.UpdateTransformUniform(localTransform, nextPose, retargetFile);
		Matrix4 rootTransform = localTransform;
		if (nextPose.Transforms.Count())
			Matrix4::Multiply(rootTransform, localTransform, nextPose.Transforms[0].ToMatrix());
		TransformBBox(Bounds, rootTransform, model->GetBounds());
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
		if (this->SimpleAnimation && model)
			Animation = new SimpleAnimationSynthesizer(model->GetSkeleton(), this->SimpleAnimation);
		Tick();
	}

	void SkeletalMeshActor::SetLocalTransform(const VectorMath::Matrix4 & val)
	{
		Actor::SetLocalTransform(val);
	}
}