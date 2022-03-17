#pragma once
#include "Platform/Runnable.h"
#include "Platform/SingleThreadRunnable.h"
#include "Platform/RunnableThread.h"
#include <functional>
#include "Engine/TaskGraphInterfaces.h"
class FRHICommandListImmediate;

////////////////////////////////////
// Render thread API
////////////////////////////////////

/**
 * Whether the renderer is currently running in a separate thread.
 * If this is false, then all rendering commands will be executed immediately instead of being enqueued in the rendering command buffer.
 */
extern  bool GIsThreadedRendering;

/**
 * Whether the rendering thread should be created or not.
 * Currently set by command line parameter and by the ToggleRenderingThread console command.
 */
extern  bool GUseThreadedRendering;

extern  void SetRHIThreadEnabled(bool bEnableDedicatedThread, bool bEnableRHIOnTaskThreads);

/** Starts the rendering thread. */
extern  void StartRenderingThread();

/** Stops the rendering thread. */
extern  void StopRenderingThread();

/**
 * Adds a task that must be completed either before the next scene draw or a flush rendering commands
 * This can be called from any thread though it probably doesn't make sense to call it from the render thread.
 * @param TaskToAdd, task to add as a pending render thread task
 */
extern  void AddFrameRenderPrerequisite(const FGraphEventRef& TaskToAdd);

/**
 * Gather the frame render prerequisites and make sure all render commands are at least queued
 */
extern  void AdvanceFrameRenderPrerequisite();

/**
 * Waits for the rendering thread to finish executing all pending rendering commands.  Should only be used from the game thread.
 */
extern  void FlushRenderingCommands(bool bFlushDeferredDeletes = false);

extern  void FlushPendingDeleteRHIResources_GameThread();
extern  void FlushPendingDeleteRHIResources_RenderThread();

extern  void TickRenderingTickables();

extern  void StartRenderCommandFenceBundler();
extern  void StopRenderCommandFenceBundler();


/** The parent class of commands stored in the rendering command queue. */
class  FRenderCommand
{
public:
	// All render commands run on the render thread
	static ENamedThreads::Type GetDesiredThread()
	{
		assert(!GIsThreadedRendering || ENamedThreads::GetRenderThread() != ENamedThreads::GameThread);
		return ENamedThreads::GetRenderThread();
	}

	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		// Don't support tasks having dependencies on us, reduces task graph overhead tracking and dealing with subsequents
		return ESubsequentsMode::FireAndForget;
	}
};

//
// Macros for using render commands.
//
// ideally this would be inline, however that changes the module dependency situation
extern class FRHICommandListImmediate& GetImmediateCommandList_ForRenderCommand();

inline bool ShouldExecuteOnRenderThread() { return (GIsThreadedRendering || !IsInGameThread()); }

#define TASK_FUNCTION(Code) \
		void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) \
		{ \
			FRHICommandListImmediate& RHICmdList = GetImmediateCommandList_ForRenderCommand(); \
			Code; \
		} 

template<typename TSTR, typename LAMBDA>
class TEnqueueUniqueRenderCommandType : public FRenderCommand
{
public:
	TEnqueueUniqueRenderCommandType(LAMBDA&& InLambda) : Lambda(Forward<LAMBDA>(InLambda)) {}

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		FRHICommandListImmediate& RHICmdList = GetImmediateCommandList_ForRenderCommand();
		Lambda(RHICmdList);
	}
private:
	LAMBDA Lambda;
};


template<typename TSTR, typename LAMBDA>
inline void EnqueueUniqueRenderCommand(LAMBDA&& Lambda)
{
	typedef TEnqueueUniqueRenderCommandType<TSTR, LAMBDA> EURCType;

#if 0 // UE_SERVER && UE_BUILD_DEBUG
	UE_LOG(LogRHI, Warning, TEXT("Render command '%s' is being executed on a dedicated server."), TSTR::TStr())
#endif
		// always use a new task for devices that have GUseThreadedRendering=false
		// even when the call is from the rendering thread
		if (GUseThreadedRendering && IsInRenderingThread())
		{
			FRHICommandListImmediate& RHICmdList = GetImmediateCommandList_ForRenderCommand();
			Lambda(RHICmdList);
		}
		else
		{
			if (ShouldExecuteOnRenderThread())
			{
				CheckNotBlockedOnRenderThread();
				TGraphTask<EURCType>::CreateTask().ConstructAndDispatchWhenReady(Forward<LAMBDA>(Lambda));
			}
			else
			{
				EURCType TempCommand(Forward<LAMBDA>(Lambda));
				FScopeCycleCounter EURCMacro_Scope(TempCommand.GetStatId());
				TempCommand.DoTask(ENamedThreads::GameThread, FGraphEventRef());
			}
		}
}

#define ENQUEUE_RENDER_COMMAND(Type) \
	struct Type##Name \
	{  \
		static const char* CStr() { return #Type; } \
		static const TCHAR* TStr() { return TEXT(#Type); } \
	}; \
	EnqueueUniqueRenderCommand<Type##Name>

template<typename LAMBDA>
inline void EnqueueUniqueRenderCommand(LAMBDA& Lambda)
{
	static_assert(sizeof(LAMBDA) == 0, "EnqueueUniqueRenderCommand enforces use of rvalue and therefore move to avoid an extra copy of the Lambda");
}

////////////////////////////////////
// Deferred cleanup
////////////////////////////////////

/**
 * The base class of objects that need to defer deletion until the render command queue has been flushed.
 */
class  FDeferredCleanupInterface
{
public:
	virtual ~FDeferredCleanupInterface() {}

		virtual void FinishCleanup() final {}
};

/**
 * A set of cleanup objects which are pending deletion.
 */
class FPendingCleanupObjects
{
	std::vector<FDeferredCleanupInterface*> CleanupArray;
public:
	FPendingCleanupObjects();
    virtual ~FPendingCleanupObjects();
};

/**
 * Adds the specified deferred cleanup object to the current set of pending cleanup objects.
 */
extern  void BeginCleanup(FDeferredCleanupInterface* CleanupObject);

/**
 * Transfers ownership of the current set of pending cleanup objects to the caller.  A new set is created for subsequent BeginCleanup calls.
 * @return A pointer to the set of pending cleanup objects.  The called is responsible for deletion.
 */
extern  FPendingCleanupObjects* GetPendingCleanupObjects();

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
