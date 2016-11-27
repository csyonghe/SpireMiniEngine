#include "Actor.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace VectorMath;

	Vec3 Actor::ParseVec3(CoreLib::Text::TokenReader & parser)
	{
		Vec3 rs;
		parser.Read("[");
		rs.x = (float)parser.ReadDouble();
		rs.y = (float)parser.ReadDouble();
		rs.z = (float)parser.ReadDouble();
		parser.Read("]");
		return rs;
	}
	Vec4 Actor::ParseVec4(CoreLib::Text::TokenReader & parser)
	{
		Vec4 rs;
		parser.Read("[");
		rs.x = (float)parser.ReadDouble();
		rs.y = (float)parser.ReadDouble();
		rs.z = (float)parser.ReadDouble();
		rs.w = (float)parser.ReadDouble();
		parser.Read("]");
		return rs;
	}
	Matrix4 Actor::ParseMatrix4(CoreLib::Text::TokenReader & parser)
	{
		Matrix4 rs;
		parser.Read("[");
		for (int i = 0; i < 16; i++)
			rs.values[i] = (float)parser.ReadDouble();
		parser.Read("]");
		return rs;
	}

	void Actor::Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec3 & v)
	{
		sb << "[";
		for (int i = 0; i < 3; i++)
			sb << v[i] << " ";
		sb << "]";
	}

	void Actor::Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec4 & v)
	{
		sb << "[";
		for (int i = 0; i < 4; i++)
			sb << v[i] << " ";
		sb << "]";
	}

	void Actor::Serialize(CoreLib::StringBuilder & sb, const VectorMath::Matrix4 & v)
	{
		sb << "[";
		for (int i = 0; i < 16; i++)
			sb << v.values[i] << " ";
		sb << "]";
	}

	bool Actor::ParseField(Level * level, CoreLib::Text::TokenReader & parser, bool &)
	{
		if (parser.LookAhead("name"))
		{
			parser.ReadToken();
			Name = parser.ReadStringLiteral();
			return true;
		}
		else if (parser.LookAhead("bounds"))
		{
			parser.ReadToken();
			Bounds.Min = ParseVec3(parser);
			Bounds.Max = ParseVec3(parser);
			return true;
		}
		else if (parser.LookAhead("transform"))
		{
			parser.ReadToken();
			LocalTransform = ParseMatrix4(parser);
			return true;
		}
		else if (parser.LookAhead("component"))
		{
			parser.ReadToken();
			SubComponents.Add(Engine::Instance()->ParseActor(level, parser));
			return true;
		}
		return false;
	}
	void Actor::SerializeFields(CoreLib::StringBuilder & sb)
	{
		sb << "name \"" << Name << "\"\n";
		sb << "transform ";
		Serialize(sb, LocalTransform);
		sb << "\n";
		for (auto & comp : SubComponents)
		{
			sb << "component ";
			comp->SerializeToText(sb);
		}
	}
	void Actor::Parse(Level * level, CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		parser.ReadToken(); // skip class name
		parser.Read("{");
		while (!parser.IsEnd() && !parser.LookAhead("}"))
		{
			if (!ParseField(level, parser, isInvalid))
				parser.ReadToken();
		}
		parser.Read("}");
	}
	void Actor::SerializeToText(CoreLib::StringBuilder & sb)
	{
		sb << GetTypeName() << "\n{\n";
		SerializeFields(sb);
		sb << "}\n";
	}
	Actor::~Actor()
	{
		OnUnload();
	}
}


