#pragma once
#include "Engine/YCommonHeader.h"
#include "Platform/Windows/YWindowsPlatformTLS.h"
#include "Platform/RunnableThread.h"
#include <atomic>

/** Thread ID of the main/game thread */
extern  uint32 GGameThreadId;

/** Thread ID of the render thread, if any */
extern  uint32 GRenderThreadId;

/** Thread ID of the slate thread, if any */
extern  uint32 GSlateLoadingThreadId;

/** Thread ID of the audio thread, if any */
extern  uint32 GAudioThreadId;

/** Has GGameThreadId been set yet? */
extern  bool GIsGameThreadIdInitialized;

/** Whether we want the rendering thread to be suspended, used e.g. for tracing. */
extern  bool GShouldSuspendRenderingThread;

extern  uint64 GFrameCounter;

/** GFrameCounter the last time GC was run. */
extern  uint64 GLastGCFrame;

/** Incremented once per frame before the scene is being rendered. In split screen mode this is incremented once for all views (not for each view). */
extern  uint32 GFrameNumber;

/** NEED TO RENAME, for RT version of GFrameTime use View.ViewFamily->FrameNumber or pass down from RT from GFrameTime). */
extern  uint32 GFrameNumberRenderThread;

extern  bool GIsCriticalError;
/** @return True if called from the game thread. */
FORCEINLINE bool IsInGameThread()
{
	if (GIsGameThreadIdInitialized)
	{
		const uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		return CurrentThreadId == GGameThreadId;
	}

	return true;
}

/** @return True if called from the audio thread, and not merely a thread calling audio functions. */
extern  bool IsInAudioThread();

/** Thread used for audio */
extern  FRunnableThread* GAudioThread;

/** @return True if called from the slate thread, and not merely a thread calling slate functions. */
extern  bool IsInSlateThread();

/** @return True if called from the rendering thread, or if called from ANY thread during single threaded rendering */
extern  bool IsInRenderingThread();

/** @return True if called from the rendering thread, or if called from ANY thread that isn't the game thread, except that during single threaded rendering the game thread is ok too.*/
extern  bool IsInParallelRenderingThread();

/** @return True if called from the rendering thread. */
// Unlike IsInRenderingThread, this will always return false if we are running single threaded. It only returns true if this is actually a separate rendering thread. Mostly useful for checks
extern  bool IsInActualRenderingThread();

/** @return True if called from the async loading thread if it's enabled, otherwise if called from game thread while is async loading code. */
extern  bool (*IsInAsyncLoadingThread)();

/** Thread used for rendering */
extern  FRunnableThread* GRenderingThread;

/** Whether the rendering thread is suspended (not even processing the tickables) */
extern  std::atomic<int32> GIsRenderingThreadSuspended;

/** @return True if called from the RHI thread, or if called from ANY thread during single threaded rendering */
extern  bool IsInRHIThread();

/** Thread used for RHI */
extern  FRunnableThread* GRHIThread_InternalUseOnly;

/** Thread ID of the the thread we are executing RHI commands on. This could either be a constant dedicated thread or changing every task if we run the rhi thread on tasks. */
extern  uint32 GRHIThreadId;

/** Whether we're currently in the async loading code path or not */
extern  bool(*IsAsyncLoading)();

/** Suspends async package loading. */
extern  void (*SuspendAsyncLoading)();

/** Resumes async package loading. */
extern  void (*ResumeAsyncLoading)();

/** Suspends async package loading. */
extern  bool (*IsAsyncLoadingSuspended)();

/** Returns true if async loading is using the async loading thread */
extern  bool(*IsAsyncLoadingMultithreaded)();

/** Helper function to flush resource streaming. */
extern  void(*GFlushStreamingFunc)(void);
