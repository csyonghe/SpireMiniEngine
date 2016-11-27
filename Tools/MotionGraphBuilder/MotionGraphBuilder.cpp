#include "CoreLib/Basic.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Tokenizer.h"
#include "GameEngineCore/Skeleton.h"
#include "GameEngineCore/MotionGraph.h"
#include "MotionGraphBuilder.h"

namespace GameEngine
{
    using namespace CoreLib::IO;
    using namespace CoreLib::Text;
    using namespace VectorMath;

    namespace Tools
    {
        using Vec3List = List<Vec3>;
        using PoseMatrices = List<Matrix4>;

        static void toQuaternion(Quaternion& q, float x, float y, float z)
        {
            Matrix4 rotx, roty, rotz;
            Matrix4::RotationX(rotx, x);
            Matrix4::RotationY(roty, y);
            Matrix4::RotationZ(rotz, z);
            Matrix4::Multiply(rotz, rotz, roty);
            Matrix4::Multiply(rotx, rotx, rotz);
            q = Quaternion::FromMatrix(rotx.GetMatrix3());
        }

        static void toEulerianAngle(const Quaternion& q, float& x, float& y, float& z)
        {
            y = -atan2(2 * q.y * q.w - 2 * q.x * q.z, 1 - 2 * q.y * q.y - 2 * q.z * q.z);
            x = -atan2(2 * q.x * q.w - 2 * q.y * q.z, 1 - 2 * q.x * q.x - 2 * q.z * q.z);
            z = -asin(2 * q.x * q.y + 2 * q.z * q.w);

            //Matrix4 rotx, roty, rotz;
            //Matrix4::RotationX(rotx, x);
            //Matrix4::RotationY(roty, y);
            //Matrix4::RotationZ(rotz, z);
            //Matrix4::Multiply(rotz, rotz, roty);
            //Matrix4::Multiply(rotx, rotx, rotz);
            //auto q1 = Quaternion::FromMatrix(rotx.GetMatrix3());
            //float t = q1.x;
        }

        List<Pose> GetPoseSequence(const Skeleton & skeleton, const SkeletalAnimation & anim)
        {
            List<Pose> result;
            for (int i = 0; i < anim.Channels.First().KeyFrames.Count(); i++)
            {
                Pose p;
                p.Transforms.SetSize(skeleton.Bones.Count());
                for (int j = 0; j < skeleton.Bones.Count(); j++)
                {
                    p.Transforms[j] = skeleton.Bones[j].BindPose;
                }
                for (auto & channel : anim.Channels)
                {
                    int boneId = -1;
                    if (skeleton.BoneMapping.TryGetValue(channel.BoneName, boneId))
                    {
                        p.Transforms[boneId] = channel.KeyFrames[i].Transform;
                    }
                }
                
                result.Add(_Move(p));
            }
            return result;
        }

        List<PoseMatrices> ConvertPoseToMatrices(const Skeleton & skeleton, const List<Pose> & poses)
        {
            List<PoseMatrices> result;

            for (int i = 0; i < poses.Count(); i++)
            {
                PoseMatrices matrices;
                matrices.SetSize(poses[0].Transforms.Count());
                for (int j = 0; j < matrices.Count(); j++)
                    matrices[j] = poses[i].Transforms[j].ToMatrix();
                for (int j = 0; j < skeleton.Bones.Count(); j++)
                {
                    auto tmp = matrices[j];
                    if (skeleton.Bones[j].ParentId != -1)
                        VectorMath::Matrix4::Multiply(matrices[j], matrices[skeleton.Bones[j].ParentId], tmp);
                }
                result.Add(_Move(matrices));
            }
            return result;
        }

        List<float> GetYawAngularVelocity(List<float> & yawAngles, const List<Pose> & poses)
        {
            List<float> yawVelocities;
            yawAngles.SetSize(poses.Count());
            
            for (int i = 0; i < poses.Count(); i++)
            {
                float x = 0, y = 0, z = 0;
                toEulerianAngle(poses[i].Transforms[0].Rotation, x, y, z);
                yawAngles[i] = y;
            }

            for (int i = 1; i < poses.Count(); i++)
            {
                yawVelocities.Add(yawAngles[i] - yawAngles[i - 1]);
            }
            yawVelocities.Add(yawVelocities.Last());

            return yawVelocities;
        }

