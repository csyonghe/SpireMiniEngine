#include "SystemWindow.h"
#include "Engine.h"
#include "CoreLib/WinForm/WinApp.h"

namespace GameEngine
{
    using namespace GraphicsUI;
    using namespace CoreLib;
    using namespace VectorMath;

    SystemWindow::SystemWindow(UIWindowsSystemInterface * psysInterface, int log2UIBufferSize)
    {
        Create();
        this->wantChars = true;
        this->uiContext = psysInterface->CreateWindowContext(GetHandle(), GetClientWidth(), GetClientHeight(), log2UIBufferSize);
        OnResized.Bind(this, &SystemWindow::WindowResized);
        OnResizing.Bind(this, &SystemWindow::WindowResizing);

    }

    SystemWindow::~SystemWindow()
    {
        CoreLib::WinForm::Application::UnRegisterComponent(this);
    }
   
    void SystemWindow::Create()
    {
        handle = CreateWindowW(CoreLib::WinForm::Application::GLFormClassName, 0, WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, CoreLib::WinForm::Application::GetHandle(), NULL);
        if (!handle)
        {
            throw "Failed to create window.";
        }
        CoreLib::WinForm::Application::RegisterComponent(this);
        SubClass();
    }

    void SystemWindow::WindowResized(CoreLib::Object * /*sender*/, CoreLib::WinForm::EventArgs /*e*/)
    {
        uiContext->SetSize(GetClientWidth(), GetClientHeight());
    }

    void SystemWindow::WindowResizing(CoreLib::Object * /*sender*/, CoreLib::WinForm::ResizingEventArgs &/* e*/)
    {
        uiContext->SetSize(GetClientWidth(), GetClientHeight());

        Engine::Instance()->RefreshUI();
    }

    int SystemWindow::ProcessMessage(CoreLib::WinForm::WinMessage & msg)
    {
        auto engine = Engine::Instance();
        int rs = -1;
        if (engine)
        {
            rs = engine->HandleWindowsMessage(msg.hWnd, msg.message, msg.wParam, msg.lParam);
        }
        if (rs == -1)
            return BaseForm::ProcessMessage(msg);
        return rs;
    }

}