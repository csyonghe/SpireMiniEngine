#include "CoreLib/Basic.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Tokenizer.h"
#include "GameEngineCore/Skeleton.h"
#include "GameEngineCore/MotionGraph.h"
#include "MotionGraphBuilder.h"
#include "WinForm/WinApp.h"
#include "WinForm/WinForm.h"

namespace GameEngine
{
    using namespace CoreLib::IO;
    using namespace CoreLib::Text;
    using namespace VectorMath;
    using namespace CoreLib::WinForm;

    namespace Tools
    {
        int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
        {
            UINT  num = 0;          // number of image encoders
            UINT  size = 0;         // size of the image encoder array in bytes

            ImageCodecInfo* pImageCodecInfo = NULL;

            GetImageEncodersSize(&num, &size);
            if (size == 0)
                return -1;  // Failure

            pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
            if (pImageCodecInfo == NULL)
                return -1;  // Failure

            GetImageEncoders(num, size, pImageCodecInfo);

            for (UINT j = 0; j < num; ++j)
            {
                if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
                {
                    *pClsid = pImageCodecInfo[j].Clsid;
                    free(pImageCodecInfo);
                    return j;  // Success
                }
            }

            free(pImageCodecInfo);
            return -1;  // Failure
        }

        void VisualizeMotionGraph(MotionGraph*graph, String fileName)
        {
            GdiplusStartupInput gdiplusStartupInput;
            ULONG_PTR gdiplusToken;
            GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
            Gdiplus::Bitmap bitBuffer(22000, 10000, PixelFormat32bppARGB);
            Gdiplus::Graphics graphics(&bitBuffer);
            auto g = &graphics;
            g->Clear(Color(255, 255, 255));
            int lineHeight = 100;
            int lineWidth = 25;
            Dictionary<MGState*, Point> statePos;

            for (auto & state : graph->States)
            {
                int y = (state.Sequence + 1) * lineHeight;
                int x = (state.IdInsequence + 1) * lineWidth; // n th in sequence
                Pen p(Gdiplus::Color(0, 0, 255));
                g->DrawEllipse(&p, x - 4, y - 8, 8, 16);
                statePos[&state] = Point(x, y);
            }
            Pen vbarPen(Color(180, 180, 180), 1.0f);

            Pen arrowPen(Color(130, 130, 40), 1.0f);
            arrowPen.SetEndCap(Gdiplus::LineCap::LineCapArrowAnchor);
            for (auto & state : graph->States)
            {
                auto thisPos = statePos[&state]();
                if (state.IdInsequence % 10 == 0)
                    g->DrawLine(&vbarPen, thisPos.X, 0, thisPos.X, 800);

                for (auto childId : state.ChildrenIds)
                {
                    auto childState = &graph->States[childId];
                    
                    // just visualize walk_turn_left conenctivity
                    if (childState->Sequence != 19 && state.Sequence != 19)
                        continue;


                    auto childPos = statePos[childState]();
                    if (thisPos.Y == childPos.Y && state.IdInsequence != graph->States[childId].IdInsequence - 1)
                    {
                        continue;
                        Point points[3];
                        points[0] = thisPos;
                        points[1].X = (thisPos.X + childPos.X) / 2;
                        points[1].Y = points[0].Y + lineHeight / 2;
                        points[2] = childPos;
                        g->DrawCurve(&arrowPen, points, 3);
                    }
                    else
                        g->DrawLine(&arrowPen, thisPos, childPos);
                }
            }
            CLSID pngClsid;
            GetEncoderClsid(L"image/png", &pngClsid);
            bitBuffer.Save(fileName.ToWString(), &pngClsid);
           
        }

        using Vec3List = List<Vec3>;
        using PoseMatrices = List<Matrix4>;

        void toQuaternion(Quaternion& q, float x, float y, float z)
        {
            // for YZX order
            Matrix4 rotx, roty, rotz;
            Matrix4::RotationX(rotx, x);
            Matrix4::RotationY(roty, y);
            Matrix4::RotationZ(rotz, z);
            Matrix4::Multiply(rotz, rotz, roty);
            Matrix4::Multiply(rotx, rotx, rotz);
            q = Quaternion::FromMatrix(rotx.GetMatrix3());

            // for YXZ order
            //Matrix4 rotx, roty, rotz;
            //Matrix4::RotationX(rotx, x);
            //Matrix4::RotationY(roty, y);
            //Matrix4::RotationZ(rotz, z);
            //Matrix4::Multiply(rotx, rotx, roty);
            //Matrix4::Multiply(rotz, rotz, rotx);
            //q = Quaternion::FromMatrix(rotz.GetMatrix3());
        }