        void SetRootYawZero(const List<Pose> & poses)
        {
            for (auto & pose : poses)
            {
                float x = 0, y = 0, z = 0;
                toEulerianAngle(pose.Transforms[0].Rotation, x, y, z);
                y = 0.0f;
                toQuaternion(pose.Transforms[0].Rotation, x, y, z);
            }
        }
        
        Quaternion ExtractYawRotation(Quaternion q)
        {
            float x = 0, y = 0, z = 0;
            toEulerianAngle(q, x, y, z);
            x = 0.0f;
            z = 0.0f;
            y = -y;
            Quaternion rs;
            toQuaternion(rs, x, y, z);
            return rs;
        }

        List<Vec3> GetRootVelocity(const List<Pose> & poses)
        {
            List<Vec3> rootVelocities;
            for (int i = 1; i < poses.Count(); i++)
            {
                rootVelocities.Add(poses[i].Transforms[0].Rotation.ToMatrix3().TransformTransposed(poses[i].Transforms[0].Translation -
                    poses[i-1].Transforms[0].Translation));
            }
            rootVelocities.Add(rootVelocities.Last());
            return rootVelocities;
        }

        void SetRootLocationZero(const List<Pose> & poses)
        {
            for (auto & pose : poses)
            {
                pose.Transforms[0].Translation.x = 0.f;
                pose.Transforms[0].Translation.z = 0.f;
            }
        }

        void GetBoneLocationsAndVelocities(const Skeleton & skeleton, 
            const List<Pose> & poses, 
            List<Vec3List> & boneLocations, 
            List<Vec3List> & boneVelocity)
        {
            auto poseMatrices = ConvertPoseToMatrices(skeleton, poses);
            int numPose = poseMatrices.Count();
            int numBone = poseMatrices[0].Count();
            boneLocations.SetSize(numPose);
            boneVelocity.SetSize(numPose);

            for (int i = 0; i < numPose; i++)
            {
                boneLocations[i].SetSize(numBone);
                for (int j = 0; j < numBone; j++)
                {
                    boneLocations[i][j].x = poseMatrices[i][j].values[12];
                    boneLocations[i][j].y = poseMatrices[i][j].values[13];
                    boneLocations[i][j].z = poseMatrices[i][j].values[14];
                }
            }

            for (int i = 0; i < numPose; i++)
            {
                boneVelocity[i].SetSize(numBone);
                for (int j = 0; j < numBone; j++)
                {
                    if (i > 0)
                    {
                        boneVelocity[i][j].x = boneLocations[i][j].x - boneLocations[i - 1][j].x;
                        boneVelocity[i][j].y = boneLocations[i][j].y - boneLocations[i - 1][j].y;
                        boneVelocity[i][j].z = boneLocations[i][j].z - boneLocations[i - 1][j].z;
                    }
                    else
                    {
                        boneVelocity[i][j].x = boneLocations[i + 1][j].x - boneLocations[i][j].x;
                        boneVelocity[i][j].y = boneLocations[i + 1][j].y - boneLocations[i][j].y;
                        boneVelocity[i][j].z = boneLocations[i + 1][j].z - boneLocations[i][j].z;
                    }
                }
            }
        }

        void GetContactLabel(List<ContactLabel> & contactLabelsAll, 
            const List<Vec3List> & boneLocations, 
            const List<Vec3List> & boneVelocities,
            float LeftFloorHeight,
            float RightFloorHeight,
            float footHeightThreshold,
            float footVelocityThreshold)
        {            
            // 3-LeftFoot, 7-RightFoot
            int numPoses = boneLocations.Count();
            
            for (int i = 0; i < numPoses; i++)
            {
                bool left = boneLocations[i][3].y < LeftFloorHeight + footHeightThreshold &&  abs(boneVelocities[i][3].y) < footVelocityThreshold;
                bool right = boneLocations[i][7].y < RightFloorHeight + footHeightThreshold &&  abs(boneVelocities[i][7].y) < footVelocityThreshold;
                if (left && right)
                    contactLabelsAll.Add(ContactLabel::BothFeetOnFloor);
                else if (left)
                    contactLabelsAll.Add(ContactLabel::LeftFootOnFloor);
                else if (right)
                    contactLabelsAll.Add(ContactLabel::RightFootOnFloor);
                else
                    contactLabelsAll.Add(ContactLabel::InAir);
            }
        }

