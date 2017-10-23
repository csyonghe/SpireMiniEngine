#include "BvhFile.h"
#include "CoreLib/Tokenizer.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib;
using namespace CoreLib::IO;
using namespace VectorMath;
using namespace CoreLib::Text;

namespace GameEngine
{
	namespace Tools
	{
		RefPtr<BvhJoint> ParseNode(TokenReader & p)
		{
			RefPtr<BvhJoint> result = new BvhJoint();
			if (p.LookAhead("ROOT") || p.LookAhead("JOINT"))
				p.ReadToken();
			else if (p.LookAhead("End"))
			{
				p.ReadToken();
				p.Read("Site");
				p.Read("{");
				p.Read("OFFSET");
				for (int i = 0; i < 3; i++)
					result->Offset[i] = (float)p.ReadDouble();
				p.Read("}");
				return result;
			}
			else
				throw TextFormatException("Invalid file format: expecting 'ROOT' or 'JOINT'.");
			result->Name = p.ReadToken().Content;
			p.Read("{");
			while (!p.LookAhead("}") && !p.IsEnd())
			{
				if (p.LookAhead("OFFSET"))
				{
					p.ReadToken();
					for (int i = 0; i < 3; i++)
						result->Offset[i] = (float)p.ReadDouble();
				}
				else if (p.LookAhead("CHANNELS"))
				{
					p.ReadToken();
					int count = p.ReadInt();
					for (int i = 0; i < count; i++)
					{
						auto channelName = p.ReadToken().Content;
						ChannelType ct;
						if (channelName == "Xposition")
							ct = ChannelType::XPos;
						else if (channelName == "Yposition")
							ct = ChannelType::YPos;
						else if (channelName == "Zposition")
							ct = ChannelType::ZPos;
						else if (channelName == "Xrotation")
							ct = ChannelType::XRot;
						else if (channelName == "Yrotation")
							ct = ChannelType::YRot;
						else if (channelName == "Zrotation")
							ct = ChannelType::ZRot;
						else if (channelName == "Xscale")
							ct = ChannelType::XScale;
						else if (channelName == "Yscale")
							ct = ChannelType::YScale;
						else if (channelName == "Zscale")
							ct = ChannelType::ZScale;
						else
							throw TextFormatException(String("invalid channel type: ") + channelName);
						result->Channels.Add(ct);
					}
				}
				else if (p.LookAhead("JOINT"))
					result->SubJoints.Add(ParseNode(p));
				else if (p.LookAhead("End"))
				{
					p.ReadToken();
					p.Read("Site");
					p.Read("{");
					p.Read("OFFSET");
					for (int i = 0; i < 3; i++)
						p.ReadDouble();
					p.Read("}");
				}
				else
					throw TextFormatException(String("invalid Bvh field: ") + p.NextToken().Content);
			}
			p.Read("}");
			return result;
		}
		BvhFile BvhFile::FromFile(const CoreLib::String & fileName)
		{
			BvhFile result;
			TokenReader parser(File::ReadAllText(fileName));
			
			if (parser.LookAhead("HIERARCHY"))
			{
				parser.ReadToken();
				result.Hierarchy = ParseNode(parser);
			}

			if (parser.LookAhead("MOTION"))
			{
				parser.ReadToken();
				parser.Read("Frames");
				parser.Read(":");
				int frameCount = parser.ReadInt();
				parser.Read("Frame"); parser.Read("Time"); parser.Read(":");
				result.FrameDuration = (float)parser.ReadDouble();
				while (!parser.IsEnd())
				{
					result.FrameData.Add((float)parser.ReadDouble());
				}
			}
			return result;
		}
	}
}
