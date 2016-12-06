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
		Drawable, Light, BoundingVolume, Camera, UserController
	};

	class RendererService;

	class Actor : public CoreLib::RefObject
	{
	protected:
		VectorMath::Matrix4 localTransform;

		VectorMath::Vec3 ParseVec3(CoreLib::Text::TokenReader & parser);
		VectorMath::Vec4 ParseVec4(CoreLib::Text::TokenReader & parser);
		VectorMath::Matrix4 ParseMatrix4(CoreLib::Text::TokenReader & parser);
		void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec3 & v);
		void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec4 & v);
		void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Matrix4 & v);

		virtual bool ParseField(Level * level, CoreLib::Text::TokenReader & parser, bool &isInvalid);
		virtual void SerializeFields(CoreLib::StringBuilder & sb);
	public:
		CoreLib::String Name;
		CoreLib::Graphics::BBox Bounds;
		CoreLib::List<CoreLib::RefPtr<Actor>> SubComponents;
		virtual void Tick() { }
		virtual EngineActorType GetEngineType() = 0;
		virtual void OnLoad() {};
		virtual void OnUnload() {};
		virtual void RegisterUI(GraphicsUI::UIEntry *) {}
		virtual void Parse(Level * level, CoreLib::Text::TokenReader & parser, bool & isInvalid);
		virtual void SerializeToText(CoreLib::StringBuilder & sb);
		virtual void GetDrawables(RendererService *) {}
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
			VectorMath::Matrix4::CreateIdentityMatrix(localTransform);
		}
		virtual ~Actor() override;
	};
}

#endif