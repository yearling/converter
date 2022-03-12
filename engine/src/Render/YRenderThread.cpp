#include "Render/YRenderThread.h"
#include "Engine/TaskGraphInterfaces.h"

YRenderThreadRunnable::YRenderThreadRunnable()
{

}

YRenderThreadRunnable::~YRenderThreadRunnable()
{

}

unsigned int YRenderThreadRunnable::Run()
{
	FTaskGraphInterface::Get().ProcessThreadUntilRequestReturn(ENamedThreads::ActualRenderingThread);
	return 0;
}

bool YRenderThreadRunnable::Init()
{
	FTaskGraphInterface::Get().AttachToThread(ENamedThreads::ActualRenderingThread);
	return true;
}

void YRenderThreadRunnable::Stop()
{
}

void YRenderThreadRunnable::Exit()
{
}

class FSingleThreadRunnable* YRenderThreadRunnable::GetSingleThreadInterface()
{
	return this;
}

void YRenderThreadRunnable::Tick()
{
}

YRenderThread::YRenderThread()
{

}

YRenderThread::~YRenderThread()
{

}

bool YRenderThread::CreateThread()
{
	render_thread_runnable_ = new YRenderThreadRunnable();
	thread_ = FRunnableThread::Create(render_thread_runnable_, "RenderThread", 1024 * 512);
	return !!thread_;
}
