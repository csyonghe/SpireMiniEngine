#include "Skeleton.h"

namespace GameEngine
{
	using namespace CoreLib::IO;

	BoneTransformation AnimationChannel::Sample(float animTime)
	{
		BoneTransformation result;
		int frame0 = BinarySearchForKeyFrame(animTime);
		int frame1 = frame0 + 1;
		float t = 0.0f;
		if (frame0 < KeyFrames.Count() - 1)
		{
			t = (animTime - KeyFrames[frame0].Time) / (KeyFrames[frame1].Time - KeyFrames[frame0].Time);
		}
		else
		{
			t = 0.0f;
			frame1 = 0;
		}
		auto & f0 = KeyFrames[frame0];
		auto & f1 = KeyFrames[frame1];
		return BoneTransformation::Lerp(f0.Transform, f1.Transform, t);

	}

    Skeleton Skeleton::TopologySort()
    {
        Skeleton result;
        CoreLib::List<int> workList;
        CoreLib::HashSet<int> visitedBones;
        for (int i = 0; i < Bones.Count(); i++)
        {
            if (Bones[i].ParentId == -1)
            {
                visitedBones.Add(i);
                workList.Add(i);
            }
        }
        CoreLib::Dictionary<int, int> boneMapping;
        for (int i = 0; i < workList.Count(); i++)
        {
            int originalBoneId = workList[i];
            boneMapping[originalBoneId] = result.Bones.Count();
            result.Bones.Add(Bones[originalBoneId]);
            result.InversePose.Add(InversePose[originalBoneId]);
            for (int j = 0; j < Bones.Count(); j++)
            {
                if (visitedBones.Contains(Bones[j].ParentId))
                {
                    if (visitedBones.Add(j))
                        workList.Add(j);
                }
            }
        }
		
        for (auto & bone : result.Bones)
            bone.ParentId = bone.ParentId == -1?-1:boneMapping[bone.ParentId]();
		
        for (auto & mapping : BoneMapping)
            result.BoneMapping[mapping.Key] = boneMapping[mapping.Value]();

        return result;
    }

    void Skeleton::SaveToStream(CoreLib::IO::Stream * stream)
	{
		BinaryWriter writer(stream);
		writer.Write(this->Bones.Count());
		for (int i = 0; i < Bones.Count(); i++)
		{
			writer.Write(Bones[i].Name);
			writer.Write(Bones[i].ParentId);
			writer.Write(InversePose[i]);
			writer.Write(Bones[i].BindPose);
		}
		writer.ReleaseStream();
	}

	void Skeleton::LoadFromStream(CoreLib::IO::Stream * stream)
	{
		BinaryReader reader(stream);
		int boneCount = reader.ReadInt32();
		Bones.SetSize(boneCount);
		InversePose.SetSize(boneCount);
		BoneMapping.Clear();
		for (int i = 0; i < Bones.Count(); i++)
		{
			reader.Read(Bones[i].Name);
			reader.Read(Bones[i].ParentId);
			reader.Read(InversePose[i]);
			reader.Read(Bones[i].BindPose);
			BoneMapping[Bones[i].Name] = i;
		}
		reader.ReleaseStream();
	}

	void Skeleton::SaveToFile(const String & file)
	{
		RefPtr<FileStream> stream = new FileStream(file, FileMode::Create);
		SaveToStream(stream.Ptr());
		stream->Close();
	}

