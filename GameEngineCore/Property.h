#ifndef GAME_ENGINE_PROPERTY_H
#define GAME_ENGINE_PROPERTY_H

#include "CoreLib/Basic.h"
#include "CoreLib/Events.h"
#include "CoreLib/Tokenizer.h"
#include "CoreLib/VectorMath.h"
#include <type_traits>

namespace GameEngine
{
	class Property;
	struct PropertyTable
	{
		CoreLib::EnumerableDictionary<CoreLib::String, int> entries;
		bool isComplete = false;
		const char * className = nullptr;
	};
	class PropertyContainer : public CoreLib::RefObject
	{
		friend class Property;
	protected:
		static CoreLib::EnumerableDictionary<const char *, PropertyTable> propertyTables;
		PropertyTable * propertyTable = nullptr;
	public:
		static void FreeRegistry();
		void RegisterProperty(Property* prop);
		PropertyTable * GetPropertyTable();
		Property * GetProperty(int offset);
		Property * FindProperty(const char * name);
		CoreLib::List<Property*> GetPropertyList();
	};

	class Property
	{
	private:
		const char * metaStr;
	public:
		virtual void ParseValue(CoreLib::Text::TokenReader & parser) = 0;
		virtual void Serialize(CoreLib::StringBuilder & sb) = 0;
	public:
		CoreLib::Event<> OnChanged;
		CoreLib::String GetStringValue()
		{
			CoreLib::StringBuilder sb;
			Serialize(sb);
			return sb.ProduceString();
		}
		void SetStringValue(CoreLib::String text)
		{
			CoreLib::Text::TokenReader parser(text);
			ParseValue(parser);
			OnChanged();
		}
		const char * GetTypeName()
		{
			int i = 0;
			int c = 0;
			while (c != 1)
			{
				if (metaStr[i] == '\0')
					c++;
				i++;
			}
			return metaStr + i;
		}
		const char * GetName()
		{
			return metaStr;
		}
		const char * GetAttribute()
		{
			int i = 0;
			int c = 0;
			while (c != 2)
			{
				if (metaStr[i] == '\0')
					c++;
				i++;
			}
			return metaStr + i;
		}
		Property(PropertyContainer * container, const char * pmetaStr)
		{
			metaStr = pmetaStr;
			container->RegisterProperty(this);
		}
		Property()
		{}
	};

	VectorMath::Quaternion ParseQuaternion(CoreLib::Text::TokenReader & parser);
	VectorMath::Vec2 ParseVec2(CoreLib::Text::TokenReader & parser);
	VectorMath::Vec3 ParseVec3(CoreLib::Text::TokenReader & parser);
	VectorMath::Vec4 ParseVec4(CoreLib::Text::TokenReader & parser);
	VectorMath::Matrix3 ParseMatrix3(CoreLib::Text::TokenReader & parser);
	VectorMath::Matrix4 ParseMatrix4(CoreLib::Text::TokenReader & parser);
	bool ParseBool(CoreLib::Text::TokenReader & parser);

	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec2 & v);
	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec3 & v);
	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec4 & v);
	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Quaternion & v);
	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Matrix3 & v);
	void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Matrix4 & v);
	void Serialize(CoreLib::StringBuilder & sb, bool v);

