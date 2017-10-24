#include "Actor.h"
#include "Engine.h"
#include "Model.h"

namespace GameEngine
{
	using namespace VectorMath;

	bool Actor::ParseField(CoreLib::String name, CoreLib::Text::TokenReader & parser)
	{
		if (name == "transform")
		{
			LocalTransform = ParseMatrix4(parser);
			return true;
		}
		return false;
	}

    void Actor::AddDrawable(const GetDrawablesParameter & params, Drawable * drawable, const CoreLib::Graphics::BBox & bounds)
    {
        drawable->CastShadow = CastShadow;
        drawable->Bounds = bounds;
        if (Engine::Instance()->GetEngineMode() == EngineMode::Editor)
            drawable->RenderCustomDepth = EditorSelected;
        else
            drawable->RenderCustomDepth = RenderCustomDepth;
        params.sink->AddDrawable(drawable);
    }

    void Actor::AddDrawable(const GetDrawablesParameter & params, Drawable * drawable)
    {
        AddDrawable(params, drawable, Bounds);
    }

    void Actor::AddDrawable(const GetDrawablesParameter & params, ModelDrawableInstance * modelInstance)
    {
        for (auto &d : modelInstance->Drawables)
            AddDrawable(params, d.Ptr(), Bounds);
    }

	void Actor::Parse(Level * plevel, CoreLib::Text::TokenReader & parser, bool & isInvalid)
	{
		level = plevel;
		auto className = parser.ReadToken(); 
		parser.Read("{");
		while (!parser.IsEnd() && !parser.LookAhead("}"))
		{
			auto propertyNameToken = parser.ReadToken();
			auto propertyName = propertyNameToken.Content.ToLower();
			auto errorRecover = [&]()
			{
				while (!parser.IsEnd() && !parser.LookAhead("}"))
				{
					if (!FindProperty(parser.NextToken().Content.ToLower().Buffer()))
						parser.ReadToken();
					else
						break;
				}
			};
			if (auto prop = FindProperty(propertyName.Buffer()))
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
		auto propList = GetPropertyList();
		for (auto & prop : propList)
		{
			sb << prop->GetName() << " ";
			prop->Serialize(sb);
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