	void Skeleton::LoadFromFile(const String & file)
	{
		RefPtr<FileStream> stream = new FileStream(file, FileMode::Open);
		LoadFromStream(stream.Ptr());
		stream->Close();
	}
	void SkeletalAnimation::SaveToStream(CoreLib::IO::Stream * stream)
	{
		BinaryWriter writer(stream);
		writer.Write(Name);
		writer.Write(Speed);
		writer.Write(Duration);
		writer.Write(Reserved);
		writer.Write(Channels.Count());
		for (int i = 0; i < Channels.Count(); i++)
		{
			writer.Write(Channels[i].BoneName);
			writer.Write(Channels[i].KeyFrames);
		}
		writer.ReleaseStream();
	}
	void SkeletalAnimation::LoadFromStream(CoreLib::IO::Stream * stream)
	{
		BinaryReader reader(stream);
		reader.Read(Name);
		reader.Read(Speed);
		reader.Read(Duration);
		reader.Read(Reserved);
		int channelCount = reader.ReadInt32();
		Channels.SetSize(channelCount);
		for (int i = 0; i < Channels.Count(); i++)
		{
			reader.Read(Channels[i].BoneName);
			reader.Read(Channels[i].KeyFrames);
		}
		reader.ReleaseStream();
	}
	void SkeletalAnimation::SaveToFile(const CoreLib::String & filename)
	{
		RefPtr<FileStream> stream = new FileStream(filename, FileMode::Create);
		SaveToStream(stream.Ptr());
		stream->Close();
	}
	void SkeletalAnimation::LoadFromFile(const CoreLib::String & filename)
	{
		RefPtr<FileStream> stream = new FileStream(filename, FileMode::Open);
		LoadFromStream(stream.Ptr());
		stream->Close();
	}
	void RetargetFile::SaveToStream(CoreLib::IO::Stream * stream)
	{
		BinaryWriter writer(stream);
		writer.Write(SourceSkeletonName);
		writer.Write(TargetSkeletonName);
		writer.Write(RootTranslationScale);
		writer.Write(RetargetedBoneOffsets.Count());
		writer.Write(RetargetedBoneOffsets.Buffer(), RetargetedBoneOffsets.Count());
		writer.Write(RetargetedInversePose.Buffer(), RetargetedInversePose.Count());
		writer.Write(SourceRetargetTransforms.Buffer(), SourceRetargetTransforms.Count());
		writer.Write(ModelBoneIdToAnimationBoneId.Buffer(), ModelBoneIdToAnimationBoneId.Count());
		writer.ReleaseStream();
	}
	void RetargetFile::LoadFromStream(CoreLib::IO::Stream * stream)
	{
		BinaryReader reader(stream);
		reader.Read(SourceSkeletonName);
		reader.Read(TargetSkeletonName);
		reader.Read(RootTranslationScale);
		int retargetTransformCount;
		reader.Read(retargetTransformCount);
		RetargetedBoneOffsets.SetSize(retargetTransformCount);
		reader.Read(RetargetedBoneOffsets.Buffer(), RetargetedBoneOffsets.Count());
		RetargetedInversePose.SetSize(retargetTransformCount);
		reader.Read(RetargetedInversePose.Buffer(), RetargetedInversePose.Count());
		SourceRetargetTransforms.SetSize(retargetTransformCount);
		reader.Read(SourceRetargetTransforms.Buffer(), SourceRetargetTransforms.Count());
		ModelBoneIdToAnimationBoneId.SetSize(retargetTransformCount);
		reader.Read(ModelBoneIdToAnimationBoneId.Buffer(), ModelBoneIdToAnimationBoneId.Count());
		reader.ReleaseStream();
	}
	void RetargetFile::SaveToFile(const CoreLib::String & filename)
	{
		RefPtr<FileStream> stream = new FileStream(filename, FileMode::Create);
		SaveToStream(stream.Ptr());
		stream->Close();
	}
	void RetargetFile::LoadFromFile(const CoreLib::String & filename)
	{
		RefPtr<FileStream> stream = new FileStream(filename, FileMode::Open);
		LoadFromStream(stream.Ptr());
		stream->Close();
	}
	void RetargetFile::SetBoneCount(int count)
	{
		RetargetedInversePose.SetSize(count);
		SourceRetargetTransforms.SetSize(count);
		RetargetedBoneOffsets.SetSize(count);
		ModelBoneIdToAnimationBoneId.SetSize(count);
		for (auto & q : SourceRetargetTransforms)
			q = VectorMath::Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
		for (auto & id : ModelBoneIdToAnimationBoneId)
			id = -1;
	}
}