#define PROPERTY_ATTRIB(type, name, attrib) GameEngine::GenericProperty<type> name = GameEngine::GenericProperty<type>(this, #name "\0" #type "\0" attrib, type());
#define PROPERTY_DEF_ATTRIB(type, name, value, attrib) GameEngine::GenericProperty<type> name = GameEngine::GenericProperty<type>(this, #name "\0" #type "\0" attrib, value);
#define PROPERTY(type, name) GameEngine::GenericProperty<type> name = GameEngine::GenericProperty<type>(this,  #name "\0" #type "\0\0", type());
#define PROPERTY_DEF(type, name, value) GameEngine::GenericProperty<type> name = GameEngine::GenericProperty<type>(this,  #name "\0" #type "\0\0", value);
#define PUBLIC_METHODS(type) \
	type GetValue()\
	{\
		return value;\
	}\
	type & operator * ()\
	{\
		return value;\
	}\
	type * operator -> ()\
	{\
		return &value;\
	}\
	operator type ()\
	{\
		return value;\
	}\
	void SetValue(type val)\
	{\
		value = CoreLib::_Move(val);\
		OnChanged();\
	}\
	type & operator = (const type & other)\
	{\
		value = other;\
		OnChanged();\
		return value;\
	}

	template<typename T, bool triviallyCopyable>
	class GenericPropertyHelper : public Property
	{
	private:
		T value;
	public:
		GenericPropertyHelper() = default;
		GenericPropertyHelper(PropertyContainer * container, const char * pmetaStr, T def)
			: Property(container, pmetaStr), value(def)
		{
		}
		virtual void Serialize(CoreLib::StringBuilder & sb) override
		{
			value.Serialize(sb);
		}
		virtual void ParseValue(CoreLib::Text::TokenReader & parser) override
		{
			value.Parse(parser);
		}
	public:
		PUBLIC_METHODS(T)
	};

	template <typename T>
	class hasSerializeFunc
	{
		typedef char one;
		typedef long two;
		template <typename C> static one test(decltype(&C::Serialize));
		template <typename C> static two test(...);
	public:
		static const bool value = sizeof(test<T>(0)) == sizeof(char);
	};

	template <typename T>
	class hasParseFunc
	{
		typedef char one;
		typedef long two;
		template <typename C> static one test(decltype(&C::Parse));
		template <typename C> static two test(...);
	public:
		static const bool value = sizeof(test<T>(0)) == sizeof(char);
	};

	template<typename T>
	using GenericProperty = GenericPropertyHelper<T, std::is_trivially_copyable<T>::value && !hasSerializeFunc<T>::value && !hasParseFunc<T>::value>;

	template<typename T>
	class GenericPropertyHelper<T, true> : public Property
	{
	private:
		T value;
	public:
		GenericPropertyHelper() = default;
		GenericPropertyHelper(PropertyContainer * container, const char * pmetaStr, T def)
			: Property(container, pmetaStr), value(def)
		{
		}
		virtual void Serialize(CoreLib::StringBuilder & sb) override
		{
			sb << "[\"";
			unsigned char * ptr = (unsigned char *)&value;
			for (int i = 0; i < (int)sizeof(value); i++)
			{
				auto ch = String((unsigned int)(ptr[i]), 16);
				if (ch.Length() == 1)
					sb << "0";
				sb << ch;
			}
			sb << "\"]";
		}
		virtual void ParseValue(CoreLib::Text::TokenReader & parser) override
		{
			unsigned char * ptr = (unsigned char *)&value;
			parser.Read("[");
			auto strVal = parser.ReadStringLiteral();
			for (int i = 0; i < strVal.Length() - 1; i += 2)
			{
				int byte0 = strVal[i] - '0';
				if (byte0 > 9) byte0 = strVal[i] - 'A' + 10;
				int byte1 = strVal[i + 1] - '0';
				if (byte1 > 9) byte1 = strVal[i + 1] - 'A' + 10;
				*ptr = (unsigned char)((byte0 << 4) + byte1);
				ptr++;
			}
			parser.Read("]");
		}
	public:
		PUBLIC_METHODS(T)
	};

	template<typename T>
	class GenericPropertyHelper<CoreLib::List<T>, false> : public Property
	{
	private:
		CoreLib::List<T> value;
	public:
		GenericPropertyHelper() = default;
		GenericPropertyHelper(PropertyContainer * container, const char * pmetaStr, CoreLib::List<T> def)
			: Property(container, pmetaStr), value(def)
		{
		}
		virtual void Serialize(CoreLib::StringBuilder & sb) override
		{
			sb << "List " << "\n{\n";
			for (auto & val : value)
			{
				GenericProperty<T> p;
				p.SetValue(val);
				p.Serialize(sb);
				sb << "\n";
			}
			sb << "}\n";
		}
		virtual void ParseValue(CoreLib::Text::TokenReader & parser) override
		{
			unsigned char * ptr = (unsigned char *)&value;
			parser.Read("List");
			parser.Read("{");
			while (!parser.LookAhead("}") && !parser.IsEnd())
			{
				GenericProperty<T> p;
				p.ParseValue(parser);
				value.Add(p.GetValue());
			}
			parser.Read("}");
		}
	public:
		PUBLIC_METHODS(CoreLib::List<T>)
	};

