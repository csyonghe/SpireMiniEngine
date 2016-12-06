#include "Engine.h"
#include "SkeletalMeshActor.h"
#include "StaticMeshActor.h"
#include "CameraActor.h"
#include "FreeRoamCameraController.h"
#include "MotionGraphMeshActor.h"
#include "AnimationVisualizationActor.h"

namespace GameEngine
{
#define REGISTER_ACTOR_CLASS(name) engine->RegisterActorClass(#name, []() {return new name##Actor(); });
	void RegisterEngineActorClasses(Engine * engine)
	{
		REGISTER_ACTOR_CLASS(StaticMesh);
		REGISTER_ACTOR_CLASS(SkeletalMesh);
		REGISTER_ACTOR_CLASS(Camera);
		REGISTER_ACTOR_CLASS(FreeRoamCameraController);
        REGISTER_ACTOR_CLASS(MotionGraphMesh);
        REGISTER_ACTOR_CLASS(AnimationVisualization);
	}
}