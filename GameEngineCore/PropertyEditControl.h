#ifndef GAME_ENGINE_PROPERTY_EDIT_CONTROL_H
#define GAME_ENGINE_PROPERTY_EDIT_CONTROL_H

namespace GameEngine
{
	using namespace GraphicsUI;

	class PropertyEdit : public Container
	{
	private:
		Property * targetProperty;
		Label * lblName;
		TextBox * txtValue;
	public:
		PropertyEdit(Container * owner, Property * pTargetProperty)
			: Container(owner), targetProperty(pTargetProperty)
		{
			lblName = new Label(this);
			lblName->Posit(0, 0, 200, 30);
			lblName->SetText(targetProperty->GetName());
			txtValue = new TextBox(this);
			txtValue->Posit(0, EM(1.1f), EM(10.0f), EM(1.1f));
			txtValue->SetText(targetProperty->GetStringValue());
			SetHeight(EM(2.5f));
			txtValue->OnKeyPress.Bind(this, &PropertyEdit::txtValue_KeyPressed);
		}
		void txtValue_KeyPressed(UI_Base *, UIKeyEventArgs & e)
		{
			if (e.Key == VK_RETURN && e.Shift == 0)
			{
				try
				{
					targetProperty->SetStringValue(txtValue->GetText());
				}
				catch (Exception)
				{
				}
				txtValue->SetText(targetProperty->GetStringValue());
			}
		}
	};
}

#endif
