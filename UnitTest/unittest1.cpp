#include "stdafx.h"
#include "CppUnitTest.h"
#include "../GameEngineCore/Property.h"
#include "../CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace GameEngine;
using namespace CoreLib;

namespace UnitTest
{		
	TEST_CLASS(PropertyTest)
	{
	public:
		
		TEST_METHOD(IntSerialize)
		{
			GenericProperty<int> p;
			p.SetValue(121);
			StringBuilder sb;
			p.Serialize(sb);
			Assert::AreEqual(sb.ProduceString().Buffer(), "121");
		}
		TEST_METHOD(FloatSerialize)
		{
			GenericProperty<float> p;
			p.SetValue(121.23f);
			StringBuilder sb;
			p.Serialize(sb);

			GenericProperty<float> p1;
			p1.SetStringValue(sb.ProduceString());
			Assert::IsTrue(p1.GetValue() - 121.23f < 1e-9f);
		}
		TEST_METHOD(Vec3Serialize)
		{
			GenericProperty<VectorMath::Vec3> p;
			auto val = VectorMath::Vec3::Create(1.0f, 2.0f, 3.0f);
			p.SetValue(val);
			StringBuilder sb;
			p.Serialize(sb);

			GenericProperty<VectorMath::Vec3> p1;
			p1.SetStringValue(sb.ProduceString());
			Assert::IsTrue((p1.GetValue()-val).Length2() < 1e-5f);
		}
		TEST_METHOD(Mat4Serialize)
		{
			GenericProperty<VectorMath::Matrix4> p;
			VectorMath::Matrix4 val;
			VectorMath::Matrix4::CreateRandomMatrix(val);
			p.SetValue(val);
			StringBuilder sb;
			p.Serialize(sb);

			GenericProperty<VectorMath::Matrix4> p1;
			p1.SetStringValue(sb.ProduceString());
			for (int i = 0; i < 16; i++)
				Assert::IsTrue(abs(p1.GetValue().values[i]-p.GetValue().values[i]) < 1e-5f);
		}
		TEST_METHOD(ListVec3Serialize)
		{
			GenericProperty<List<VectorMath::Vec3>> p;
			List<VectorMath::Vec3> vals;
			vals.Add(VectorMath::Vec3::Create(1.0f, 2.0f, 3.0f));
			vals.Add(VectorMath::Vec3::Create(10.0f, 20.0f, 30.0f));
			vals.Add(VectorMath::Vec3::Create(100.0f, 200.0f, 300.0f));

			p.SetValue(vals);
			StringBuilder sb;
			p.Serialize(sb);

			auto text = sb.ProduceString();

			GenericProperty<List<VectorMath::Vec3>> p1;
			p1.SetStringValue(text);
			auto vals1 = p1.GetValue();

			Assert::IsTrue(vals.Count() == vals1.Count());
			for (int i = 0; i < vals.Count(); i++)
				Assert::IsTrue((vals[i] - vals1[i]).Length2() < 1e-5f);
		}

		TEST_METHOD(ListStringSerialize)
		{
			GenericProperty<List<String>> p;
			List<String> vals;
			vals.Add("first string");
			vals.Add("");
			vals.Add("thrid\n\"String\"\\//");

			p.SetValue(vals);
			StringBuilder sb;
			p.Serialize(sb);

			auto text = sb.ProduceString();

			GenericProperty<List<String>> p1;
			p1.SetStringValue(text);
			auto vals1 = p1.GetValue();

			Assert::IsTrue(vals.Count() == vals1.Count());
			for (int i = 0; i < vals.Count(); i++)
				Assert::AreEqual(vals[i].Buffer(), vals1[i].Buffer());
		}

		TEST_METHOD(MetaStrTest)
		{
			struct Container : public PropertyContainer
			{
			public:
				PROPERTY(CoreLib::List<String>, strList);
				PROPERTY(int, intProp);
				PROPERTY_ATTRIB(float, floatProp, "COLOR");
			};
			Container c;
			auto propName = c.strList.GetName();
			auto propTypeName = c.strList.GetTypeName();
			Assert::AreEqual(propTypeName, "CoreLib::List<String>");
			Assert::AreEqual(propName, "strList");

			auto propAttribName = c.floatProp.GetAttribute();
			Assert::AreEqual(propAttribName, "COLOR");

			auto list = c.GetPropertyList();
			Assert::AreEqual(list.Count(), 3);
			Assert::AreEqual(list[0]->GetName(), "strList");
			Assert::AreEqual(list[1]->GetName(), "intProp");
			Assert::AreEqual(list[2]->GetName(), "floatProp");

		}

		TEST_METHOD(CustomSerialize)
		{
			struct CustomType
			{
				int t;
				String b;
				void Serialize(StringBuilder & sb)
				{
					sb << t << " " << CoreLib::Text::EscapeStringLiteral(b);
				}
				void Parse(CoreLib::Text::TokenReader & parser)
				{
					t = parser.ReadInt();
					b = parser.ReadStringLiteral();
				}
			};
			GenericProperty<CustomType> p;
			CustomType val;
			val.t = 1682;
			val.b = "test str";

			p.SetValue(val);
			StringBuilder sb;
			p.Serialize(sb);

			auto text = sb.ProduceString();

			GenericProperty<CustomType> p1;
			p1.SetStringValue(text);
			auto val1 = p1.GetValue();

			Assert::IsTrue(val.t == val1.t && val.b == val1.b);
		}
	};
}