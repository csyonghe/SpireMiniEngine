#ifndef GX_GLTEXT_H
#define GX_GLTEXT_H

#include "CoreLib/Basic.h"
#include "CoreLib/LibUI/LibUI.h"
#include "CoreLib/Imaging/Bitmap.h"
#include "CoreLib/WinForm/WinTimer.h"
#include "CoreLib/MemoryPool.h"
#include "HardwareRenderer.h"
#include "AsyncCommandBuffer.h"
#include "EngineLimits.h"

namespace GameEngine
{
	class Font
	{
	public:
		CoreLib::String FontName;
		int Size;
		bool Bold, Underline, Italic, StrikeOut;
		Font()
		{
			NONCLIENTMETRICS NonClientMetrics;
			NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(NonClientMetrics.iPaddedBorderWidth);
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
			FontName = CoreLib::String::FromWString(NonClientMetrics.lfMessageFont.lfFaceName);
			Size = 9;
			Bold = false;
			Underline = false;
			Italic = false;
			StrikeOut = false;
		}
		Font(const CoreLib::String& sname, int ssize)
		{
			FontName = sname;
			Size = ssize;
			Bold = false;
			Underline = false;
			Italic = false;
			StrikeOut = false;
		}
		Font(const CoreLib::String & sname, int ssize, bool sBold, bool sItalic, bool sUnderline)
		{
			FontName = sname;
			Size = ssize;
			Bold = sBold;
			Underline = sUnderline;
			Italic = sItalic;
			StrikeOut = false;
		}
		CoreLib::String ToString() const
		{
			CoreLib::StringBuilder sb;
			sb << FontName << Size << Bold << Underline << Italic << StrikeOut;
			return sb.ProduceString();
		}
	};

	class DIBImage;
    class SystemWindow;

	struct TextSize
	{
		int x, y;
	};

	class TextRasterizationResult
	{
	public:
		TextSize Size;
		int BufferSize;
		unsigned char * ImageData;
	};

	class UIWindowsSystemInterface;

	class TextRasterizer
	{
	private:
		unsigned int TexID;
		DIBImage *Bit;
	public:
		TextRasterizer();
		~TextRasterizer();
		bool MultiLine = false;
		void SetFont(const Font & Font, int dpi);
		TextRasterizationResult RasterizeText(UIWindowsSystemInterface * system, const CoreLib::String & text, unsigned char * existingBuffer, int existingBufferSize);
		TextSize GetTextSize(const CoreLib::String & text);
		TextSize GetTextSize(const CoreLib::List<unsigned int> & text);

	};


	class BakedText : public GraphicsUI::IBakedText
	{
	public:
		UIWindowsSystemInterface* system;
		unsigned char * textBuffer;
		int BufferSize;
		int Width, Height;
		virtual int GetWidth() override
		{
			return Width;
		}
		virtual int GetHeight() override
		{
			return Height;
		}
		~BakedText();
	};


	class WindowsFont : public GraphicsUI::IFont
	{
	private:
		CoreLib::RefPtr<TextRasterizer> rasterizer;
		UIWindowsSystemInterface * system;
        HWND wndHandle;
		Font fontDesc;
	public:
		WindowsFont(UIWindowsSystemInterface * ctx, HWND wnd, int dpi, const Font & font)
		{
			system = ctx;
            wndHandle = wnd;
			fontDesc = font;
			rasterizer = new TextRasterizer();
			UpdateFontContext(dpi);
		}
		void UpdateFontContext(int dpi)
		{
			rasterizer->SetFont(fontDesc, dpi);
		}
        HWND GetWindowHandle()
        {
            return wndHandle;
        }
		virtual GraphicsUI::Rect MeasureString(const CoreLib::String & text) override;
		virtual GraphicsUI::Rect MeasureString(const CoreLib::List<unsigned int> & text) override;
		virtual GraphicsUI::IBakedText * BakeString(const CoreLib::String & text, GraphicsUI::IBakedText * previous) override;

	};

	class GLUIRenderer;

