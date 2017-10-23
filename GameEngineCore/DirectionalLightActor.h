#ifndef GAME_ENGINE_DIRECTIONAL_LIGHT_ACTOR
#define GAME_ENGINE_DIRECTIONAL_LIGHT_ACTOR

#include "LightActor.h"

namespace GameEngine
{
	class DirectionalLightActor : public LightActor
	{
	public:
		PROPERTY_DEF(bool, EnableCascadedShadows, false);
		PROPERTY_DEF(float, ShadowDistance, 40000.0f);
		PROPERTY_DEF(int, NumShadowCascades, 4);
		PROPERTY_DEF(float, TransitionFactor, 0.8f);
		PROPERTY_DEF(VectorMath::Vec3, Color, VectorMath::Vec3::Create(1.0f, 1.0f, 1.0f));
		PROPERTY_DEF(float, Ambient, 0.2f);
	protected:
		virtual Mesh CreateGizmoMesh() override;
	public:
		virtual CoreLib::String GetTypeName() override
		{
			return "DirectionalLight";
		}
		DirectionalLightActor()
		{
			lightType = LightType::Directional;
		}
	};
}

#endif