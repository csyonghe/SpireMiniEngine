#ifndef GAME_ENGINE_FRAMEID_DISPLAY_ACTOR_H
#define GAME_ENGINE_FRAMEID_DISPLAY_ACTOR_H

#include "Actor.h"

namespace GraphicsUI
{
    class Label;
    class Container;
}

namespace GameEngine
{
    class FrameIdDisplayActor : public Actor
    {
    private:
        GraphicsUI::Label * label = nullptr;
        GraphicsUI::Container * container = nullptr;
        GraphicsUI::UIEntry * uiEntry = nullptr;
        void showFrameId_changed();
    public:
        PROPERTY_DEF(bool, ShowFrameID, true);
        virtual void RegisterUI(GraphicsUI::UIEntry *) override;
        virtual void OnUnload() override;
        virtual void OnLoad() override;
        virtual void Tick() override;
        virtual CoreLib::String GetTypeName() override
        {
            return "FrameIdDisplay";
        }
        virtual EngineActorType GetEngineType() override
        {
            return EngineActorType::Util;
        }
    };
}

#endif