    class UIWindowContext : public GraphicsUI::UIWindowContext
    {
    private:
        void TickTimerTick(CoreLib::Object *, CoreLib::WinForm::EventArgs e);
        void HoverTimerTick(CoreLib::Object *, CoreLib::WinForm::EventArgs e);
    public:
        int screenWidth, screenHeight;
        int primitiveBufferSize, vertexBufferSize, indexBufferSize;
        SystemWindow * window;
        HardwareRenderer * hwRenderer;
        UIWindowsSystemInterface * sysInterface;
        CoreLib::RefPtr<WindowSurface> surface;
        CoreLib::RefPtr<Buffer> vertexBuffer, indexBuffer, primitiveBuffer;
        CoreLib::RefPtr<GraphicsUI::UIEntry> uiEntry;
        CoreLib::RefPtr<Texture2D> uiOverlayTexture;
        CoreLib::RefPtr<FrameBuffer> frameBuffer;
        CoreLib::RefPtr<Buffer> uniformBuffer;
        CoreLib::WinForm::Timer tmrHover, tmrTick;
        CoreLib::Array<CoreLib::RefPtr<DescriptorSet>, DynamicBufferLengthMultiplier> descSets;
        VectorMath::Matrix4 orthoMatrix;
        CoreLib::RefPtr<AsyncCommandBuffer> cmdBuffer, blitCmdBuffer;
        void SetSize(int w, int h);
        UIWindowContext();
        ~UIWindowContext();
    };
	class UIWindowsSystemInterface : public GraphicsUI::ISystemInterface
	{
	private:
        bool isWindows81OrGreater = false;
        HRESULT (WINAPI *getDpiForMonitor)(void* hmonitor, int dpiType, unsigned int *dpiX, unsigned int *dpiY);

    private:
		unsigned char * textBuffer = nullptr;
		CoreLib::Dictionary<CoreLib::String, CoreLib::RefPtr<WindowsFont>> fonts;
		CoreLib::RefPtr<Buffer> textBufferObj;
		CoreLib::MemoryPool textBufferPool;
		VectorMath::Vec4 ColorToVec(GraphicsUI::Color c);
		Fence* textBufferFence = nullptr;
		int GetCurrentDpi(HWND windowHandle);
	public:
		GLUIRenderer * uiRenderer;
		CoreLib::EnumerableDictionary<SystemWindow*, UIWindowContext*> windowContexts;
		HardwareRenderer * rendererApi = nullptr;
		virtual void SetClipboardText(const CoreLib::String & text) override;
		virtual CoreLib::String GetClipboardText() override;
		virtual GraphicsUI::IFont * LoadDefaultFont(GraphicsUI::UIWindowContext * ctx, GraphicsUI::DefaultFontType dt = GraphicsUI::DefaultFontType::Content) override;
		virtual void SwitchCursor(GraphicsUI::CursorType c) override;
		void UpdateCompositionWindowPos(HIMC hIMC, int x, int y);
	public:
		UIWindowsSystemInterface(HardwareRenderer * ctx);
		~UIWindowsSystemInterface();
		void WaitForDrawFence();
		unsigned char * AllocTextBuffer(int size);
		void FreeTextBuffer(unsigned char * buffer, int size)
		{
			textBufferPool.Free(buffer, size);
		}
		int GetTextBufferRelativeAddress(unsigned char * buffer)
		{
			return (int)(buffer - textBuffer);
		}
		Buffer * GetTextBufferObject()
		{
			return textBufferObj.Ptr();
		}
        GraphicsUI::IFont * LoadFont(UIWindowContext * ctx, const Font & f);
        GraphicsUI::IImage * CreateImageObject(const CoreLib::Imaging::Bitmap & bmp);
		void TransferDrawCommands(UIWindowContext * ctx, Texture2D* baseTexture, CoreLib::List<GraphicsUI::DrawCommand> & commands);
		void ExecuteDrawCommands(UIWindowContext * ctx, Fence* fence);
		int HandleSystemMessage(SystemWindow* window, UINT message, WPARAM &wParam, LPARAM &lParam);
        FrameBuffer * CreateFrameBuffer(Texture2D * texture);
        CoreLib::RefPtr<UIWindowContext> CreateWindowContext(SystemWindow* handle, int w, int h, int log2BufferSize);
        void UnregisterWindowContext(UIWindowContext * ctx);
	};
}

#endif