#define BASIC_TYPE_PROPERTY(type, readerFunc, writerExpr) \
	template<> \
	class GenericPropertyHelper<type, std::is_trivially_copyable<type>::value> : public Property \
	{ \
	private: \
		type value; \
	public: \
		GenericPropertyHelper() = default;\
		GenericPropertyHelper(PropertyContainer * container, const char * pmetaStr, type def)\
			: Property(container, pmetaStr), value(def)\
		{ }\
		virtual void Serialize(CoreLib::StringBuilder & sb) override \
		{ \
			sb << writerExpr;\
		} \
		virtual void ParseValue(CoreLib::Text::TokenReader & parser) override\
		{\
			value = (type)parser.readerFunc();\
		}\
		PUBLIC_METHODS(type)\
	};
	BASIC_TYPE_PROPERTY(int, ReadInt, value)
	BASIC_TYPE_PROPERTY(unsigned int, ReadUInt, value)
	BASIC_TYPE_PROPERTY(float, ReadFloat, CoreLib::String(value, "%.8g"))
	BASIC_TYPE_PROPERTY(double, ReadDouble, CoreLib::String(value, "%.15g"))
	BASIC_TYPE_PROPERTY(CoreLib::String, ReadStringLiteral, CoreLib::Text::EscapeStringLiteral(value))
	
	template<>
	class GenericPropertyHelper<bool, std::is_trivially_copyable<bool>::value> : public Property
	{ 
	private: 
		bool value = false;
	public: 
		GenericPropertyHelper() = default; 
			GenericPropertyHelper(PropertyContainer * container, const char * pmetaStr, bool def)
			: Property(container, pmetaStr), value(def)
		{ }
		virtual void Serialize(CoreLib::StringBuilder & sb) override 
		{ 
			GameEngine::Serialize(sb, value);
		} 
		virtual void ParseValue(CoreLib::Text::TokenReader & parser) override
		{
			value = ParseBool(parser);
		}
	public:
		PUBLIC_METHODS(bool)
	};

#define VECTOR_PROPERTY(type, defaultVal, values) \
	template<>\
	class GenericPropertyHelper<VectorMath::type, std::is_trivially_copyable<VectorMath::type>::value> : public Property\
	{\
	private:\
		VectorMath::type value = defaultVal;\
	public:\
		GenericPropertyHelper() = default;\
		GenericPropertyHelper(PropertyContainer * container, const char * pmetaStr, VectorMath::type def)\
			: Property(container, pmetaStr), value(def)\
		{ }\
		virtual void Serialize(CoreLib::StringBuilder & sb) override\
		{\
			sb << "[ ";\
			for (int i = 0; i < sizeof(value) / sizeof(float); i++)\
				sb << CoreLib::String(values(value, i), "%.8g") << " ";\
			sb << "]";\
		}\
		virtual void ParseValue(CoreLib::Text::TokenReader & parser) override\
		{\
			auto word = parser.Read("[");\
			for (int i = 0; i < sizeof(value) / sizeof(float); i++)\
				values(value, i) = parser.ReadFloat();\
			parser.Read("]");\
		}\
		PUBLIC_METHODS(VectorMath::type)\
	};
	template<typename T>
	inline float & GetVectorValue(T & v, int i)
	{
		return v[i];
	}
	template<typename T>
	inline float & GetMatrixValue(T & v, int i)
	{
		return v.values[i];
	}
	VECTOR_PROPERTY(Vec2, VectorMath::Vec2::Create(0.0f), GetVectorValue);
	VECTOR_PROPERTY(Vec3, VectorMath::Vec3::Create(0.0f), GetVectorValue);
	VECTOR_PROPERTY(Vec4, VectorMath::Vec4::Create(0.0f), GetVectorValue);
	VECTOR_PROPERTY(Matrix3, VectorMath::Matrix3(0.0f), GetMatrixValue);
	VECTOR_PROPERTY(Matrix4, VectorMath::Matrix4(0.0f), GetMatrixValue);
}

#endif