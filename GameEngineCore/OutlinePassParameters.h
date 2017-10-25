#ifndef GAME_ENGINE_OUTLINE_PARAMS_H
#define GAME_ENGINE_OUTLINE_PARAMS_H

#include "CoreLib/VectorMath.h"
#include "CoreLib/Basic.h"

namespace GameEngine
{
    struct OutlinePassParameters
    {
        VectorMath::Vec4 color = VectorMath::Vec4::Create(0xFF/255.0f, 0xC1 / 255.0f, 0x07 / 255.0f, 1.0f);
        VectorMath::Vec2 pixelSize = VectorMath::Vec2::Create(0.0f, 0.0f);
        float Width = 5.0f;
        OutlinePassParameters()
        {
        }
        bool operator == (const OutlinePassParameters & other) const
        {
            return Width == other.Width && pixelSize.x == other.pixelSize.x && pixelSize.y == other.pixelSize.y;
        }
    };
}

#endif