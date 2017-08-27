#include "SkeletalMeshActor.h"
#include "Engine.h"
#include "EulerAngle.h"

namespace GameEngine
{
//#define TEST_PATH
	using namespace CoreLib::IO;

	bool SkeletalMeshActor::Pause(const CoreLib::String & /*name*/, ActionInput /*value*/)
	{
		mPause = !mPause;
		if (mPause)
		{
			mPauseTime = mCurrTime;
		}
		else
		{
			mPlaybackN = 0;
			mGapTime = Engine::Instance()->GetTime() - mPauseTime;
		}
		return true;
	}

	bool SkeletalMeshActor::PlayBack(const CoreLib::String & /*name*/, ActionInput /*value*/)
	{
		if (!mPause)
			return true;
		mPlaybackN++;
		mAnimation->GetLastPose(mNextPose, mPlaybackN);
		return true;
	}

	bool SkeletalMeshActor::PlayForward(const CoreLib::String & /*name*/, ActionInput /*value*/)
	{
		if (!mPause)
			return true;
		mPlaybackN--;
		mAnimation->GetLastPose(mNextPose, mPlaybackN);

		return true;
	}

	bool SkeletalMeshActor::ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid)
	{
		if (Actor::ParseField(parser, isInvalid))
			return true;
		if (parser.LookAhead("mesh"))
		{
			parser.ReadToken();
			mMeshName = parser.ReadStringLiteral();
			mMesh = level->LoadMesh(mMeshName);
			Bounds = mMesh->Bounds;
			if (!mMesh)
				isInvalid = false;
			return true;
		}
		if (parser.LookAhead("material"))
		{
			if (parser.NextToken(1).Content == "{")
			{
				MaterialInstance = level->CreateNewMaterial();
				MaterialInstance->Parse(parser);
				MaterialInstance->Name = Name;
			}
			else
			{
				parser.ReadToken();
				auto materialName = parser.ReadStringLiteral();
				MaterialInstance = level->LoadMaterial(materialName);
				if (!MaterialInstance)
					isInvalid = true;
			}
			return true;
		}
		if (parser.LookAhead("Skeleton"))
		{
			parser.ReadToken();
			mSkeletonName = parser.ReadStringLiteral();
			mSkeleton = level->LoadSkeleton(mSkeletonName);
			if (!mSkeleton)
				isInvalid = true;
			return true;
		}
		if (parser.LookAhead("RetargetFile"))
		{
			parser.ReadToken();
			String mRetargetName = parser.ReadStringLiteral();
			mRetargetFile = level->LoadRetargetFile(mRetargetName);
			if (!mRetargetFile)
				isInvalid = true;
			return true;
		}
		if (parser.LookAhead("SimpleAnimation"))
		{
			parser.ReadToken();
			mSimpleAnimationName = parser.ReadStringLiteral();
			mSimpleAnimation = level->LoadSkeletalAnimation(mSimpleAnimationName);
			if (!mSimpleAnimation)
				isInvalid = true;
			return true;
		}
		if (parser.LookAhead("GenerateScript"))
		{
			parser.ReadToken();
			mParam.scriptLength = parser.ReadInt();
			mParam.generateScript = mParam.scriptLength > 0 ? true : false;
			return true;
		}
		if (parser.LookAhead("MotionDatabase"))
		{
			parser.ReadToken();
			auto databaseName = parser.ReadStringLiteral();
			auto actualName = Engine::Instance()->FindFile(databaseName, ResourceType::Graphs);
			if (actualName.Length())
			{
				mParam.database = new GameEngine::MotionDatabase();
				mParam.database->LoadFromFile(actualName);
			}
			return true;
		}
		return false;
	}

	void SkeletalMeshActor::Tick()
	{
		if (mPause)
		{
			mCurrTime = mPauseTime;
		}
		else
		{
			mCurrTime = Engine::Instance()->GetTime() - mGapTime;
		}
		if (mAnimation)
		{
			if (!mPause)
			{
				mAnimation->GetPose(mNextPose, mCurrTime);
			}
		}
	}

	void SkeletalMeshActor::GetDrawables(const GetDrawablesParameter & params)
	{
		if (!mDrawable)
			mDrawable = params.rendererService->CreateSkeletalDrawable(mMesh, mSkeleton, MaterialInstance);
		mDrawable->UpdateTransformUniform(localTransform, mNextPose, mRetargetFile);
		params.sink->AddDrawable(mDrawable.Ptr());
	}

	void SkeletalMeshActor::OnLoad()
	{
		if (this->mSimpleAnimation)
		{
			mParam.skeletalAnimation = mSimpleAnimation;
			mParam.skeleton = mSkeleton;
            mAnimation = new SimpleAnimationSynthesizer(mParam);
			int frameNum = this->mSimpleAnimation->Channels[0].KeyFrames.Count();
#ifdef TEST_PATH
			String filename = "C:\\Users\\yanzhe\\Desktop\\locations_origin.csv";
			StreamWriter writer(filename);
			
			for (int i = 0; i < frameNum; i++)
			{
				auto & root = this->SimpleAnimation->Channels[0].KeyFrames[i];
				if (i % 4 == 0)
				{
					float yaw = GetYawAngle(root.Transform.Rotation);
					writer.Write(String(root.Transform.Translation.x, "%.6f") + ", " + String(root.Transform.Translation.z, "%.6f")  + ", " + String(yaw, "%.6f") + "\n");
				}
			}
			String msg = "Output path to ";
			msg = msg + filename + "\n";
			Print(msg.Buffer());
			writer.Close();
#else
			auto & f0 = mSimpleAnimation->Channels[0].KeyFrames[0];
			auto & f1 = mSimpleAnimation->Channels[0].KeyFrames[frameNum-1];
			Print("Start at (%f, %f).\n", f0.Transform.Translation.x, f0.Transform.Translation.z);
			Print("End at (%f, %f).\n", f1.Transform.Translation.x, f1.Transform.Translation.z);
#endif
		}
		Tick();
		Engine::Instance()->GetInputDispatcher()->BindActionHandler("Pause", ActionInputHandlerFunc(this, &SkeletalMeshActor::Pause));
		Engine::Instance()->GetInputDispatcher()->BindActionHandler("PlayBack", ActionInputHandlerFunc(this, &SkeletalMeshActor::PlayBack));
		Engine::Instance()->GetInputDispatcher()->BindActionHandler("PlayForward", ActionInputHandlerFunc(this, &SkeletalMeshActor::PlayForward));
	}

	void SkeletalMeshActor::SetLocalTransform(const VectorMath::Matrix4 & val)
	{
		Actor::SetLocalTransform(val);
		CoreLib::Graphics::TransformBBox(Bounds, localTransform, mMesh->Bounds);
	}
}