        float CalculateVec3Distance(VectorMath::Vec3 v1, VectorMath::Vec3 v2)
        {
            auto diff = v2 - v1;
            float dist = Vec3::Dot(diff, diff);
            return sqrt(dist);
        }

        float CalculateStateDistance(const Vec3List & lastLocations, const Vec3List & currLocations,
            const Vec3List & lastVelocities, const Vec3List & currVelocities,
            const float * boneWeights)
        {
            int numBone = lastLocations.Count();
            float distance = 0.0f;
            float locationWeight = 0.5f;
            float velocityWeight = 0.5f;

            for (int i = 0; i < numBone; i++)
            {
                distance = distance +
                    boneWeights[i] * 
                    (locationWeight * CalculateVec3Distance(lastLocations[i], currLocations[i]) +
                    velocityWeight * CalculateVec3Distance(lastVelocities[i], currVelocities[i]));
            }
            return distance;
        }
 
        int AddConnections(MotionGraph & graph, 
            const List<Vec3List> & locations,
            const List<Vec3List> & velocities,
            const float * boneWeights,
            int minGap, 
            float distanceThreshold)
        {
            int edgesAdded = 0;

            int numStates = graph.States.Count();
            for (int i = 0; i < numStates; i++)
            {
                auto & state = graph.States[i];
                if (i != numStates - 1 && graph.States[i + 1].Contact == state.Contact)
                    continue;

                
                int lastSameSequenceConnection = i;
                for (int j = 0; j < numStates; j++)
                {
                    if (i == j || 
                        (graph.States[i].Sequence == graph.States[j].Sequence && (abs(j - lastSameSequenceConnection) < minGap || abs(j - i) < minGap)))
                        continue;
                    if (i != numStates - 1 && graph.States[i + 1].Contact != graph.States[j].Contact)
                        continue;
                    if (j != 0 && state.Contact != graph.States[j - 1].Contact)
                        continue;
                    if (state.Contact == graph.States[j].Contact)
                        continue;

                    if (CalculateStateDistance(locations[i], locations[j], 
                        velocities[i], velocities[j],
                        boneWeights) <= distanceThreshold)
                    {
                        state.ChildrenIds.Add(j);
                        graph.States[j].ChildrenIds.Add(i);  
                        edgesAdded++;
                        if(graph.States[i].Sequence == graph.States[j].Sequence)
                            lastSameSequenceConnection = j;
                    }
                }
            }
            return edgesAdded;
        }

        int CullDeadEnd(MotionGraph & graph)
        {
            List<List<int>> parentIds;
            List<int> walkList;
            HashSet<int> deletedStates;
            int numStates = graph.States.Count();
            parentIds.SetSize(numStates);

            for (int i = 0; i < numStates; i++)
            {
                if (graph.States[i].ChildrenIds.Count() == 0)
                {
                    deletedStates.Add(i);
                    walkList.Add(i);
                }

                for (int childId : graph.States[i].ChildrenIds)
                {
                    parentIds[childId].Add(i);
                }
            }

            for (int i = 0; i < walkList.Count(); i++)
            {
                for (int parentId : parentIds[walkList[i]])
                {
                    graph.States[parentId].ChildrenIds.Remove(walkList[i]);
                    if (graph.States[parentId].ChildrenIds.Count() == 0)
                    {
                        if (deletedStates.Add(parentId))
                            walkList.Add(parentId);
                    }
                }
            }

            List<MGState> newStates;
            Dictionary<int, int> idMapping;
            for (int i = 0; i < numStates; i++)
            {
                if (deletedStates.Contains(i))
                    continue;

                idMapping[i] = newStates.Count();
                newStates.Add(_Move(graph.States[i]));
            }
            for (int i = 0; i < newStates.Count(); i++)
            {
                for (auto & child : newStates[i].ChildrenIds)
                {
                    child = idMapping[child];
                }
            }
            graph.States = _Move(newStates);
            

            return deletedStates.Count();
        }

