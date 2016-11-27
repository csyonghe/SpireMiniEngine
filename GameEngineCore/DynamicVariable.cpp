#include "DynamicVariable.h"

namespace GameEngine
{
	DynamicVariable DynamicVariable::Parse(CoreLib::Text::TokenReader & parser)
	{
		DynamicVariable var;
		if (parser.LookAhead("vec2"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Vec2;
			parser.Read("[");
			var.Vec2Value.x = (float)parser.ReadDouble();
			var.Vec2Value.y = (float)parser.ReadDouble();
			parser.Read("]");
		}
		else if (parser.LookAhead("float"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Float;
			parser.Read("[");
			var.FloatValue = (float)parser.ReadDouble();
			parser.Read("]");
		}
		else if (parser.LookAhead("vec3"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Vec3;
			parser.Read("[");
			var.Vec3Value.x = (float)parser.ReadDouble();
			var.Vec3Value.y = (float)parser.ReadDouble();
			var.Vec3Value.z = (float)parser.ReadDouble();

			parser.Read("]");
		}
		else if (parser.LookAhead("vec4"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Vec4;
			parser.Read("[");
			var.Vec4Value.x = (float)parser.ReadDouble();
			var.Vec4Value.y = (float)parser.ReadDouble();
			var.Vec4Value.z = (float)parser.ReadDouble();
			var.Vec4Value.w = (float)parser.ReadDouble();
			parser.Read("]");
		}
		else if (parser.LookAhead("int"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Int;
			parser.Read("[");
			var.FloatValue = (float)parser.ReadInt();
			parser.Read("]");
		}
		else if (parser.LookAhead("texture"))
		{
			parser.ReadToken();
			var.VarType = DynamicVariableType::Texture;
			parser.Read("[");
			var.StringValue = parser.ReadStringLiteral();
			parser.Read("]");
		}
		return var;
	}

}

