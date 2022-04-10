#include "Engine/CoreGlobals.h"
uint32					GGameThreadId = 0;
uint32					GRenderThreadId = 0;
uint32					GSlateLoadingThreadId = 0;
uint32					GAudioThreadId = 0;
/** Has GGameThreadId been set yet?																			*/
bool					GIsGameThreadIdInitialized = false;

/** Whether we want the rendering thread to be suspended, used e.g. for tracing.							*/
bool					GShouldSuspendRenderingThread = false;

/* Steadily increasing frame counter.*/
uint64					GFrameCounter=0;
uint64					GLastGCFrame = 0;
/** Incremented once per frame before the scene is being rendered. In split screen mode this is incremented once for all views (not for each view). */
uint32					GFrameNumber = 1;
/** NEED TO RENAME, for RT version of GFrameTime use View.ViewFamily->FrameNumber or pass down from RT from GFrameTime). */
uint32					GFrameNumberRenderThread = 1;

bool					GIsRequestingExit = false;					/* Indicates that MainLoop() should be exited at the end of the current iteration */
bool GIsCriticalError = false;
bool GIsGPUCrashed = false;
bool(*IsAsyncLoading)();
void (*SuspendAsyncLoading)();
void (*ResumeAsyncLoading)();
bool (*IsAsyncLoadingSuspended)();
bool(*IsAsyncLoadingMultithreaded)();
void(*GFlushStreamingFunc)(void);
