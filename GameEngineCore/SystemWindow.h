#ifndef GAME_ENGINE_SYSTEM_WINDOW_H
#define GAME_ENGINE_SYSTEM_WINDOW_H

#include "CoreLib/WinForm/WinForm.h"
#include "CoreLib/LibUI/LibUI.h"
#include "HardwareRenderer.h"
#include "UISystem_Windows.h"

namespace GameEngine
{
    class SystemWindow : public CoreLib::WinForm::BaseForm
    {
    private:
        CoreLib::RefPtr<UIWindowContext> uiContext;
    protected:
        virtual void Create() override;
        void WindowResized(CoreLib::Object* sender, CoreLib::WinForm::EventArgs e);
        void WindowResizing(CoreLib::Object* sender, CoreLib::WinForm::ResizingEventArgs & e);

    protected:
        int ProcessMessage(CoreLib::WinForm::WinMessage & msg) override;
    public:
        SystemWindow(UIWindowsSystemInterface * sysInterface, int log2UIBufferSize);
        ~SystemWindow();
        GraphicsUI::UIEntry * GetUIEntry()
        {
            return uiContext->uiEntry.Ptr();
        }
        UIWindowContext * GetUIContext()
        {
            return uiContext.Ptr();
        }
    };
}

#endif