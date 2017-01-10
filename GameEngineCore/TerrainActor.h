#ifndef GAME_ENGINE_TERRAIN_ACTOR_H
#define GAME_ENGINE_TERRAIN_ACTOR_H

#include "Actor.h"
#include "RenderContext.h"

namespace GameEngine
{
	class Level;

	class TerrainActor : public Actor
	{
	protected:
		CoreLib::RefPtr<Drawable> drawable;
		Mesh terrainMesh;
		bool localTransformChanged = true;
		void BuildMesh(int w, int h, float cellSpace, float heightScale, CoreLib::ArrayView<unsigned short> heightField);
	protected:
		virtual bool ParseField(CoreLib::Text::TokenReader & parser, bool & isInvalid) override;
	public:
		Material * MaterialInstance = nullptr;

		virtual void OnLoad() override;
		virtual void GetDrawables(const GetDrawablesParameter & params) override;
		virtual void SetLocalTransform(const VectorMath::Matrix4 & val) override;
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Drawable;
		}
		virtual CoreLib::String GetTypeName() override
		{
			return "Terrain";
		}
	};
}

#endif