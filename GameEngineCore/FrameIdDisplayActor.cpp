#include "FrameIdDisplayActor.h"
#include "CoreLib/LibUI/LibUI.h"
#include "Engine.h"

using namespace GraphicsUI;

namespace GameEngine
{
    void FrameIdDisplayActor::showFrameId_changed()
    {
        if (label)
            label->Visible = ShowFrameID.GetValue();
        if (container)
            container->Visible = ShowFrameID.GetValue();
    }
    void FrameIdDisplayActor::RegisterUI(GraphicsUI::UIEntry * puiEntry)
    {
        uiEntry = puiEntry;
        container = new GraphicsUI::Container(uiEntry);
        label = new GraphicsUI::Label(container);
        container->Posit(emToPixel(1.0f), emToPixel(0.0f), emToPixel(15.0f), emToPixel(3.0f));
        container->BackColor.A = 160;
        label->Posit(emToPixel(0.5f), emToPixel(0.5f), emToPixel(10.0f), emToPixel(2.5f));
        label->SetFont(Engine::Instance()->LoadFont(Font("Segoe UI", 24)));
        label->Visible = ShowFrameID.GetValue();
        container->Visible = ShowFrameID.GetValue();
    }
    void FrameIdDisplayActor::OnUnload()
    {
        if (uiEntry)
            uiEntry->RemoveChild(container);
    }
    void FrameIdDisplayActor::OnLoad()
    {
        ShowFrameID.OnChanged.Bind(this, &FrameIdDisplayActor::showFrameId_changed);
    }
    void FrameIdDisplayActor::Tick()
    {
        label->FontColor = Color(255, 255, 255, 255);
        label->SetText(Engine::Instance()->GetFrameId());
        int boxH = label->GetHeight() + emToPixel(1.0f);
        int boxW = label->GetWidth() + emToPixel(1.0f);
        container->Posit((uiEntry->ClientRect().w - boxW) / 2, uiEntry->ClientRect().h - boxH - emToPixel(3.0f), boxW, boxH);
    }
}