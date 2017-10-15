#ifndef GAME_ENGINE_ACTOR_H
#define GAME_ENGINE_ACTOR_H

#include "CoreLib/Basic.h"
#include "CoreLib/Tokenizer.h"
#include "CoreLib/Graphics/BBox.h"
#include "Property.h"

namespace GraphicsUI
{
	class UIEntry;
}

namespace GameEngine
{
	enum class EngineActorType
	{
		Drawable, Light, EnvMap, Atmosphere, BoundingVolume, Camera, UserController, ToneMapping
	};

	class RendererService;
	class DrawableSink;

	struct GetDrawablesParameter
	{
		RendererService * rendererService;
		DrawableSink * sink;
		VectorMath::Vec3 CameraPos, CameraDir;
		bool IsEditorMode = false;
	};

	class Level;

	class Actor : public PropertyContainer
	{
	protected:
		Level * level = nullptr;
	public:
		PROPERTY(    CoreLib::String,   Name);
		PROPERTY_DEF(           bool,   CastShadow, true);
		PROPERTY(VectorMath::Matrix4,   LocalTransform);
	protected:
		virtual bool ParseField(CoreLib::String, CoreLib::Text::TokenReader &);
		virtual void SerializeFields(CoreLib::StringBuilder &) {}
	public:
		CoreLib::Graphics::BBox Bounds;
		CoreLib::List<CoreLib::RefPtr<Actor>> SubComponents;
		virtual void Tick() { }
		virtual EngineActorType GetEngineType() = 0;
		virtual void OnLoad() {};
		virtual void OnUnload() {};
		virtual void RegisterUI(GraphicsUI::UIEntry *) {}
		virtual void Parse(Level * plevel, CoreLib::Text::TokenReader & parser, bool & isInvalid);
		virtual void SerializeToText(CoreLib::StringBuilder & sb);
		virtual void GetDrawables(const GetDrawablesParameter & /*params*/) {}
		virtual CoreLib::String GetTypeName() { return "Actor"; }
		VectorMath::Matrix4 GetLocalTransform()
		{
			return LocalTransform.GetValue();
		}
		virtual void SetLocalTransform(const VectorMath::Matrix4 & val)
		{
			LocalTransform.SetValue(val);
		}
		virtual VectorMath::Vec3 GetPosition();
		Actor()
		{
			Bounds.Init();
			VectorMath::Matrix4 identity;
			VectorMath::Matrix4::CreateIdentityMatrix(identity);
			LocalTransform.SetValue(identity);
		}
		virtual ~Actor() override;
	};
}

#endif