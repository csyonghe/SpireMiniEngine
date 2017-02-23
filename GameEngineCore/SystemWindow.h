#ifndef GAME_ENGINE_SYSTEM_WINDOW_H
#define GAME_ENGINE_SYSTEM_WINDOW_H

#include "CoreLib/WinForm/WinForm.h"
#include "LibUI/LibUI.h"
#include "HardwareRenderer.h"

namespace GameEngine
{
    class SystemWindow : public CoreLib::WinForm::Form
    {
    private:
        CoreLib::RefPtr<GraphicsUI::UIEntry> uiEntry;
        CoreLib::RefPtr<WindowSurface> surface;
    };
}

#endif