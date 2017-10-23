#include "View.h"
#include "Property.h"

namespace GameEngine
{
	void View::Serialize(CoreLib::StringBuilder & sb)
	{
		sb << "{\n";
		sb << "position ";
		GameEngine::Serialize(sb, Position);
		sb << "\norientation ";
		GameEngine::Serialize(sb, VectorMath::Vec3::Create(Yaw, Pitch, Roll));
		sb << "\nznear " << ZNear;
		sb << "\nzfar " << ZFar;
		sb << "\nfov " << FOV;
		sb << "\n}\n";
	}

	void View::Parse(CoreLib::Text::TokenReader & parser)
	{
		parser.Read("{");
		while (!parser.IsEnd() && !parser.LookAhead("}"))
		{
			auto word = parser.ReadWord();
			if (word == "position")
			{
				Position = ParseVec3(parser);
			}
			else if (word == "orientation")
			{
				auto orientation = ParseVec3(parser);
				Yaw = orientation.x;
				Pitch = orientation.y;
				Roll = orientation.z;
			}
			else if (word == "znear")
			{
				ZNear = (float)parser.ReadDouble();
			}
			else if (word == "zfar")
			{
				ZFar = (float)parser.ReadDouble();
			}
			else if (word == "fov")
			{
				FOV = (float)parser.ReadDouble();
			}
		}
		parser.Read("}");
	}
}