#include "SSAO.h"

namespace GameEngine
{
	class SSAOImpl
	{
	public:
		void Init(RendererSharedResource * /*sharedRes*/, const char * /*depthSource*/)
		{

		}
		void Resize(int /*w*/, int /*h*/)
		{
		}
		void RegisterWork(FrameRenderTask & /*task*/)
		{
		}
		char * GetResultName()
		{
			return "";
		}
	};

	SSAO::SSAO()
	{
		impl = new SSAOImpl();
	}

	SSAO::~SSAO()
	{
		delete impl;
	}

	void SSAO::Init(RendererSharedResource * sharedRes, const char * depthSource)
	{
		impl->Init(sharedRes, depthSource);
	}

	void SSAO::Resize(int w, int h)
	{
		impl->Resize(w, h);
	}

	void SSAO::RegisterWork(FrameRenderTask & task)
	{
		impl->RegisterWork(task);
	}

	char * SSAO::GetResultName()
	{
		return impl->GetResultName();
	}

}

