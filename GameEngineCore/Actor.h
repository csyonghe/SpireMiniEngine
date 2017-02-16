#ifndef GAME_ENGINE_ACTOR_H
#define GAME_ENGINE_ACTOR_H

#include "CoreLib/Basic.h"
#include "CoreLib/Tokenizer.h"
#include "Mesh.h"
#include "AnimationSynthesizer.h"
#include "Material.h"
#include "CoreLib/Graphics/BBox.h"

namespace GraphicsUI
{
	class UIEntry;
}

namespace GameEngine
{
	enum class EngineActorType
	{
		Drawable, Light, Atmosphere, BoundingVolume, Camera, UserController, ToneMapping
	};

	class RendererService;
	class DrawableSink;

	struct GetDrawablesParameter
	{
		RendererService * rendererService;
		DrawableSink * sink;
		VectorMath::Vec3 CameraPos, CameraDir;
	};

	class Actor : public CoreLib::RefObject
	{
	protected:
		Level * level = nullptr;
		VectorMath::Matrix4 localTransform;
		int ParseInt(CoreLib::Text::TokenReader & parser);
		bool ParseBool(CoreLib::Text::TokenReader & parser);
		VectorMath::Vec3 ParseVec3(CoreLib::Text::TokenReader & parser);
		VectorMath::Vec4 ParseVec4(CoreLib::Text::TokenReader & parser);
		VectorMath::Matrix4 ParseMatrix4(CoreLib::Text::TokenReader & parser);
		void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec3 & v);
		void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec4 & v);
		void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Matrix4 & v);

		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool &isInvalid);
		virtual void SerializeFields(CoreLib::StringBuilder & sb);
	public:
		CoreLib::String Name;
		CoreLib::Graphics::BBox Bounds;
		CoreLib::List<CoreLib::RefPtr<Actor>> SubComponents;
		bool CastShadow = true;
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
			return localTransform;
		}
		virtual void SetLocalTransform(const VectorMath::Matrix4 & val)
		{
			localTransform = val;
		}
		Actor()
		{
			Bounds.Init();
			VectorMath::Matrix4::CreateIdentityMatrix(localTransform);
		}
		virtual ~Actor() override;
	};
}

#endif