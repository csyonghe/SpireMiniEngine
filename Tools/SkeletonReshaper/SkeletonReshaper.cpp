// SkeletonReshaper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "GameEngineCore/Skeleton.h"
#include "CoreLib/Basic.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Tokenizer.h"

using namespace CoreLib;
using namespace CoreLib::Text;
using namespace CoreLib::IO;
using namespace VectorMath;

namespace GameEngine
{
	namespace SkeletonReshapeTool
	{
		class SkeletonNode
		{
		public:
			String BoneName;
			List<String> SourceBones;
			SkeletonNode * Parent = nullptr;
			int Id = 0;
			List<SkeletonNode*> Children;
			int GetParentId()
			{
				return Parent ? Parent->Id : -1;
			}
		};
		class SkeletonReshapeScript
		{
		public:
			List<RefPtr<SkeletonNode>> Nodes;
			bool ReadIf(TokenReader & parser, String expect)
			{
				if (parser.LookAhead(expect))
				{
					parser.Read(expect);
					return true;
				}
				return false;
			}
			void LoadFromFile(String fileName)
			{
				Nodes.Clear();
				Parse(File::ReadAllText(fileName));
			}
			void Parse(String text)
			{
				TokenReader parser(text);
				ParseNode(parser);
			}
			SkeletonNode* ParseNode(TokenReader & parser)
			{
				auto name = parser.ReadStringLiteral();
				RefPtr<SkeletonNode> node = new SkeletonNode();
				node->Id = Nodes.Count();
				Nodes.Add(node);
				node->BoneName = name;
				if (parser.LookAhead(":"))
				{
					parser.Read(":");
					do
					{
						node->SourceBones.Add(parser.ReadStringLiteral());
					} while (ReadIf(parser, ","));
				}
				else
					node->SourceBones.Add(node->BoneName);
				if (ReadIf(parser, "{"))
				{
					while (!ReadIf(parser, "}"))
					{
						auto child = ParseNode(parser);
						child->Parent = node.Ptr();
						node->Children.Add(child);
					}
				}
				return node.Ptr();
			}
			RefPtr<Skeleton> ReshapeSkeleton(Skeleton * skeleton)
			{
				RefPtr<Skeleton> result = new Skeleton();
				result->Bones.SetSize(Nodes.Count());
				result->InversePose.SetSize(result->Bones.Count());
				for (int i = 0; i < Nodes.Count(); i++)
				{
					result->BoneMapping[result->Bones[i].Name] = i;
					result->Bones[i].Name = Nodes[i]->BoneName;
					result->Bones[i].ParentId = Nodes[i]->GetParentId();
					Matrix4 transform;
					Matrix4::CreateIdentityMatrix(transform);
					for (auto b : Nodes[i]->SourceBones)
					{
						int bid = -1;
						skeleton->BoneMapping.TryGetValue(b, bid);
						if (bid == -1)
						{
							printf("input skeleton does not have bone '%s'.\n", b.Buffer());
							return nullptr;
						}
						Matrix4::Multiply(transform, transform, skeleton->Bones[bid].BindPose.ToMatrix());
					}
					result->Bones[i].BindPose.FromMatrix(transform);
					result->InversePose[i] = transform;
				}
				for (int i = 1; i < Nodes.Count(); i++)
					Matrix4::Multiply(result->InversePose[i], result->InversePose[result->Bones[i].ParentId], result->InversePose[i]);
				for (int i = 0; i < Nodes.Count(); i++)
					result->InversePose[i].Inverse(result->InversePose[i]);
				return result;
			}
			RefPtr<SkeletalAnimation> ReshapeAnimation(Skeleton * newSkeleton, Skeleton * skeleton, SkeletalAnimation * anim)
			{
				const int sampleRate = 20;
				RefPtr<SkeletalAnimation> result = new SkeletalAnimation();
				result->Name = anim->Name;
				result->Speed = anim->Speed;
				result->Duration = anim->Duration;
				EnumerableHashSet<SkeletonNode*> activeChannels;
				for (auto & channel : anim->Channels)
				{
					for (auto & node : Nodes)
						if (node->SourceBones.Contains(channel.BoneName))
							activeChannels.Add(node.Ptr());
				}
				result->Channels.SetSize(activeChannels.Count());
				int sampleCount = (int)(anim->Duration * sampleRate);
				float timeStep = anim->Duration / sampleCount;
				for (auto node : activeChannels)
				{
					AnimationChannel channel;
					channel.BoneName = node->BoneName;
					channel.BoneId = node->Id;
					
					for (int i = 0; i <= sampleCount; i++)
					{
						AnimationKeyFrame kf;
						kf.Time = i * timeStep;
						Matrix4 transform;
						Matrix4::CreateIdentityMatrix(transform);
						for (int j = 0; j < node->SourceBones.Count(); j++)
						{
							auto boneName = node->SourceBones[j];
							int id = anim->Channels.FindFirst([&](AnimationChannel & ch) {return ch.BoneName == boneName; });
							if (id != -1)
							{
								auto m = anim->Channels[id].Sample(kf.Time).ToMatrix();
								Matrix4::Multiply(transform, transform, m);
							}
						}
						kf.Transform.FromMatrix(transform);
						channel.KeyFrames.Add(kf);
					}
					result->Channels.Add(channel);
				}
				return result;
			}
		};
	}
}

using namespace GameEngine::SkeletonReshapeTool;
using namespace GameEngine;

int wmain(int argc, const wchar_t** argv)
{
	try
	{
		bool exportAnim = false;
		SkeletonReshapeScript script;
		Skeleton skeleton;
		SkeletalAnimation anim;
		String skeletonFileName;
		String animFileName;
		for (int i = 0; i < argc - 1; i++)
		{
			auto name = String::FromWString(argv[i]);
			if (name == "-script")
			{
				script.LoadFromFile(String::FromWString(argv[i + 1]));
			}
			else if (name == "-skeleton")
			{
				skeleton.LoadFromFile(String::FromWString(argv[i + 1]));
				skeletonFileName = String::FromWString(argv[i + 1]);
			}
			else if (name == "-anim")
			{
				anim.LoadFromFile(String::FromWString(argv[i + 1]));
				animFileName = String::FromWString(argv[i + 1]);
			}
		}
		if (skeletonFileName.Length())
		{
			auto newSkeleton = script.ReshapeSkeleton(&skeleton);
			if (animFileName.Length())
			{
				auto newAnim = script.ReshapeAnimation(newSkeleton.Ptr(), &skeleton, &anim);
				newAnim->SaveToFile(Path::ReplaceExt(animFileName, "reshaped.anim"));
			}
			newSkeleton->SaveToFile(Path::ReplaceExt(skeletonFileName, "reshaped.skeleton"));
		}
	}
	catch (Exception & e)
	{
		printf("%s\n", e.Message.Buffer() );
	}
    return 0;
}

