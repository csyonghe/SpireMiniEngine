#ifndef ANIMATION_VISUALIZATION_ACTOR_H
#define ANIMATION_VISUALIZATION_ACTOR_H

#include "Actor.h"
#include "CoreLib/LibUI/LibUI.h"
#include "Engine.h"
#include "MotionGraphAnimationSynthesizer.h"

namespace GameEngine
{
    class AnimationVisualizationActor : public Actor
    {
    private:
        Pose nextPose;
        RefPtr<Drawable> drawable;
        GraphicsUI::Form * uiForm = nullptr;
        int frameId = 0;

    protected:
        virtual bool ParseField(CoreLib::String fieldName, CoreLib::Text::TokenReader & parser) override;
    
    public:
        Mesh * Mesh = nullptr;
        Skeleton * Skeleton = nullptr;
        MotionGraph* Graph = nullptr;

        CoreLib::String MeshName, SkeletonName, MotionGraphName;
        Material * MaterialInstance = nullptr;
        virtual void Tick() override;
        
        Pose & GetCurrentPose()
        {
            return nextPose;
        }

        virtual void GetDrawables(const GetDrawablesParameter & params) override;
        virtual EngineActorType GetEngineType() override
        {
            return EngineActorType::Drawable;
        }
        virtual CoreLib::String GetTypeName() override
        {
            return "AnimationVisualization";
        }
        virtual void OnLoad() override;

        virtual void RegisterUI(GraphicsUI::UIEntry * entry) override
        {
            uiForm = new GraphicsUI::Form(entry);
            uiForm->SetText(String("Playback Control - ") + Name.GetValue());
            uiForm->Posit(EM(10.0f), EM(1.0f), EM(20.0f), EM(6.0f));

            auto label = new GraphicsUI::Label(uiForm);
            label->Posit(EM(1.0f), EM(1.0f), EM(5.0f), EM(1.0f));
            label->SetText("Frame: 0");

            auto lblContact = new GraphicsUI::Label(uiForm);
            lblContact->SetText("[]");
            lblContact->Posit(EM(15.0f), EM(1.0f), EM(5.0f), EM(1.0f));

            auto slider = new GraphicsUI::ScrollBar(uiForm);
            slider->Posit(EM(1.0f), EM(2.2f), EM(18.0f), EM(1.0f));
            slider->SetValue(0, Graph->States.Count()-1, 0, 1);
            slider->OnChanged.Bind([=](GraphicsUI::UI_Base*)
            {
                frameId = slider->GetPosition();
                label->SetText(String("Frame: ") + frameId + "/" + Graph->States.Count() + "    seq: " + Graph->States[frameId].Sequence 
                + "[" + Graph->States[frameId].IdInsequence + "]");
                switch (Graph->States[frameId].Contact)
                {
                case ContactLabel::BothFeetOnFloor:
                    lblContact->SetText("[Both]");
                    break;
                case ContactLabel::LeftFootOnFloor:
                    lblContact->SetText("[Left]");
                    break;
                case ContactLabel::RightFootOnFloor:
                    lblContact->SetText("[Right]");
                    break;
                default:
                    lblContact->SetText("[InAir]");
                    break;
                }
            });
            entry->ShowWindow(uiForm);
        }
    };
}


#endif