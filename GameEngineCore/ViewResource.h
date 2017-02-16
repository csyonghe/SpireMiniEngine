#ifndef GAME_ENGINE_VIEW_RESOURCE_H
#define GAME_ENGINE_VIEW_RESOURCE_H

#include "HardwareRenderer.h"
#include "CoreLib/Events.h"

namespace GameEngine
{
	class RenderTarget : public CoreLib::RefObject
	{
	public:
		GameEngine::StorageFormat Format;
		CoreLib::RefPtr<Texture2D> Texture;
		CoreLib::RefPtr<Texture2DArray> TextureArray;
		int Layer = 0;
		float ResolutionScale = 1.0f;
		bool UseFixedResolution = false;
		int FixedWidth = 1024, FixedHeight = 1024;
		int Width = 0, Height = 0;
		RenderTarget() = default;
		RenderTarget(GameEngine::StorageFormat format, CoreLib::RefPtr<Texture2DArray> texArray, int layer, int w, int h);
	};

	class RenderOutput : public CoreLib::RefObject
	{
		friend class ViewResource;
	private:
		CoreLib::RefPtr<FrameBuffer> frameBuffer;
		CoreLib::List<CoreLib::RefPtr<RenderTarget>> bindings;
		RenderTargetLayout * renderTargetLayout = nullptr;
		RenderOutput() {}
	public:
		void GetSize(int &w, int &h)
		{
			w = bindings.First()->Width;
			h = bindings.First()->Height;
		}
		FrameBuffer * GetFrameBuffer()
		{
			return frameBuffer.Ptr();
		}
	};

	class ViewResource : public CoreLib::RefObject
	{
	private:
		HardwareRenderer * hwRenderer;
		int screenWidth = 0, screenHeight = 0;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<RenderTarget>> renderTargets;
		CoreLib::List<CoreLib::RefPtr<RenderOutput>> renderOutputs;
		void UpdateRenderResultFrameBuffer(RenderOutput * output);
	public:
		CoreLib::Event<> Resized;
		CoreLib::RefPtr<RenderTarget> LoadSharedRenderTarget(CoreLib::String name, StorageFormat format, float ratio = 1.0f, int w0 = 1024, int h0 = 1024);
		template<typename ...TRenderTargets>
		RenderOutput * CreateRenderOutput(RenderTargetLayout * renderTargetLayout, TRenderTargets ... renderTargets)
		{
			RefPtr<RenderOutput> result = new RenderOutput();
			typename FirstType<TRenderTargets...>::type targets[] = { renderTargets... };
			for (auto & target : targets)
			{
				result->bindings.Add(target);
			}
			result->renderTargetLayout = renderTargetLayout;
			renderOutputs.Add(result);
			UpdateRenderResultFrameBuffer(result.Ptr());
			return result.Ptr();
		}
		void DestroyRenderOutput(RenderOutput * output)
		{
			int id = renderOutputs.IndexOf(output);
			if (id != -1)
			{
				renderOutputs[id] = nullptr;
				renderOutputs.FastRemoveAt(id);
			}
		}
		void Resize(int w, int h);
		ViewResource(HardwareRenderer * hw)
		{
			hwRenderer = hw;
		}
		int GetWidth()
		{
			return screenWidth;
		}
		int GetHeight()
		{
			return screenHeight;
		}
	};
}

#endif