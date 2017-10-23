#ifndef GAME_ENGINE_PROPERTY_EDIT_CONTROL_H
#define GAME_ENGINE_PROPERTY_EDIT_CONTROL_H


namespace GameEngine
{
	using namespace GraphicsUI;
	using namespace CoreLib::IO;

	class PropertyEdit : public Container
	{
	protected:
		Property * targetProperty;
	public:
		Event<Property *> OnPropertyChanged;
		PropertyEdit(Container * owner, Property * pTargetProperty)
			: Container(owner), targetProperty(pTargetProperty)
		{
		}
		virtual void Update() = 0;
	};

	class GeneralPropertyEdit : public PropertyEdit
	{
	private:
		Label * lblName;
		TextBox * txtValue;
	public:
		GeneralPropertyEdit(Container * owner, Property * pTargetProperty)
			: PropertyEdit(owner, pTargetProperty)
		{
			lblName = new Label(this);
			lblName->Posit(0, 0, 200, 30);
			lblName->SetText(targetProperty->GetName());
			txtValue = new TextBox(this);
			txtValue->Posit(0, EM(1.1f), EM(10.0f), EM(1.1f));
			txtValue->SetText(targetProperty->GetStringValue());
			SetHeight(EM(3.2f));
			txtValue->OnKeyPress.Bind(this, &GeneralPropertyEdit::txtValue_KeyPressed);
		}
		virtual void Update() override
		{
			txtValue->SetText(targetProperty->GetStringValue());
		}
		void txtValue_KeyPressed(UI_Base *, UIKeyEventArgs & e)
		{
			if (e.Key == VK_RETURN && e.Shift == 0)
			{
				try
				{
					Engine::Instance()->GetRenderer()->Wait();
					targetProperty->SetStringValue(txtValue->GetText());
					OnPropertyChanged(targetProperty);
				}
				catch (Exception)
				{
				}
				txtValue->SetText(targetProperty->GetStringValue());
			}
		}
	}; 
	
	class BoolPropertyEdit : public PropertyEdit
	{
	private:
		CheckBox * chkValue;
	public:
		BoolPropertyEdit(Container * owner, Property * pTargetProperty)
			: PropertyEdit(owner, pTargetProperty)
		{
			chkValue = new CheckBox(this);
			chkValue->Posit(0, 0, 200, 30);
			chkValue->SetText(targetProperty->GetName());
			SetHeight(EM(2.0f));
			Update();
			chkValue->OnChanged.Bind(this, &BoolPropertyEdit::chkValue_Changed);
		}
		virtual void Update() override
		{
			chkValue->Checked = targetProperty->GetStringValue() == "true";
		}
		void chkValue_Changed(UI_Base *)
		{
			try
			{
				Engine::Instance()->GetRenderer()->Wait();
				targetProperty->SetStringValue(chkValue->Checked? "true":"false");
				OnPropertyChanged(targetProperty);
			}
			catch (Exception)
			{
			}
			chkValue->Checked = targetProperty->GetStringValue() == "true";
		}
	};

	class ResourcePropertyEdit : public PropertyEdit
	{
	private:
		ResourceType resType = ResourceType::Level;
		String ext;
		CoreLib::WinForm::FileDialog openDlg;
		Label * lblName;
		TextBox * txtValue;
		Button * btnBrowse;
		GenericProperty<CoreLib::String> * strProperty;
	public:
		ResourcePropertyEdit(Container * owner, Property * pTargetProperty, String attrib, SystemWindow * window)
			: PropertyEdit(owner, pTargetProperty), openDlg(window)
		{
			strProperty = (GenericProperty<CoreLib::String>*)pTargetProperty;
			CoreLib::Text::TokenReader parser(attrib);
			parser.Read("resource");
			parser.Read("(");
			auto resourceType = parser.ReadWord();
			parser.Read(",");
			ext = parser.ReadWord();
			parser.Read(")");
			if (resourceType == "Mesh" || resourceType == "Model")
				resType = ResourceType::Mesh;
			else if (resourceType == "Material")
				resType = ResourceType::Material;
			else if (resourceType == "Animation")
				resType = ResourceType::Animation;
			else if (resourceType == "Shader")
				resType = ResourceType::Shader;
			else if (resourceType == "Texture")
				resType = ResourceType::Texture;
			else if (resourceType == "Landscape")
				resType = ResourceType::Landscape; 

			openDlg.Filter = resourceType + "|*." + ext;
			openDlg.DefaultEXT = ext;
			openDlg.FileMustExist = true;
			openDlg.FileName = Engine::Instance()->GetDirectory(false, resType) + CoreLib::IO::Path::PathDelimiter;
			lblName = new Label(this);
			lblName->Posit(0, 0, 200, 30);
			lblName->SetText(targetProperty->GetName());
			txtValue = new TextBox(this);
			txtValue->Posit(0, EM(1.1f), EM(10.0f), EM(1.1f));
			txtValue->SetText(strProperty->GetValue());
			txtValue->OnKeyPress.Bind(this, &ResourcePropertyEdit::txtValue_KeyPressed);
			btnBrowse = new Button(this);
			btnBrowse->SetText("Browse");
			btnBrowse->Posit(0, EM(2.3f), EM(5.0f), EM(1.2f));
			btnBrowse->OnClick.Bind(this, &ResourcePropertyEdit::btnBrowse_Clicked);
			SetHeight(EM(4.5f));
		}
		void SetPropertyValue(String value)
		{
			try
			{
				Engine::Instance()->GetRenderer()->Wait();
				strProperty->SetValue(value);
				OnPropertyChanged(strProperty);
			}
			catch (Exception)
			{
			}
			txtValue->SetText(strProperty->GetValue());
		}
		String GetRelativePath(ResourceType type, String path)
		{
			auto engineDir = Path::Normalize(Engine::Instance()->GetDirectory(true, type));
			auto gameDir = Path::Normalize(Engine::Instance()->GetDirectory(false, type));
			path = Path::Normalize(path);
			if (Path::IsSubPathOf(path, engineDir))
				return Path::GetRelativePath(path, engineDir);
			else
				return Path::GetRelativePath(path, gameDir);
		}
		void btnBrowse_Clicked(UI_Base *)
		{
			if (openDlg.ShowOpen())
			{
				auto newPath = GetRelativePath(resType, openDlg.FileName);
				SetPropertyValue(newPath);
			}
		}
		void txtValue_KeyPressed(UI_Base *, UIKeyEventArgs & e)
		{
			if (e.Key == VK_RETURN && e.Shift == 0)
			{
				SetPropertyValue(txtValue->GetText());
			}
		}
		virtual void Update() override
		{
			txtValue->SetText(strProperty->GetValue());
		}
	};

	PropertyEdit * CreatePropertyEdit(Container * owner, Property * pTargetProperty, SystemWindow * mainWindow)
	{
		String attrib = pTargetProperty->GetAttribute();
		if (strcmp(pTargetProperty->GetTypeName(), "bool") == 0)
			return new BoolPropertyEdit(owner, pTargetProperty);
		else if (attrib.StartsWith("resource"))
			return new ResourcePropertyEdit(owner, pTargetProperty, attrib, mainWindow);
		else
			return new GeneralPropertyEdit(owner, pTargetProperty);
	}
}

#endif
