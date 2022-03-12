#pragma once
#include "Platform/Runnable.h"
#include "Platform/SingleThreadRunnable.h"
#include "Platform/RunnableThread.h"
#include <functional>
#include "Engine/TaskGraphInterfaces.h"

class YRenderThreadRunnable :public FRunnable, public FSingleThreadRunnable
{
public:
	YRenderThreadRunnable();
	~YRenderThreadRunnable() override;

	virtual unsigned int Run() override;


	virtual bool Init() override;


	virtual void Stop() override;


	virtual void Exit() override;


	virtual class FSingleThreadRunnable* GetSingleThreadInterface() override;


	virtual void Tick() override;

};

class YRenderThread
{
public:
	YRenderThread();
	~YRenderThread();
	bool CreateThread();
private:
	YRenderThreadRunnable* render_thread_runnable_ = nullptr;
	FRunnableThread* thread_ = nullptr;
};

struct RenderThreadTask
{
public:
	RenderThreadTask(std::function<void()>&& func) :func_(std::move(func)) {}
	~RenderThreadTask() {}
	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::ActualRenderingThread;
		//return ENamedThreads::AnyThread;
	}
	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::TrackSubsequents;
	}
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		//::Sleep(30);
		func_();
	}
	std::function<void()> func_;
};
