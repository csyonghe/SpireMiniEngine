#include "MotionGraph.h"

using namespace CoreLib;

namespace GameEngine
{
    using namespace CoreLib::IO;

    void MotionGraph::SaveToStream(CoreLib::IO::Stream * stream)
    {
        BinaryWriter writer(stream);

        writer.Write(Speed);
        writer.Write(Duration);
        writer.Write(States.Count());
        for (auto & state : States)
        {
            writer.Write(state.Sequence);
            writer.Write(state.Pose.Transforms);
            writer.Write(state.Contact);
            writer.Write(state.Velocity);
            writer.Write(state.YawAngularVelocity);
            writer.Write(state.ChildrenIds.Count());
            for (auto id : state.ChildrenIds)
            {
                writer.Write(id);
            }
        }

        writer.ReleaseStream();
    }

    void MotionGraph::LoadFromStream(CoreLib::IO::Stream * stream)
    {
        BinaryReader reader(stream);
        
        reader.Read(Speed);
        reader.Read(Duration);
        int numStates = 0;
        reader.Read(numStates);
        for (int i = 0; i < numStates; i++)
        {
            MGState state;
            reader.Read(state.Sequence);
            reader.Read(state.Pose.Transforms);
            reader.Read(state.Contact);
            reader.Read(state.Velocity);
            reader.Read(state.YawAngularVelocity);

            int numChildren = 0;
            reader.Read(numChildren);
            for (int j = 0; j < numChildren; j++)
            {
                int id;
                reader.Read(id);
                state.ChildrenIds.Add(id);
            }
            States.Add(state);
        }

        reader.ReleaseStream();
    }

    void MotionGraph::SaveToFile(const CoreLib::String & filename)
    {
        RefPtr<FileStream> stream = new FileStream(filename, FileMode::Create);
        SaveToStream(stream.Ptr());
        stream->Close();
    }

    void MotionGraph::LoadFromFile(const CoreLib::String & filename)
    {
        RefPtr<FileStream> stream = new FileStream(filename, FileMode::Open);
		LoadFromStream(stream.Ptr());
		stream->Close();
    }

}
