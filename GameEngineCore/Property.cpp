#include "Property.h"
#include <typeinfo>
namespace GameEngine
{
	using namespace VectorMath;

	CoreLib::EnumerableDictionary<const char *, PropertyTable> PropertyContainer::propertyTables;

	bool ParseBool(CoreLib::Text::TokenReader & parser)
	{
		if (parser.LookAhead("true"))
		{
			parser.ReadToken();
			return true;
		}
		else if (parser.LookAhead("false"))
		{
			parser.ReadToken();
			return false;
		}
		else
			return parser.ReadInt() != 0;
	}

	VectorMath::Quaternion ParseQuaternion(CoreLib::Text::TokenReader & parser)
	{
		Quaternion rs;
		parser.Read("[");
		rs.x = (float)parser.ReadDouble();
		if (parser.LookAhead(","))
			parser.ReadToken();
		rs.y = (float)parser.ReadDouble();
		if (parser.LookAhead(","))
			parser.ReadToken();
		rs.z = (float)parser.ReadDouble();
		if (parser.LookAhead(","))
			parser.ReadToken();
		rs.w = (float)parser.ReadDouble();
		parser.Read("]");
		return rs;
	}

	VectorMath::Vec2 ParseVec2(CoreLib::Text::TokenReader & parser)
	{
		Vec2 rs;
		parser.Read("[");
		rs.x = (float)parser.ReadDouble();
		if (parser.LookAhead(","))
			parser.ReadToken();
		rs.y = (float)parser.ReadDouble();
		parser.Read("]");
		return rs;
	}

	Vec3 ParseVec3(CoreLib::Text::TokenReader & parser)
	{
		Vec3 rs;
		parser.Read("[");
		rs.x = (float)parser.ReadDouble();
		if (parser.LookAhead(","))
			parser.ReadToken();
		rs.y = (float)parser.ReadDouble();
		if (parser.LookAhead(","))
			parser.ReadToken();
		rs.z = (float)parser.ReadDouble();
		parser.Read("]");
		return rs;
	}
	Vec4 ParseVec4(CoreLib::Text::TokenReader & parser)
	{
		Vec4 rs;
		parser.Read("[");
		rs.x = (float)parser.ReadDouble();
		if (parser.LookAhead(","))
			parser.ReadToken();
		rs.y = (float)parser.ReadDouble();
		if (parser.LookAhead(","))
			parser.ReadToken();
		rs.z = (float)parser.ReadDouble();
		if (parser.LookAhead(","))
			parser.ReadToken();
		rs.w = (float)parser.ReadDouble();
		parser.Read("]");
		return rs;
	}
	Matrix3 ParseMatrix3(CoreLib::Text::TokenReader & parser)
	{
		Matrix3 rs;
		parser.Read("[");
		for (int i = 0; i < 9; i++)
		{
			rs.values[i] = (float)parser.ReadDouble();
			if (parser.LookAhead(","))
				parser.ReadToken();
		}
		parser.Read("]");
		return rs;
	}
	Matrix4 ParseMatrix4(CoreLib::Text::TokenReader & parser)
	{
		Matrix4 rs;
		parser.Read("[");
		for (int i = 0; i < 16; i++)
		{
			rs.values[i] = (float)parser.ReadDouble();
			if (parser.LookAhead(","))
				parser.ReadToken();
		}
		parser.Read("]");
		return rs;
	}

	void Serialize(CoreLib::StringBuilder & sb, bool v)
	{
		if (v)
			sb << "true";
		else
			sb << "false";
	}

	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec2 & v)
	{
		sb << "[";
		sb << v.x << " " << v.y;
		sb << "]";
	}

	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec3 & v)
	{
		sb << "[";
		for (int i = 0; i < 3; i++)
			sb << v[i] << " ";
		sb << "]";
	}

	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec4 & v)
	{
		sb << "[";
		for (int i = 0; i < 4; i++)
			sb << v[i] << " ";
		sb << "]";
	}

	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Quaternion & v)
	{
		sb << "[";
		sb << v.x << " " << v.y << " " << v.z << " " << v.w << "]";
	}

	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Matrix3 & v)
	{
		sb << "[";
		for (int i = 0; i < 9; i++)
			sb << v.values[i] << " ";
		sb << "]";
	}

	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Matrix4 & v)
	{
		sb << "[";
		for (int i = 0; i < 16; i++)
			sb << v.values[i] << " ";
		sb << "]";
	}


	void PropertyContainer::FreeRegistry()
	{
		propertyTables = CoreLib::EnumerableDictionary<const char *, PropertyTable>();
	}

	void PropertyContainer::RegisterProperty(Property * prop)
	{
		auto table = GetPropertyTable();
		if (!table->isComplete)
		{
			String propName = String(prop->GetName).ToLower();
			if (table->entries.ContainsKey(propName))
			{
				table->isComplete = true;
				return;
			}
			table->entries[propName] = (int)(((unsigned char *)prop) - ((unsigned char *)this));
		}
	}

	PropertyTable* PropertyContainer::GetPropertyTable()
	{
		if (propertyTable)
			return propertyTable;
		auto className = typeid(*this).name();
		if (auto table = propertyTables.TryGetValue(className))
			propertyTable = table;
		else
		{
			propertyTables[className] = PropertyTable();
			propertyTable = &propertyTables[className].GetValue();
		}
		return propertyTable;
	}

	Property * PropertyContainer::GetProperty(int offset)
	{
		return (Property*)(((unsigned char *)this) + offset);
	}

	Property * PropertyContainer::FindProperty(const char * name)
	{
		auto table = GetPropertyTable();
		int offset = -1;
		if (table->entries.TryGetValue(name, offset))
		{
			return GetProperty(offset);
		}
		return nullptr;
	}

	CoreLib::List<Property*> PropertyContainer::GetPropertyList()
	{
		CoreLib::List<Property*> rs;
		auto table = GetPropertyTable();
		for (auto & entry : table->entries)
		{
			rs.Add(GetProperty(entry.Value));
		}
		return rs;
	}

}