        MotionGraph CreateTestMotionGraph(const CoreLib::String & filename)
        {
            MotionGraph graph;
            graph.Duration = 1.0f / 120.f;
            graph.Speed = 0.003f;

            auto lines = Split(File::ReadAllText(filename), L'\n');

            String parentPath = Path::GetDirectoryName(filename);

            Skeleton skeleton;
            skeleton.LoadFromFile(Path::Combine(parentPath, lines[0]));

            String dir = Path::Combine(parentPath, lines[1]);
            auto anims = From(lines).Skip(2).Select([&](String name)
            {
                SkeletalAnimation anim;
                anim.LoadFromFile(Path::Combine(dir, name));
                return anim;
            }).ToList();

            auto poses = GetPoseSequence(skeleton, anims[0]);
            for (int i = 0; i < 12; i++)
            {
                MGState state;
                state.Pose = poses[0];
                state.Pose.Transforms[0].Rotation.x = 0.f;
                state.Pose.Transforms[0].Rotation.y = 0.f;
                state.Pose.Transforms[0].Rotation.z = 0.f;
                graph.States.Add(state);
            }
            graph.States[0].Velocity = Vec3::Create(10.0f, 0.0f, 0.0f);
            graph.States[1].Velocity = Vec3::Create(10.0f, 0.0f, 0.0f);
            graph.States[2].Velocity = Vec3::Create(10.0f, 0.0f, 0.0f);
            graph.States[3].Velocity = Vec3::Create(10.0f, 0.0f, 0.0f);
            graph.States[4].Velocity = Vec3::Create(10.0f, 0.0f, 0.0f);
            graph.States[5].Velocity = Vec3::Create(20.0f, 0.0f, 0.0f);
            graph.States[6].Velocity = Vec3::Create(10.0f, 0.0f, 0.0f);
            graph.States[7].Velocity = Vec3::Create(22.4f, 0.0f, 0.0f);
            graph.States[8].Velocity = Vec3::Create(22.4f, 0.0f, 0.0f);
            graph.States[9].Velocity = Vec3::Create(10.0f, 0.0f, 0.0f);
            graph.States[10].Velocity = Vec3::Create(20.0f, 0.0f, 0.0f);
            graph.States[11].Velocity = Vec3::Create(10.0f, 0.0f, 0.0f);

            graph.States[0].ChildrenIds.Add(1);
            graph.States[0].ChildrenIds.Add(5);
            graph.States[1].ChildrenIds.Add(2);
            graph.States[1].ChildrenIds.Add(7);
            graph.States[2].ChildrenIds.Add(3);
            graph.States[3].ChildrenIds.Add(4);
            graph.States[5].ChildrenIds.Add(6);
            graph.States[6].ChildrenIds.Add(10);
            graph.States[10].ChildrenIds.Add(11);
            graph.States[11].ChildrenIds.Add(3);
            graph.States[7].ChildrenIds.Add(8);
            graph.States[8].ChildrenIds.Add(9);

            graph.States[5].YawAngularVelocity = -Math::Pi / 2;
            graph.States[6].YawAngularVelocity = Math::Pi / 2;
            graph.States[10].YawAngularVelocity = Math::Pi / 2;
            graph.States[11].YawAngularVelocity = -Math::Pi / 2;
            graph.States[7].YawAngularVelocity = Math::Pi / 4;
            graph.States[8].YawAngularVelocity = -Math::Pi / 4;

            return graph;
        }

