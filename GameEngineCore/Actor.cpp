#include "Actor.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace VectorMath;

	void Actor::Parse(Level * plevel, CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		level = plevel;
		auto className = parser.ReadToken(); 
		parser.Read("{");
		while (!parser.IsEnd() && !parser.LookAhead("}"))
		{
			auto propertyNameToken = parser.ReadToken();
			auto propertyName = propertyNameToken.Content.ToLower();
			Property * prop = nullptr;
			auto errorRecover = [&]()
			{
				while (!parser.IsEnd() && !parser.LookAhead("}"))
				{
					if (!properties.ContainsKey(parser.NextToken().Content.ToLower()))
						parser.ReadToken();
					else
						break;
				}
			};
			if (properties.TryGetValue(propertyName, prop))
			{
				try
				{
					prop->ParseValue(parser);
				}
				catch (CoreLib::Text::TextFormatException)
				{
					Print("Cannot parse property '%s'. (line %d)\n", propertyNameToken.Content.Buffer(), propertyNameToken.Position.Line);
					errorRecover();
					isInvalid = true;
				}
			}
			else
			{
				try
				{
					if (!ParseField(propertyNameToken.Content, parser))
					{
						Print("Actor '%s' does not have property '%s'. (line %d)\n", className.Content.Buffer(), propertyNameToken.Content.Buffer(), propertyNameToken.Position.Line);
						errorRecover();
						isInvalid = true;
					}
				}
				catch (CoreLib::Text::TextFormatException)
				{
					Print("Cannot parse property '%s'. (line %d)\n", propertyNameToken.Content.Buffer(), propertyNameToken.Position.Line);
					errorRecover();
					isInvalid = true;
				}
			}
		}
		parser.Read("}");
	}
	void Actor::SerializeToText(CoreLib::StringBuilder & sb)
	{
		sb << GetTypeName() << "\n{\n";
		for (auto & prop : properties)
		{
			sb << prop.Key << " ";
			prop.Value->Serialize(sb);
			sb << "\n";
		}
		SerializeFields(sb);
		sb << "}\n";
	}
	VectorMath::Vec3 Actor::GetPosition()
	{
		auto l = LocalTransform.GetValue();
		return VectorMath::Vec3::Create(l.values[12], l.values[13], l.values[14]);
	}
	Actor::~Actor()
	{
		OnUnload();
	}
}