        void toEulerianAngle(const Quaternion& q, float& x, float& y, float& z)
        {
            // for YZX order
            auto matrix3 = q.ToMatrix3().m;
            x = atan2f(matrix3[1][2], matrix3[1][1]);
            y = atan2f(matrix3[2][0], matrix3[0][0]);
            z = asinf(-matrix3[1][0]);
            if (cosf(z) == 0)
                printf("Note: Needs special handling when cosz = 0. \n.");

            // for YXZ order
            //auto matrix3 = q.ToMatrix3().m;
            //x = asinf(matrix3[1][2]);
            //y = atan2f(-matrix3[0][2], matrix3[2][2]);
            //z = atan2f(-matrix3[1][0], matrix3[1][1]);
            //if (cosf(x) == 0)
            //    printf("Note: Needs special handling when cosz = 0. \n.");

            //Quaternion a;
            //toQuaternion(a, x, y, z);
            //int b = 1;
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

            yawVelocities.Add(0.f);
            for (int i = 1; i < poses.Count(); i++)
            {
                yawVelocities.Add(yawAngles[i] - yawAngles[i - 1]);
            }
            yawVelocities[0] = yawVelocities[1];
            
            return yawVelocities;
        }

        void SetRootYawZero(const List<Pose> & poses)
        {
            for (auto & pose : poses)
            {
                float x = 0, y = 0, z = 0;
                toEulerianAngle(pose.Transforms[0].Rotation, x, y, z);
                Matrix4 roty;
                Matrix4::RotationY(roty, -y);
                Matrix4 original = pose.Transforms[0].Rotation.ToMatrix4();
                Matrix4::Multiply(original, roty, original);
                pose.Transforms[0].Rotation = Quaternion::FromMatrix(original.GetMatrix3());
            }
        }
        
