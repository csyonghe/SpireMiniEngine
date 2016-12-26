#ifndef GAME_ENGINE_STATIC_ACTOR_H
#define GAME_ENGINE_STATIC_ACTOR_H

#include "Actor.h"
#include "RenderContext.h"

namespace GameEngine
{
	class Level;

	class StaticMeshActor : public Actor
	{
	protected:
		CoreLib::RefPtr<Drawable> drawable;
		bool localTransformChanged = true;
	protected:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid) override;
	public:
		CoreLib::String MeshName;
		Mesh * Mesh = nullptr;
		Material * MaterialInstance;
		
		virtual void OnLoad() override;
		virtual void GetDrawables(const GetDrawablesParameter & params) override;
		virtual void SetLocalTransform(const VectorMath::Matrix4 & val) override;
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Drawable;
		}
		virtual CoreLib::String GetTypeName() override
		{
			return "StaticMesh";
		}
	};
}

#endif