        MotionGraph BuildMotionGraph(const CoreLib::String & filename)
        {
            MotionGraph graph;
            auto lines = Split(File::ReadAllText(filename), L'\n');
            
            String parentPath = Path::GetDirectoryName(filename);

            Skeleton skeleton;
            skeleton.LoadFromFile(Path::Combine(parentPath, lines[0]));
            
            String dir = Path::Combine(parentPath, lines[1]);
            auto anims = From(lines).Skip(2).Select([&](String name)
            {
                SkeletalAnimation anim; 
                anim.LoadFromFile(Path::Combine(dir, name)); 
                return anim; 
            }).ToList();

            if (anims.Count() > 0)      // Assuming frames in graph will share the same duration
            {
                graph.Duration = anims[0].Duration / anims[0].Channels.First().KeyFrames.Count();
                graph.Speed = anims[0].Speed;
            }
            
            int sequenceId = 0;
            List<Vec3List> boneLocationsAll;
            List<Vec3List> boneVelocitiesAll;
            List<ContactLabel> contactLabelsAll;
            
            for (auto & anim : anims)
            {
                auto poses = GetPoseSequence(skeleton, anim);
                List<Vec3> rootVelocity = GetRootVelocity(poses);
                List<float> yawAngles;
                List<float> yawRootVelocities = GetYawAngularVelocity(yawAngles, poses);
                SetRootYawZero(poses);
                SetRootLocationZero(poses);

                List<Vec3List> boneLocations;
                List<Vec3List> boneVelocities;
                GetBoneLocationsAndVelocities(skeleton, poses, boneLocations, boneVelocities);
                boneLocationsAll.AddRange(boneLocations);
                boneVelocitiesAll.AddRange(boneVelocities);
                GetContactLabel(contactLabelsAll, boneLocations, boneVelocities, 3.2f, 3.5f, 0.1f, 0.1f);

                for (int i = 0; i < poses.Count(); i++)
                {
                    MGState state;
                    state.Pose = poses[i];
                    state.Contact = contactLabelsAll[i];
                    state.Sequence = sequenceId;
                    state.Velocity = rootVelocity[i];
                    state.YawAngularVelocity = yawRootVelocities[i];
                    if (i != poses.Count() - 1)
                        state.ChildrenIds.Add(graph.States.Count() + 1);
                    graph.States.Add(_Move(state));
                }
                sequenceId++;
            }
            printf("%d states loaded. \n", graph.States.Count());

            float weights[21] = { 1.0f,         // pelvis
                1.0f, 1.0f, 1.5f, 1.5f,         // lfemur, ltibia, lfoot, ltoes
                1.0f, 1.0f, 1.5f, 1.5f,         // rfemur, rtibia, rfoot, rtoes
                1.0f, 1.0f,                     // lowerback, upperback, 
                1.0f, 1.0f, 1.2f, 1.2f,         // lclavicle, lhumerus, lradius, lhand, 
                1.0f, 1.0f, 1.2f, 1.2f,         // rclavicle, rhumerus, rradius, rhand
                1.0f, 1.0f                      // lowerNeck, Neck
            };

            int edges = AddConnections(graph, boneLocationsAll, boneVelocitiesAll, weights, 120, 27.0f);
            printf("%d edges added. \n", edges);

            int numRemovedStates = CullDeadEnd(graph);
            printf("%d states removed. \n", numRemovedStates);
            return graph;
        }
    }
}

using namespace GameEngine;

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2)
    {
        printf("Usage: MotionGraphBuilder -dataset <filename>\n");
        return -1;
    }
    if (String::FromWString(argv[1]) == "-dataset")
    {
        MotionGraph graph = Tools::BuildMotionGraph(String::FromWString(argv[2]));
        graph.SaveToFile(Path::ReplaceExt(String::FromWString(argv[2]), "mog"));
    }
    else
        printf("Invalid arguments.\n");
	return 0;
}

//void TestContactLabel(const MotionGraph & graph,
//    const List<Vec3List> & boneLocationsAll,
//    const List<Vec3List> & boneVelocitiesAll,
//    const List<ContactLabel> & contactLabelsAll)
//{
//    StreamWriter writer("C:\\Users\\yangy\\Desktop\\VisualizeContact\\contactLabels.txt");
//    //writer.Write(String(L"LeftFootLocation, LeftFootVelocity, RightFootLocation, RightFootVelocity, Label \n"));
//    int lastSequenceNum = -1;
//    for (int i = 0; i < graph.States.Count(); i++)
//    {
//        if (graph.States[i].Sequence != lastSequenceNum)
//        {
//            lastSequenceNum = graph.States[i].Sequence;
//            //writer.Write(String(lastSequenceNum) + String(L"\n"));
//        }
//        writer.Write(String(boneLocationsAll[i][3].y) + String(L" ") +
//            String(boneVelocitiesAll[i][3].y) + String(L" ") +
//            String(boneLocationsAll[i][7].y) + String(L" ") +
//            String(boneVelocitiesAll[i][7].y) + String(L" ") +
//            String((int)contactLabelsAll[i]) + String(L"\n"));
//    }
//}
