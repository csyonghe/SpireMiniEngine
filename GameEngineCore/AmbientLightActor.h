#ifndef GAME_ENGINE_AMBIENT_LIGHT_ACTOR
#define GAME_ENGINE_AMBIENT_LIGHT_ACTOR

#include "LightActor.h"
#include "CoreLib/VectorMath.h"

namespace GameEngine
{
    class AmbientLightActor : public LightActor
    {
    protected:
        virtual Mesh CreateGizmoMesh() override;
    public:
        PROPERTY(VectorMath::Vec3, Ambient);
        virtual CoreLib::String GetTypeName() override
        {
            return "AmbientLight";
        }
        AmbientLightActor()
        {
            lightType = LightType::Ambient;
        }

    };
}

#endif