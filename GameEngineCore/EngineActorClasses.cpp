#include "Engine.h"
#include "SkeletalMeshActor.h"
#include "StaticMeshActor.h"
#include "CameraActor.h"
#include "FreeRoamCameraController.h"
#include "ArcBallCameraController.h"
#include "AnimationVisualizationActor.h"
#include "DirectionalLightActor.h"
#include "PointLightActor.h"
#include "AtmosphereActor.h"
#include "TerrainActor.h"
#include "ToneMappingActor.h"

namespace GameEngine
{
#define REGISTER_ACTOR_CLASS(name) engine->RegisterActorClass(#name, []() {return new name##Actor(); });
	void RegisterEngineActorClasses(Engine * engine)
	{
		REGISTER_ACTOR_CLASS(StaticMesh);
		REGISTER_ACTOR_CLASS(SkeletalMesh);
		REGISTER_ACTOR_CLASS(Camera);
		REGISTER_ACTOR_CLASS(FreeRoamCameraController);
		REGISTER_ACTOR_CLASS(ArcBallCameraController);
        REGISTER_ACTOR_CLASS(AnimationVisualization);
		REGISTER_ACTOR_CLASS(DirectionalLight);
		REGISTER_ACTOR_CLASS(PointLight);
		REGISTER_ACTOR_CLASS(Atmosphere);
		REGISTER_ACTOR_CLASS(Terrain);
		REGISTER_ACTOR_CLASS(ToneMapping);

	}
}