        List<Vec3> GetRootVelocity(const List<float>& /*yawAngles*/, const List<Pose> & poses)
        {
            List<Vec3> rootVelocities;
            rootVelocities.Add(Vec3::Create(0.f));
            for (int i = 1; i < poses.Count(); i++)
            {
                //Vec3 velocity = poses[i].Transforms[0].Translation - poses[i - 1].Transforms[0].Translation;
                //Matrix4 roty;
                //Matrix4::RotationY(roty, -yawAngles[i-1]);
                //rootVelocities.Add(roty.TransformNormal(velocity));
                Vec3 velocity = poses[i].Transforms[0].Translation - poses[i - 1].Transforms[0].Translation;
                rootVelocities.Add(poses[i-1].Transforms[0].Rotation.ToMatrix3().TransformTransposed(velocity));
            }
            rootVelocities[0] = rootVelocities[1];
            
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

        struct ContactDescriptor
        {
            int lFootId;
            int rFootId;
            float lFloorHeight;
            float rFloorHeight;
            float heightThreshold;
            float velocityThreshold;
        };

        void GetContactLabel(List<ContactLabel> & contactLabelsAll, const List<Vec3List> & boneLocations, const List<Vec3List> & boneVelocities,
            const ContactDescriptor & desc)

        {            
            int numPoses = boneLocations.Count();
            
            for (int i = 0; i < numPoses; i++)
            {
                if (boneLocations[0].Count() < 8)
                {
                    contactLabelsAll.Add(ContactLabel::BothFeetOnFloor);
                    continue;
                }

                bool left = boneLocations[i][desc.lFootId].y < desc.lFloorHeight + desc.heightThreshold &&
                            abs(boneVelocities[i][3].y) < desc.velocityThreshold;
                bool right = boneLocations[i][desc.rFootId].y < desc.rFloorHeight + desc.heightThreshold &&  
                             abs(boneVelocities[i][7].y) < desc.velocityThreshold;

                if (left && right)
                    contactLabelsAll.Add(ContactLabel::BothFeetOnFloor);
                else if (left)
                    contactLabelsAll.Add(ContactLabel::LeftFootOnFloor);
                else if (right)
                    contactLabelsAll.Add(ContactLabel::RightFootOnFloor);
                else
                {
                    // hack: ensure at least one of feet is on the floor
                    //contactLabelsAll.Add(ContactLabel::InAir);

                    if (boneLocations[i][desc.lFootId].y - desc.lFloorHeight < boneLocations[i][desc.rFootId].y - desc.rFloorHeight)
                    {
                        contactLabelsAll.Add(ContactLabel::LeftFootOnFloor);
                    }
                    else
                    {
                        contactLabelsAll.Add(ContactLabel::RightFootOnFloor);
                    }
                }
                    
            }
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
                    (locationWeight * (lastLocations[i]-currLocations[i]).Length() +
                    velocityWeight * (lastVelocities[i]-currVelocities[i]).Length());
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
            for (int i = 0; i < numStates-1; i++)
            {
                auto & state = graph.States[i];
                if (graph.States[i + 1].Contact == state.Contact)
                    continue;

                int lastSameSequenceConnection = i;
                for (int j = 1; j < numStates; j++)
                {
                    if (i == j ||
                        graph.States[i].ChildrenIds.Contains(j))
                        continue;

                    // condition: no short jump intersequence
                    if (graph.States[i].Sequence == graph.States[j].Sequence 
                        && (abs(j - lastSameSequenceConnection) < minGap || abs(j - i) < minGap))
                        continue;

                    // condition: contact(i+1) = contact(j)
                    if (graph.States[i + 1].Contact != graph.States[j].Contact)
                        continue;

                    // condition: contact(i) = contact(j-1)
                    if (state.Contact != graph.States[j - 1].Contact)
                        continue;

                    // i and i+1 are in the same sequence, j and j-1 are in the same sequence
                    if (graph.States[j - 1].Sequence != graph.States[j].Sequence ||
                        graph.States[i].Sequence != graph.States[i+1].Sequence)
                        continue;

                    // measure the similarity of i+1 and j
                    if (CalculateStateDistance(locations[i+1], locations[j], velocities[i+1], velocities[j], boneWeights)
                        <= distanceThreshold)
                    {
                        state.ChildrenIds.Add(j);
                        graph.States[j - 1].ChildrenIds.Add(i + 1);
                        //graph.States[j].ChildrenIds.Add(i);  
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
            for (int i = 0; i < 9; i++)
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
            graph.States[0].ChildrenIds.Add(1);
            graph.States[1].ChildrenIds.Add(2);
            graph.States[2].ChildrenIds.Add(3);
            graph.States[3].ChildrenIds.Add(4);
            graph.States[4].ChildrenIds.Add(0);
            
            graph.States[5].Velocity = Vec3::Create(10.0f, 0.0f, 0.0f);
            graph.States[6].Velocity = Vec3::Create(20.0f, 0.0f, 0.0f);
            graph.States[7].Velocity = Vec3::Create(10.0f, 0.0f, 0.0f);
            graph.States[8].Velocity = Vec3::Create(20.0f, 0.0f, 0.0f);
            graph.States[5].ChildrenIds.Add(6);
            graph.States[6].ChildrenIds.Add(7);
            graph.States[7].ChildrenIds.Add(8);
            graph.States[8].ChildrenIds.Add(3);
            graph.States[1].ChildrenIds.Add(7);
            graph.States[0].ChildrenIds.Add(5);
            

            graph.States[5].YawAngularVelocity = -Math::Pi / 2;
            graph.States[6].YawAngularVelocity = Math::Pi / 2;
            graph.States[7].YawAngularVelocity = Math::Pi / 2;
            graph.States[8].YawAngularVelocity = -Math::Pi / 2;

            return graph;
        }

        void TestContactLabel(const MotionGraph & graph,
            const List<Vec3List> & boneLocationsAll,
            const List<Vec3List> & boneVelocitiesAll,
            const List<ContactLabel> & contactLabelsAll)
        {
            StreamWriter writer("C:\\Users\\yanzhe\\Desktop\\VisualizeContact\\contactLabels.txt");

            for (int i = 0; i < graph.States.Count(); i++)
            {
                writer.Write(String(boneLocationsAll[i][3].y) + String(" ") +
                    String(boneVelocitiesAll[i][3].y) + String(" ") +
                    String(boneLocationsAll[i][7].y) + String(" ") +
                    String(boneVelocitiesAll[i][7].y) + String(" ") +
                    String((int)contactLabelsAll[i]) + String("\n"));
            }
            printf("Output contact label data to C:\\Users\\yangy\\Desktop\\VisualizeContact\\contactLabels.txt \n");
        }

        MotionGraph BuildMotionGraph(const CoreLib::String & filename, int leftFootId, int rightFootId)
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
                List<float> yawAngles;
                List<float> yawRootVelocities = GetYawAngularVelocity(yawAngles, poses);
                List<ContactLabel> contactLabels;
                List<Vec3> rootVelocity = GetRootVelocity(yawAngles, poses);
                SetRootYawZero(poses);
                SetRootLocationZero(poses);

                List<Vec3List> boneLocations;
                List<Vec3List> boneVelocities;
                GetBoneLocationsAndVelocities(skeleton, poses, boneLocations, boneVelocities);
                boneLocationsAll.AddRange(boneLocations);
                boneVelocitiesAll.AddRange(boneVelocities);
               
                ContactDescriptor desc{leftFootId, rightFootId, 3.3f, 3.55f, 0.1f, 0.1f };
                GetContactLabel(contactLabels, boneLocations, boneVelocities, desc);
                contactLabelsAll.AddRange(contactLabels);

                for (int i = 0; i < poses.Count(); i++)
                {
                    MGState state;
                    state.Pose = poses[i];
                    state.Contact = contactLabels[i];
                    state.Sequence = sequenceId;
                    state.IdInsequence = i;
                    state.Velocity = rootVelocity[i];
                    state.YawAngularVelocity = yawRootVelocities[i];
                    state.LeftFootToRootDistance = boneLocations[i][0].y - boneLocations[i][3].y;
                    state.RightFootToRootDistance = boneLocations[i][0].y - boneLocations[i][7].y;
                    
                    if (i != poses.Count() - 1)
                        state.ChildrenIds.Add(graph.States.Count() + 1);
                    graph.States.Add(_Move(state));
                }
                sequenceId++;
            }
            printf("%d states loaded. \n", graph.States.Count());

            //TestContactLabel(graph, boneLocationsAll, boneVelocitiesAll, contactLabelsAll);

            float weights[21] = { 1.0f,         // pelvis
                1.0f, 1.0f, 1.5f, 1.5f,         // lfemur, ltibia, lfoot, ltoes
                1.0f, 1.0f, 1.5f, 1.5f,         // rfemur, rtibia, rfoot, rtoes
                1.0f, 1.0f,                     // lowerback, upperback, 
                1.0f, 1.0f, 1.2f, 1.2f,         // lclavicle, lhumerus, lradius, lhand, 
                1.0f, 1.0f, 1.2f, 1.2f,         // rclavicle, rhumerus, rradius, rhand
                1.0f, 1.0f                      // lowerNeck, Neck
            };

            int edges = AddConnections(graph, boneLocationsAll, boneVelocitiesAll, weights, 120, 30.0f);
            printf("%d edges added. \n", edges);

            int numRemovedStates = CullDeadEnd(graph);
            printf("%d states removed. \n", numRemovedStates);
            return graph;
        }

        struct QueueElement
        {
            List<int> Path;
            Vec3 Pos;
            float Yaw;
        };

        void PreComputeOptimalMatrix(MotionGraph & graph)
        {
            int numStates = graph.States.Count();
            for (int i = 0; i < numStates; i++)
            {
                if (graph.States[i].ChildrenIds.Count() == 1)   // consider transition states only
                    continue;
                printf("%d ", i);
                HashSet<int> visited;
                List<QueueElement> queue;
                QueueElement start;
                start.Path.Add(i);
                start.Pos = Vec3::Create(0.f);
                start.Yaw = 0.f;
                queue.Add(start);

                while (queue.Count())
                {
                    List<QueueElement> temp;

                    for (auto e : queue)
                    {
                        int j = e.Path.Last();
                        visited.Add(j);

                        if (i != j)
                        {
                            IndexPair p;
                            StateTransitionInfo info;
                            p.Id1 = i;
                            p.Id2 = j;
                            info.DeltaYaw = e.Yaw - start.Yaw;
                            info.DeltaPos = e.Pos - start.Pos;
                            graph.TransitionDictionary[p] = info;
                        }

                        for (auto child : graph.States[j].ChildrenIds)
                        {
                            if (visited.Contains(child))
                                continue;

                            Quaternion rotation = graph.States[j - 1].Pose.Transforms[0].Rotation;
                            Quaternion::SetYawAngle(rotation, e.Yaw);
                            QueueElement eChild = e;
                            eChild.Yaw += graph.States[j].YawAngularVelocity;
                            eChild.Pos += rotation.Transform(graph.States[j].Velocity);
                            eChild.Path.Add(child);
                            temp.Add(eChild);
                        }
                    }
                    queue = _Move(temp);
                    printf(".");
                }
                printf("\n");
            }
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
        MotionGraph graph = Tools::BuildMotionGraph(String::FromWString(argv[2]), 3, 7);
        //MotionGraph graph = Tools::CreateTestMotionGraph(String::FromWString(argv[2]));
        Tools::PreComputeOptimalMatrix(graph);
        graph.SaveToFile(Path::ReplaceExt(String::FromWString(argv[2]), "mog"));
        GameEngine::Tools::VisualizeMotionGraph(&graph, Path::ReplaceExt(String::FromWString(argv[2]), "png"));

        
    }
    else
        printf("Invalid arguments.\n");
	return 0;
}


