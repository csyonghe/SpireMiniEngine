#include "InputDispatcher.h"
#include "CoreLib/Tokenizer.h"
#include "CoreLib/LibIO.h"

namespace GameEngine
{
	using namespace CoreLib;
	using namespace CoreLib::Text;
	using namespace CoreLib::IO;

	InputDispatcher::InputDispatcher(const RefPtr<HardwareInputInterface>& pInputInterface)
	{
		inputInterface = pInputInterface;
	}

	void InputDispatcher::LoadMapping(const CoreLib::String & fileName)
	{
		TokenReader parser(File::ReadAllText(fileName));
		while (!parser.IsEnd())
		{
			bool isAxis = true;
			InputMappingValue mapVal;
			if (parser.LookAhead("action"))
			{
				parser.Read("action");
				isAxis = false;
			}
			else
				parser.Read("axis");
			mapVal.ActionName = parser.ReadWord();
			parser.Read(":");
			auto ch = parser.ReadStringLiteral().ToUpper();
			if (isAxis)
			{
				if (parser.LookAhead(","))
				{
					parser.Read(",");
					mapVal.Value = (float)parser.ReadDouble();
				}
			}
			wchar_t key = ch[0];
			if (ch == "SPACE")
				key = SpecialKeys::Space;
			else if (ch == "MBLEFT")
				key = SpecialKeys::MouseLeftButton;
			else if (ch == "MBRIGHT")
				key = SpecialKeys::MouseRightButton;
			else if (ch == "MBMIDDLE")
				key = SpecialKeys::MouseMiddleButton;
			else if (ch == "CTRL")
				key = SpecialKeys::Control;
			else if (ch == "UP")
				key = SpecialKeys::UpArrow;
			else if (ch == "DOWN")
				key = SpecialKeys::DownArrow;
			else if (ch == "LEFT")
				key = SpecialKeys::LeftArrow;
			else if (ch == "RIGHT")
				key = SpecialKeys::RightArrow;
			else if (ch == "ESCAPE")
				key = SpecialKeys::Escape;
			else if (ch == "ENTER")
				key = SpecialKeys::Enter;
			else if (ch == "SHIFT")
				key = SpecialKeys::Shift;
			else if (ch == "`")
				key = SpecialKeys::Tilda;
			if (isAxis)
				keyboardAxisMappings[key] = mapVal;
			else
				keyboardActionMappings[key] = mapVal;
			actionHandlers[mapVal.ActionName] = List<ActionInputHandlerFunc>();
		}
	}
	void InputDispatcher::BindActionHandler(const CoreLib::String & axisName, ActionInputHandlerFunc handlerFunc)
	{
		auto list = actionHandlers.TryGetValue(axisName);
		if (list)
			list->Add(handlerFunc);
	}
	void InputDispatcher::UnbindActionHandler(const CoreLib::String & axisName, ActionInputHandlerFunc func)
	{
		auto list = actionHandlers.TryGetValue(axisName);
		if (list)
		{
			for (int i = list->Count() - 1; i >= 0; i--)
			{
				if ((*list)[i] == func)
				{
					list->RemoveAt(i);
					break;
				}
			}
		}
	}
	void InputDispatcher::BindMouseInputHandler(ActorMouseInputHandlerFunc handlerFunc)
	{
		mouseActionHandlers.Add(handlerFunc);
	}
	void InputDispatcher::UnbindMouseInputHandler(ActorMouseInputHandlerFunc handlerFunc)
	{
		for (int i = mouseActionHandlers.Count() - 1; i >= 0; i--)
		{
			if (mouseActionHandlers[i] == handlerFunc)
			{
				mouseActionHandlers.RemoveAt(i);
				break;
			}
		}
	}
	void InputDispatcher::DispatchInput()
	{
		for (auto & binding : keyboardActionMappings)
		{
			if (inputInterface->QueryKeyState(binding.Key).HasPressed)
			{
				DispatchAction(binding.Value.ActionName, binding.Value.Value);
			}
		}

		for (auto & binding : keyboardAxisMappings)
		{
			if (inputInterface->QueryKeyState(binding.Key).IsDown)
			{
				DispatchAction(binding.Value.ActionName, binding.Value.Value);
			}
		}
	}
	void InputDispatcher::DispatchAction(CoreLib::String actionName, float actionValue)
	{
		auto handlers = actionHandlers.TryGetValue(actionName);
		if (handlers)
		{
			for (auto & handler : *handlers)
				if (handler(actionName, actionValue))
					break;
		}
	}
}
