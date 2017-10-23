#include "WinCommonDlg.h"
#include "../LibIO.h"
#include "../LibMath.h"

using namespace CoreLib::IO;

namespace CoreLib
{
	namespace WinForm
	{
		FileDialog::FileDialog(const Component * _owner)
		{
			owner = const_cast<Component*>(_owner);
			memset(&fn, 0, sizeof(OPENFILENAME));
			memset(fileBuf,0,sizeof(fileBuf));
			memset(filterBuf,0,sizeof(filterBuf));
			fn.lStructSize = sizeof(OPENFILENAME);
			fn.hInstance = 0;
			fn.Flags = OFN_EXPLORER|OFN_ENABLESIZING;
			fn.hwndOwner = owner->GetHandle();
			fn.lpstrCustomFilter = 0;
			fn.lpfnHook = nullptr;
			fn.nMaxCustFilter = 0;
			fn.lpstrFile = fileBuf;
			fn.nMaxFile = FileBufferSize;
			MultiSelect = false;
			CreatePrompt = true;
			FileMustExist = false;
			HideReadOnly = false;
			OverwritePrompt = true;
			PathMustExist = false;
		}

		FileDialog::~FileDialog()
		{

		}

		void FileDialog::PrepareDialog()
		{	
			memset(fileBuf,0,sizeof(fileBuf));
			fn.nFileOffset = 0;
			if (Filter.Length())
			{
				memset(filterBuf,0,sizeof(wchar_t)*FilterBufferSize);
				if (Filter.Length()<FilterBufferSize)
				{
					for (int i=0; i<Filter.Length(); i++)
					{
						if (Filter[i]=='|')
							filterBuf[i] = '\0';
						else
							filterBuf[i] = Filter[i];
					}
				}
				else
					throw "Filter too long.";
				fn.lpstrFilter = filterBuf;
			}
			if (DefaultEXT.Length())
			{
				fn.lpstrDefExt = DefaultEXT.ToWString();
			}
			else
				fn.lpstrDefExt = 0;
			initDir = Path::GetDirectoryName(FileName);
			fn.lpstrInitialDir = initDir.ToWString();
			
			fn.Flags = OFN_EXPLORER | OFN_ENABLESIZING;
			if (MultiSelect)
				fn.Flags = (fn.Flags | OFN_ALLOWMULTISELECT);
			if (CreatePrompt)
				fn.Flags = (fn.Flags | OFN_CREATEPROMPT);
			if (FileMustExist)
				fn.Flags = (fn.Flags | OFN_FILEMUSTEXIST);
			if (HideReadOnly)
				fn.Flags = (fn.Flags | OFN_HIDEREADONLY);
			if (OverwritePrompt)
				fn.Flags = (fn.Flags | OFN_OVERWRITEPROMPT);
			if (PathMustExist)
				fn.Flags = (fn.Flags | OFN_PATHMUSTEXIST);
		}

		bool FileDialog::ShowOpen()
		{
			PrepareDialog();
			bool succ = (GetOpenFileName(&fn)!=0);
			PostDialogShow();
			return succ;
		}

		bool FileDialog::ShowSave()
		{
			PrepareDialog();
			bool succ = GetSaveFileName(&fn)!=0;
			PostDialogShow();
			return succ;
		}

		void FileDialog::PostDialogShow()
		{
			StringBuilder cFileName;
			bool zero = false;
			FileNames.Clear();
			int i = 0;
			char * strPtr = (char*)fn.lpstrFile;
			while (i < FileBufferSize * sizeof(wchar_t))
			{
				int codePoint = CoreLib::IO::GetUnicodePointFromUTF16([&](int)
				{
					return strPtr[i++];
				});
				if (codePoint != 0)
				{
					char buf[5];
					int len = CoreLib::IO::EncodeUnicodePointToUTF8(buf, codePoint);
					cFileName.Append(buf, len);
					zero = false;
				}
				else
				{
					if (!zero)
					{
						FileNames.Add(cFileName.ToString());
						cFileName.Clear();
						zero = true;
					}
					else
						break;
				}
			}
			if (FileNames.Count())
				FileName = FileNames[0];
			if (FileNames.Count() > 1)
				FileNames = From(FileNames).Skip(1).Select([&](String x) {return Path::Combine(FileNames[0], x); }).ToList();
		}

		UINT_PTR CALLBACK ColorHookProc(HWND hdlg, UINT uiMsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
		{
			if (uiMsg == WM_INITDIALOG)
			{
				RECT r, rs;
				HWND handle = hdlg;
				HWND hp = GetParent(handle);
				GetWindowRect(hp , &r);
				GetWindowRect(hdlg, &rs);
				int w = rs.right - rs.left;
				int h = rs.bottom - rs.top;
				MoveWindow(handle, (r.left + r.right-w)/2, (r.top+r.bottom-h)/2, w,h, TRUE);
				return 1;
			}
			return 0;
		}

		ColorDialog::ColorDialog(Component * _owner)
		{
			owner = _owner;
			FullOpen = true;
			PreventFullOpen = false;
			memset(&cs, 0, sizeof(CHOOSECOLOR));
			cs.hInstance = 0;
			cs.lpfnHook = ColorHookProc;
			cs.hwndOwner = owner->GetHandle();
			cs.lpCustColors = cr;
			cs.lStructSize = sizeof(CHOOSECOLOR);
		}

		bool ColorDialog::ShowColor()
		{
			cs.Flags = CC_ENABLEHOOK|CC_RGBINIT|CC_ANYCOLOR;
			if (FullOpen)
				cs.Flags = (cs.Flags | CC_FULLOPEN);
			if (PreventFullOpen)
				cs.Flags = (cs.Flags | CC_PREVENTFULLOPEN);
			bool succ = (ChooseColor(&cs)!=0);
			Color = cs.rgbResult;
			return succ;
		}
	}
}