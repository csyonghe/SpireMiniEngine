#include "SkeletalMeshActor.h"
#include "Engine.h"

namespace GameEngine
{
	bool SkeletalMeshActor::ParseField(Level * level, CoreLib::Text::TokenReader & parser, bool &isInvalid)
	{
		if (Actor::ParseField(level, parser, isInvalid))
			return true;
		if (parser.LookAhead("mesh"))
		{
			parser.ReadToken();
			MeshName = parser.ReadStringLiteral();
			Mesh = level->LoadMesh(MeshName);
			if (!Mesh)
				isInvalid = false;
			return true;
		}
		if (parser.LookAhead("material"))
		{
			if (parser.NextToken(1).Content == "{")
			{
				MaterialInstance = level->CreateNewMaterial();
				MaterialInstance->Parse(parser);
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
			SkeletonName = parser.ReadStringLiteral();
			Skeleton = level->LoadSkeleton(SkeletonName);
			if (!Skeleton)
				isInvalid = true;
			return true;
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
        if (parser.LookAhead("MotionGraph"))
        {
            parser.ReadToken();
            MotionGraphName = parser.ReadStringLiteral();
            MotionGraph = level->LoadMotionGraph(MotionGraphName);
            if (!MotionGraph)
                isInvalid = true;
            return true;
        }
		return false;
	}

	void GameEngine::SkeletalMeshActor::Tick()
	{
		auto time = Engine::Instance()->GetCurrentTime();
        //static float time = 0.0f;
        //time += 0.001f;
        if (Animation)
			Animation->GetPose(nextPose, time);
	}

	void SkeletalMeshActor::OnLoad()
	{
        if (this->SimpleAnimation)
            Animation = new SimpleAnimationSynthesizer(Skeleton, this->SimpleAnimation);
        else if (this->MotionGraph)
        {
            Animation = new SearchGraphAnimationSynthesizer(Skeleton, this->MotionGraph);
            //Animation = new MotionGraphAnimationSynthesizer(Skeleton, this->MotionGraph);
        }
           
		Tick();
	}
}