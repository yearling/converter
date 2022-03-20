// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHICommandList.h: RHI Command List definitions for queueing up & executing later.
=============================================================================*/

#pragma once
#include "Template/Template.h"
#include "Math/YColor.h"
#include "Math/IntPoint.h"
#include "Math/IntRect.h"
#include "Math/Float16Color.h"
#include "Platform/ThreadSafeCounter.h"
#include "Engine/TaskGraphInterfaces.h"
#include "RHI/RHIResources.h"
#include "Engine/YCommonHeader.h"
#include "Engine/TaskGraphCommon.h"
#include <array>
#include "Utility/MemStack.h"
#include <algorithm>
#include <lmerrlog.h>
#include "DynamicRHI.h"
class FApp;
class FBlendStateInitializerRHI;
class FGraphicsPipelineStateInitializer;
class FLastRenderTimeContainer;
class FReadSurfaceDataFlags;
class FRHICommandListBase;
class FRHIComputeShader;
class FRHIDepthRenderTargetView;
class FRHIRenderTargetView;
class FRHISetRenderTargetsInfo;
class IRHICommandContext;
class IRHIComputeContext;
struct FBoundShaderStateInput;
struct FDepthStencilStateInitializerRHI;
struct FRasterizerStateInitializerRHI;
struct FResolveParams;
struct FRHIResourceCreateInfo;
struct FRHIResourceInfo;
struct FRHIUniformBufferLayout;
struct FSamplerStateInitializerRHI;
struct FTextureMemoryStats;
enum class EAsyncComputeBudget;
enum class EClearDepthStencil;
enum class EResourceTransitionAccess;
enum class EResourceTransitionPipeline;
class FComputePipelineState;
class FGraphicsPipelineState;

//DECLARE_STATS_GROUP(TEXT("RHICmdList"), STATGROUP_RHICMDLIST, STATCAT_Advanced);


// set this one to get a stat for each RHI command 
#define RHI_STATS 0

#if RHI_STATS
DECLARE_STATS_GROUP(TEXT("RHICommands"),STATGROUP_RHI_COMMANDS, STATCAT_Advanced);
#define RHISTAT(Method)	DECLARE_SCOPE_CYCLE_COUNTER(TEXT(#Method), STAT_RHI##Method, STATGROUP_RHI_COMMANDS)
#else
#define RHISTAT(Method)
#endif

extern  bool GUseRHIThread_InternalUseOnly;
extern  bool GUseRHITaskThreads_InternalUseOnly;
extern  bool GIsRunningRHIInSeparateThread_InternalUseOnly;
extern  bool GIsRunningRHIInDedicatedThread_InternalUseOnly;
extern  bool GIsRunningRHIInTaskThread_InternalUseOnly;

/** private accumulator for the RHI thread. */
extern  uint32_t GWorkingRHIThreadTime;
extern  uint32_t GWorkingRHIThreadStallTime;

/**
* Whether the RHI commands are being run in a thread other than the render thread
*/
bool inline IsRunningRHIInSeparateThread()
{
	return GIsRunningRHIInSeparateThread_InternalUseOnly;
}

/**
* Whether the RHI commands are being run on a dedicated thread other than the render thread
*/
bool inline IsRunningRHIInDedicatedThread()
{
	return GIsRunningRHIInDedicatedThread_InternalUseOnly;
}

/**
* Whether the RHI commands are being run on a dedicated thread other than the render thread
*/
bool inline IsRunningRHIInTaskThread()
{
	return GIsRunningRHIInTaskThread_InternalUseOnly;
}




extern  bool GEnableAsyncCompute;
//extern  TAutoConsoleVariable<int32_t> CVarRHICmdWidth;
//extern  TAutoConsoleVariable<int32_t> CVarRHICmdFlushRenderThreadTasks;
extern int32_t CVarRHImdWidth;
extern int32_t CVarRHICmdFlushRenderThreadTasks;

class FRHICommandListBase;

struct  FLockTracker
{
	struct FLockParams
	{
		void* RHIBuffer;
		void* Buffer;
		uint32_t BufferSize;
		uint32_t Offset;
		EResourceLockMode LockMode;

		inline FLockParams(void* InRHIBuffer, void* InBuffer, uint32_t InOffset, uint32_t InBufferSize, EResourceLockMode InLockMode)
			: RHIBuffer(InRHIBuffer)
			, Buffer(InBuffer)
			, BufferSize(InBufferSize)
			, Offset(InOffset)
			, LockMode(InLockMode)
		{
		}
	};
	std::vector<FLockParams> OutstandingLocks;
	uint32_t TotalMemoryOutstanding;

	FLockTracker()
	{
		TotalMemoryOutstanding = 0;
	}

	inline void Lock(void* RHIBuffer, void* Buffer, uint32_t Offset, uint32_t SizeRHI, EResourceLockMode LockMode)
	{
#if DO_CHECK
		for (auto& Parms : OutstandingLocks)
		{
			assert(Parms.RHIBuffer != RHIBuffer);
		}
#endif
		OutstandingLocks.push_back(FLockParams(RHIBuffer, Buffer, Offset, SizeRHI, LockMode));
		TotalMemoryOutstanding += SizeRHI;
	}
	inline FLockParams Unlock(void* RHIBuffer)
	{
		for (int32_t Index = 0; Index < OutstandingLocks.size(); Index++)
		{
			if (OutstandingLocks[Index].RHIBuffer == RHIBuffer)
			{
				FLockParams Result = OutstandingLocks[Index];
				//OutstandingLocks.RemoveAtSwap(Index, 1, false);
				OutstandingLocks.erase(OutstandingLocks.begin() + Index);
				return Result;
			}
		}
		assert(!"Mismatched RHI buffer locks.");
		return FLockParams(nullptr, nullptr, 0, 0, RLM_WriteOnly);
	}
};

#ifdef CONTINUABLE_PSO_VERIFY
#define PSO_VERIFY ensure
#else
#define PSO_VERIFY	assert
#endif

enum class ECmdList
{
	EGfx,
	ECompute,	
};

class IRHICommandContextContainer
{
public:
	virtual ~IRHICommandContextContainer()
	{
	}

	virtual IRHICommandContext* GetContext()
	{
		return nullptr;
	}

	virtual void SubmitAndFreeContextContainer(int32_t Index, int32_t Num)
	{
		assert(0);
	}

	virtual void FinishContext()
	{
		assert(0);
	}
};

struct FRHICommandListDebugContext
{
	FRHICommandListDebugContext()
	{
#if RHI_COMMAND_LIST_DEBUG_TRACES
		DebugStringStore[MaxDebugStoreSize] = 1337;
#endif
	}

	void PushMarker(const char* Marker)
	{
#if RHI_COMMAND_LIST_DEBUG_TRACES
		//allocate a new slot for the stack of pointers
		//and preserve the top of the stack in case we reach the limit
		if (++DebugMarkerStackIndex >= MaxDebugMarkerStackDepth)
		{
			for (uint32_t i = 1; i < MaxDebugMarkerStackDepth; i++)
			{
				DebugMarkerStack[i - 1] = DebugMarkerStack[i];
				DebugMarkerSizes[i - 1] = DebugMarkerSizes[i];
			}
			DebugMarkerStackIndex = MaxDebugMarkerStackDepth - 1;
		}

		//try and copy the sting into the debugstore on the stack
		char* Offset = &DebugStringStore[DebugStoreOffset];
		uint32_t MaxLength = MaxDebugStoreSize - DebugStoreOffset;
		uint32_t Length = TryCopyString(Offset, Marker, MaxLength) + 1;

		//if we reached the end reset to the start and try again
		if (Length >= MaxLength)
		{
			DebugStoreOffset = 0;
			Offset = &DebugStringStore[DebugStoreOffset];
			MaxLength = MaxDebugStoreSize;
			Length = TryCopyString(Offset, Marker, MaxLength) + 1;

			//if the sting was bigger than the size of the store just terminate what we have
			if (Length >= MaxDebugStoreSize)
			{
				DebugStringStore[MaxDebugStoreSize - 1] = TEXT('\0');
			}
		}

		//add the string to the stack
		DebugMarkerStack[DebugMarkerStackIndex] = Offset;
		DebugStoreOffset += Length;
		DebugMarkerSizes[DebugMarkerStackIndex] = Length;

		assert(DebugStringStore[MaxDebugStoreSize] == 1337);
#endif
	}

	void PopMarker()
	{
#if RHI_COMMAND_LIST_DEBUG_TRACES
		//clean out the debug stack if we have valid data
		if (DebugMarkerStackIndex >= 0 && DebugMarkerStackIndex < MaxDebugMarkerStackDepth)
		{
			DebugMarkerStack[DebugMarkerStackIndex] = nullptr;
			//also free the data in the store to postpone wrapping as much as possibler
			DebugStoreOffset -= DebugMarkerSizes[DebugMarkerStackIndex];

			//in case we already wrapped in the past just assume we start allover again
			if (DebugStoreOffset >= MaxDebugStoreSize)
			{
				DebugStoreOffset = 0;
			}
		}

		//pop the stack pointer
		if (--DebugMarkerStackIndex == (~0u) - 1)
		{
			//in case we wrapped in the past just restart
			DebugMarkerStackIndex = ~0u;
		}
#endif
	}

#if RHI_COMMAND_LIST_DEBUG_TRACES
private:

	//Tries to copy a string and early exits if it hits the limit. 
	//Returns the size of the string or the limit when reached.
	uint32_t TryCopyString(char* Dest, const char* Source, uint32_t MaxLength)
	{
		uint32_t Length = 0;
		while(Source[Length] != TEXT('\0') && Length < MaxLength)
		{
			Dest[Length] = Source[Length];
			Length++;
		}

		if (Length < MaxLength)
		{
			Dest[Length] = TEXT('\0');
		}
		return Length;
	}

	uint32_t DebugStoreOffset = 0;
	static constexpr int MaxDebugStoreSize = 1023;
	char DebugStringStore[MaxDebugStoreSize + 1];

	uint32_t DebugMarkerStackIndex = ~0u;
	static constexpr int MaxDebugMarkerStackDepth = 32;
	const char* DebugMarkerStack[MaxDebugMarkerStackDepth] = {};
	uint32_t DebugMarkerSizes[MaxDebugMarkerStackDepth] = {};
#endif
};

struct FRHICommandBase
{
	FRHICommandBase* Next = nullptr;
	virtual void ExecuteAndDestruct(FRHICommandListBase& CmdList, FRHICommandListDebugContext& DebugContext) = 0;
};

// Thread-safe allocator for GPU fences used in deferred command list execution
// Fences are stored in a ringbuffer
class  FRHICommandListFenceAllocator
{
public:
	static const int MAX_FENCE_INDICES		= 4096;
	FRHICommandListFenceAllocator()
	{
		CurrentFenceIndex = 0;
		for ( int i=0; i<MAX_FENCE_INDICES; i++)
		{
			FenceIDs[i] = 0xffffffffffffffffull;
			FenceFrameNumber[i] = 0xffffffff;
		}
	}

	uint32_t AllocFenceIndex()
	{
		assert(IsInRenderingThread());
		uint32_t FenceIndex = ( FPlatformAtomics::InterlockedIncrement(&CurrentFenceIndex)-1 ) % MAX_FENCE_INDICES;
		assert(FenceFrameNumber[FenceIndex] != GFrameNumberRenderThread);
		FenceFrameNumber[FenceIndex] = GFrameNumberRenderThread;

		return FenceIndex;
	}

	volatile uint64_t& GetFenceID( int32_t FenceIndex )
	{
		assert( FenceIndex < MAX_FENCE_INDICES );
		return FenceIDs[ FenceIndex ];
	}

private:
	volatile int32_t CurrentFenceIndex;
	uint64_t FenceIDs[MAX_FENCE_INDICES];
	uint32_t FenceFrameNumber[MAX_FENCE_INDICES];
};

extern  FRHICommandListFenceAllocator GRHIFenceAllocator;

class  FRHICommandListBase : public FNoncopyable
{
public:
	FRHICommandListBase(FRHIGPUMask InGPUMask);
	~FRHICommandListBase();

	/** Custom new/delete with recycling */
	void* operator new(size_t Size);
	void operator delete(void *RawMemory);

	inline void Flush();
	inline bool IsImmediate();
	inline bool IsImmediateAsyncCompute();

	const int32_t GetUsedMemory() const;
	void QueueAsyncCommandListSubmit(FGraphEventRef& AnyThreadCompletionEvent, class FRHICommandList* CmdList);
	void QueueParallelAsyncCommandListSubmit(FGraphEventRef* AnyThreadCompletionEvents, bool bIsPrepass, class FRHICommandList** CmdLists, int32_t* NumDrawsIfKnown, int32_t Num, int32_t MinDrawsPerTranslate, bool bSpewMerge);
	void QueueRenderThreadCommandListSubmit(FGraphEventRef& RenderThreadCompletionEvent, class FRHICommandList* CmdList);
	void QueueAsyncPipelineStateCompile(FGraphEventRef& AsyncCompileCompletionEvent);
	void QueueCommandListSubmit(class FRHICommandList* CmdList);
	void WaitForTasks(bool bKnownToBeComplete = false);
	void WaitForDispatch();
	void WaitForRHIThreadTasks();
	void HandleRTThreadTaskCompletion(const FGraphEventRef& MyCompletionGraphEvent);

	inline void* Alloc(int32_t AllocSize, int32_t Alignment)
	{
		return MemManager.Alloc(AllocSize, Alignment);
	}

	template <typename T>
	inline void* Alloc()
	{
		return Alloc(sizeof(T), alignof(T));
	}


	inline char* AllocString(const char* Name)
	{
		int32_t Len = strlen(Name) + 1;
		char* NameCopy  = (char*)Alloc(Len * (int32_t)sizeof(char), (int32_t)sizeof(char));
		strcpy(NameCopy, Name);
		return NameCopy;
	}

	template <typename TCmd>
	inline void* AllocCommand()
	{
		checkSlow(!IsExecuting());
		TCmd* Result = (TCmd*)Alloc<TCmd>();
		++NumCommands;
		*CommandLink = Result;
		CommandLink = &Result->Next;
		return Result;
	}
	inline uint32_t GetUID()
	{
		return UID;
	}
	inline bool HasCommands() const
	{
		return (NumCommands > 0);
	}
	inline bool IsExecuting() const
	{
		return bExecuting;
	}

	bool Bypass();

	inline void ExchangeCmdList(FRHICommandListBase& Other)
	{
		assert(!RTTasks.size() && !Other.RTTasks.size());
		//FMemory::Memswap(this, &Other, sizeof(FRHICommandListBase));
		std::swap_ranges(this, this + sizeof(FRHICommandListBase), &Other);
		if (CommandLink == &Other.Root)
		{
			CommandLink = &Root;
		}
		if (Other.CommandLink == &Root)
		{
			Other.CommandLink = &Other.Root;
		}
	}

	void SetContext(IRHICommandContext* InContext)
	{
		assert(InContext);
		Context = InContext;
	}

	inline IRHICommandContext& GetContext()
	{
		checkSlow(Context);
		return *Context;
	}

	void SetComputeContext(IRHIComputeContext* InContext)
	{
		assert(InContext);
		ComputeContext = InContext;
	}

	IRHIComputeContext& GetComputeContext()
	{
		checkSlow(ComputeContext);
		return *ComputeContext;
	}

	void CopyContext(FRHICommandListBase& ParentCommandList)
	{
		assert(Context);
		ensure(GPUMask == ParentCommandList.GPUMask);
		Context = ParentCommandList.Context;
		ComputeContext = ParentCommandList.ComputeContext;
	}

	void MaybeDispatchToRHIThread()
	{
		if (IsImmediate() && HasCommands() && GRHIThreadNeedsKicking && IsRunningRHIInSeparateThread())
		{
			MaybeDispatchToRHIThreadInner();
		}
	}
	void MaybeDispatchToRHIThreadInner();

	struct FDrawUpData
	{

		uint32_t PrimitiveType;
		uint32_t NumPrimitives;
		uint32_t NumVertices;
		uint32_t VertexDataStride;
		void* OutVertexData;
		uint32_t MinVertexIndex;
		uint32_t NumIndices;
		uint32_t IndexDataStride;
		void* OutIndexData;

		FDrawUpData()
			: PrimitiveType(PT_Num)
			, OutVertexData(nullptr)
			, OutIndexData(nullptr)
		{
		}
	};

	inline const FRHIGPUMask& GetGPUMask() const { return GPUMask; }

private:
	FRHICommandBase* Root;
	FRHICommandBase** CommandLink;
	bool bExecuting;
	uint32_t NumCommands;
	uint32_t UID;
	IRHICommandContext* Context;
	IRHIComputeContext* ComputeContext;
	FMemStackBase MemManager; 
	FGraphEventArray RTTasks;

	friend class FRHICommandListExecutor;
	friend class FRHICommandListIterator;

protected:
	FRHIGPUMask GPUMask;
	void Reset();

public:
	//TStatId	ExecuteStat;
	enum class ERenderThreadContext
	{
		SceneRenderTargets,
		Num
	};
	void *RenderThreadContexts[(int32_t)ERenderThreadContext::Num];

protected:
	//the values of this struct must be copied when the commandlist is split 
	struct FPSOContext
	{
		uint32_t CachedNumSimultanousRenderTargets = 0;
		std::array<FRHIRenderTargetView, MaxSimultaneousRenderTargets> CachedRenderTargets;
		FRHIDepthRenderTargetView CachedDepthStencilTarget;
	} PSOContext;

	void CacheActiveRenderTargets(
		uint32_t NewNumSimultaneousRenderTargets,
		const FRHIRenderTargetView* NewRenderTargetsRHI,
		const FRHIDepthRenderTargetView* NewDepthStencilTargetRHI
		)
	{
		PSOContext.CachedNumSimultanousRenderTargets = NewNumSimultaneousRenderTargets;

		for (uint32_t RTIdx = 0; RTIdx < PSOContext.CachedNumSimultanousRenderTargets; ++RTIdx)
		{
			PSOContext.CachedRenderTargets[RTIdx] = NewRenderTargetsRHI[RTIdx];
		}

		PSOContext.CachedDepthStencilTarget = (NewDepthStencilTargetRHI) ? *NewDepthStencilTargetRHI : FRHIDepthRenderTargetView();
	}

	void CacheActiveRenderTargets(const FRHIRenderPassInfo& Info)
	{
		FRHISetRenderTargetsInfo RTInfo;
		Info.ConvertToRenderTargetsInfo(RTInfo);
		CacheActiveRenderTargets(RTInfo.NumColorRenderTargets, RTInfo.ColorRenderTarget, &RTInfo.DepthStencilRenderTarget);
	}

public:
	void CopyRenderThreadContexts(const FRHICommandListBase& ParentCommandList)
	{
		for (int32_t Index = 0; ERenderThreadContext(Index) < ERenderThreadContext::Num; Index++)
		{
			RenderThreadContexts[Index] = ParentCommandList.RenderThreadContexts[Index];
		}
	}
	void SetRenderThreadContext(void* InContext, ERenderThreadContext Slot)
	{
		RenderThreadContexts[int32_t(Slot)] = InContext;
	}
	inline void* GetRenderThreadContext(ERenderThreadContext Slot)
	{
		return RenderThreadContexts[int32_t(Slot)];
	}

	FDrawUpData DrawUPData; 

	struct FCommonData
	{
		class FRHICommandListBase* Parent = nullptr;

		enum class ECmdListType
		{
			Immediate = 1,
			Regular,
		};
		ECmdListType Type = ECmdListType::Regular;
		bool bInsideRenderPass = false;
		bool bInsideComputePass = false;
	};

	bool DoValidation() const
	{
		//static auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RenderPass.Validation"));
		//return CVar && CVar->GetInt() != 0;
		return 0;
	}

	inline bool IsOutsideRenderPass() const
	{
		return !Data.bInsideRenderPass;
	}

	inline bool IsInsideRenderPass() const
	{
		return Data.bInsideRenderPass;
	}

	inline bool IsInsideComputePass() const
	{
		return Data.bInsideComputePass;
	}

	FCommonData Data;
};

template<typename TCmd>
struct FRHICommand : public FRHICommandBase
{
	void ExecuteAndDestruct(FRHICommandListBase& CmdList, FRHICommandListDebugContext& Context) override final
	{
		TCmd *ThisCmd = static_cast<TCmd*>(this);
#if RHI_COMMAND_LIST_DEBUG_TRACES
		ThisCmd->StoreDebugInfo(Context);
#endif
		ThisCmd->Execute(CmdList);
		ThisCmd->~TCmd();
	}

	virtual void StoreDebugInfo(FRHICommandListDebugContext& Context) {};
};

struct  FRHICommandBeginUpdateMultiFrameResource final : public FRHICommand<FRHICommandBeginUpdateMultiFrameResource>
{
	FTextureRHIParamRef Texture;
	inline FRHICommandBeginUpdateMultiFrameResource(FTextureRHIParamRef InTexture)
		: Texture(InTexture)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct  FRHICommandEndUpdateMultiFrameResource final : public FRHICommand<FRHICommandEndUpdateMultiFrameResource>
{
	FTextureRHIParamRef Texture;
	inline FRHICommandEndUpdateMultiFrameResource(FTextureRHIParamRef InTexture)
		: Texture(InTexture)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct  FRHICommandBeginUpdateMultiFrameUAV final : public FRHICommand<FRHICommandBeginUpdateMultiFrameResource>
{
	FUnorderedAccessViewRHIParamRef UAV;
	inline FRHICommandBeginUpdateMultiFrameUAV(FUnorderedAccessViewRHIParamRef InUAV)
		: UAV(InUAV)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct  FRHICommandEndUpdateMultiFrameUAV final : public FRHICommand<FRHICommandEndUpdateMultiFrameResource>
{
	FUnorderedAccessViewRHIParamRef UAV;
	inline FRHICommandEndUpdateMultiFrameUAV(FUnorderedAccessViewRHIParamRef InUAV)
		: UAV(InUAV)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetRasterizerState final : public FRHICommand<FRHICommandSetRasterizerState>
{
	FRasterizerStateRHIParamRef State;
	inline FRHICommandSetRasterizerState(FRasterizerStateRHIParamRef InState)
		: State(InState)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetDepthStencilState final : public FRHICommand<FRHICommandSetDepthStencilState>
{
	FDepthStencilStateRHIParamRef State;
	uint32_t StencilRef;
	inline FRHICommandSetDepthStencilState(FDepthStencilStateRHIParamRef InState, uint32_t InStencilRef)
		: State(InState)
		, StencilRef(InStencilRef)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetStencilRef final : public FRHICommand<FRHICommandSetStencilRef>
{
	uint32_t StencilRef;
	inline FRHICommandSetStencilRef(uint32_t InStencilRef)
		: StencilRef(InStencilRef)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template <typename TShaderRHIParamRef, ECmdList CmdListType>
struct FRHICommandSetShaderParameter final : public FRHICommand<FRHICommandSetShaderParameter<TShaderRHIParamRef, CmdListType> >
{
	TShaderRHIParamRef Shader;
	const void* NewValue;
	uint32_t BufferIndex;
	uint32_t BaseIndex;
	uint32_t NumBytes;
	inline FRHICommandSetShaderParameter(TShaderRHIParamRef InShader, uint32_t InBufferIndex, uint32_t InBaseIndex, uint32_t InNumBytes, const void* InNewValue)
		: Shader(InShader)
		, NewValue(InNewValue)
		, BufferIndex(InBufferIndex)
		, BaseIndex(InBaseIndex)
		, NumBytes(InNumBytes)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template <typename TShaderRHIParamRef, ECmdList CmdListType>
struct FRHICommandSetShaderUniformBuffer final : public FRHICommand<FRHICommandSetShaderUniformBuffer<TShaderRHIParamRef, CmdListType> >
{
	TShaderRHIParamRef Shader;
	uint32_t BaseIndex;
	FUniformBufferRHIParamRef UniformBuffer;
	inline FRHICommandSetShaderUniformBuffer(TShaderRHIParamRef InShader, uint32_t InBaseIndex, FUniformBufferRHIParamRef InUniformBuffer)
		: Shader(InShader)
		, BaseIndex(InBaseIndex)
		, UniformBuffer(InUniformBuffer)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template <typename TShaderRHIParamRef, ECmdList CmdListType>
struct FRHICommandSetShaderTexture final : public FRHICommand<FRHICommandSetShaderTexture<TShaderRHIParamRef, CmdListType> >
{
	TShaderRHIParamRef Shader;
	uint32_t TextureIndex;
	FTextureRHIParamRef Texture;
	inline FRHICommandSetShaderTexture(TShaderRHIParamRef InShader, uint32_t InTextureIndex, FTextureRHIParamRef InTexture)
		: Shader(InShader)
		, TextureIndex(InTextureIndex)
		, Texture(InTexture)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template <typename TShaderRHIParamRef, ECmdList CmdListType>
struct FRHICommandSetShaderResourceViewParameter final : public FRHICommand<FRHICommandSetShaderResourceViewParameter<TShaderRHIParamRef, CmdListType> >
{
	TShaderRHIParamRef Shader;
	uint32_t SamplerIndex;
	FShaderResourceViewRHIParamRef SRV;
	inline FRHICommandSetShaderResourceViewParameter(TShaderRHIParamRef InShader, uint32_t InSamplerIndex, FShaderResourceViewRHIParamRef InSRV)
		: Shader(InShader)
		, SamplerIndex(InSamplerIndex)
		, SRV(InSRV)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template <typename TShaderRHIParamRef, ECmdList CmdListType>
struct FRHICommandSetUAVParameter final : public FRHICommand<FRHICommandSetUAVParameter<TShaderRHIParamRef, CmdListType> >
{
	TShaderRHIParamRef Shader;
	uint32_t UAVIndex;
	FUnorderedAccessViewRHIParamRef UAV;
	inline FRHICommandSetUAVParameter(TShaderRHIParamRef InShader, uint32_t InUAVIndex, FUnorderedAccessViewRHIParamRef InUAV)
		: Shader(InShader)
		, UAVIndex(InUAVIndex)
		, UAV(InUAV)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template <typename TShaderRHIParamRef, ECmdList CmdListType>
struct FRHICommandSetUAVParameter_IntialCount final : public FRHICommand<FRHICommandSetUAVParameter_IntialCount<TShaderRHIParamRef, CmdListType> >
{
	TShaderRHIParamRef Shader;
	uint32_t UAVIndex;
	FUnorderedAccessViewRHIParamRef UAV;
	uint32_t InitialCount;
	inline FRHICommandSetUAVParameter_IntialCount(TShaderRHIParamRef InShader, uint32_t InUAVIndex, FUnorderedAccessViewRHIParamRef InUAV, uint32_t InInitialCount)
		: Shader(InShader)
		, UAVIndex(InUAVIndex)
		, UAV(InUAV)
		, InitialCount(InInitialCount)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template <typename TShaderRHIParamRef, ECmdList CmdListType>
struct FRHICommandSetShaderSampler final : public FRHICommand<FRHICommandSetShaderSampler<TShaderRHIParamRef, CmdListType> >
{
	TShaderRHIParamRef Shader;
	uint32_t SamplerIndex;
	FSamplerStateRHIParamRef Sampler;
	inline FRHICommandSetShaderSampler(TShaderRHIParamRef InShader, uint32_t InSamplerIndex, FSamplerStateRHIParamRef InSampler)
		: Shader(InShader)
		, SamplerIndex(InSamplerIndex)
		, Sampler(InSampler)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandDrawPrimitive final : public FRHICommand<FRHICommandDrawPrimitive>
{
	uint32_t PrimitiveType;
	uint32_t BaseVertexIndex;
	uint32_t NumPrimitives;
	uint32_t NumInstances;
	inline FRHICommandDrawPrimitive(uint32_t InPrimitiveType, uint32_t InBaseVertexIndex, uint32_t InNumPrimitives, uint32_t InNumInstances)
		: PrimitiveType(InPrimitiveType)
		, BaseVertexIndex(InBaseVertexIndex)
		, NumPrimitives(InNumPrimitives)
		, NumInstances(InNumInstances)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandDrawIndexedPrimitive final : public FRHICommand<FRHICommandDrawIndexedPrimitive>
{
	FIndexBufferRHIParamRef IndexBuffer;
	uint32_t PrimitiveType;
	int32_t BaseVertexIndex;
	uint32_t FirstInstance;
	uint32_t NumVertices;
	uint32_t StartIndex;
	uint32_t NumPrimitives;
	uint32_t NumInstances;
	inline FRHICommandDrawIndexedPrimitive(FIndexBufferRHIParamRef InIndexBuffer, uint32_t InPrimitiveType, int32_t InBaseVertexIndex, uint32_t InFirstInstance, uint32_t InNumVertices, uint32_t InStartIndex, uint32_t InNumPrimitives, uint32_t InNumInstances)
		: IndexBuffer(InIndexBuffer)
		, PrimitiveType(InPrimitiveType)
		, BaseVertexIndex(InBaseVertexIndex)
		, FirstInstance(InFirstInstance)
		, NumVertices(InNumVertices)
		, StartIndex(InStartIndex)
		, NumPrimitives(InNumPrimitives)
		, NumInstances(InNumInstances)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetBoundShaderState final : public FRHICommand<FRHICommandSetBoundShaderState>
{
	FBoundShaderStateRHIParamRef BoundShaderState;
	inline FRHICommandSetBoundShaderState(FBoundShaderStateRHIParamRef InBoundShaderState)
		: BoundShaderState(InBoundShaderState)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetBlendState final : public FRHICommand<FRHICommandSetBlendState>
{
	FBlendStateRHIParamRef State;
	FLinearColor BlendFactor;
	inline FRHICommandSetBlendState(FBlendStateRHIParamRef InState, const FLinearColor& InBlendFactor)
		: State(InState)
		, BlendFactor(InBlendFactor)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetBlendFactor final : public FRHICommand<FRHICommandSetBlendFactor>
{
	FLinearColor BlendFactor;
	inline FRHICommandSetBlendFactor(const FLinearColor& InBlendFactor)
		: BlendFactor(InBlendFactor)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetStreamSource final : public FRHICommand<FRHICommandSetStreamSource>
{
	uint32_t StreamIndex;
	FVertexBufferRHIParamRef VertexBuffer;
	uint32_t Offset;
	inline FRHICommandSetStreamSource(uint32_t InStreamIndex, FVertexBufferRHIParamRef InVertexBuffer, uint32_t InOffset)
		: StreamIndex(InStreamIndex)
		, VertexBuffer(InVertexBuffer)
		, Offset(InOffset)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetViewport final : public FRHICommand<FRHICommandSetViewport>
{
	uint32_t MinX;
	uint32_t MinY;
	float MinZ;
	uint32_t MaxX;
	uint32_t MaxY;
	float MaxZ;
	inline FRHICommandSetViewport(uint32_t InMinX, uint32_t InMinY, float InMinZ, uint32_t InMaxX, uint32_t InMaxY, float InMaxZ)
		: MinX(InMinX)
		, MinY(InMinY)
		, MinZ(InMinZ)
		, MaxX(InMaxX)
		, MaxY(InMaxY)
		, MaxZ(InMaxZ)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetStereoViewport final : public FRHICommand<FRHICommandSetStereoViewport>
{
	uint32_t LeftMinX;
	uint32_t RightMinX;
	uint32_t LeftMinY;
	uint32_t RightMinY;
	float MinZ;
	uint32_t LeftMaxX;
	uint32_t RightMaxX;
	uint32_t LeftMaxY;
	uint32_t RightMaxY;
	float MaxZ;
	inline FRHICommandSetStereoViewport(uint32_t InLeftMinX, uint32_t InRightMinX, uint32_t InLeftMinY, uint32_t InRightMinY, float InMinZ, uint32_t InLeftMaxX, uint32_t InRightMaxX, uint32_t InLeftMaxY, uint32_t InRightMaxY, float InMaxZ)
		: LeftMinX(InLeftMinX)
		, RightMinX(InRightMinX)
		, LeftMinY(InLeftMinY)
		, RightMinY(InRightMinY)
		, MinZ(InMinZ)
		, LeftMaxX(InLeftMaxX)
		, RightMaxX(InRightMaxX)
		, LeftMaxY(InLeftMaxY)
		, RightMaxY(InRightMaxY)
		, MaxZ(InMaxZ)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetScissorRect final : public FRHICommand<FRHICommandSetScissorRect>
{
	bool bEnable;
	uint32_t MinX;
	uint32_t MinY;
	uint32_t MaxX;
	uint32_t MaxY;
	inline FRHICommandSetScissorRect(bool InbEnable, uint32_t InMinX, uint32_t InMinY, uint32_t InMaxX, uint32_t InMaxY)
		: bEnable(InbEnable)
		, MinX(InMinX)
		, MinY(InMinY)
		, MaxX(InMaxX)
		, MaxY(InMaxY)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetRenderTargets final : public FRHICommand<FRHICommandSetRenderTargets>
{
	uint32_t NewNumSimultaneousRenderTargets;
	FRHIRenderTargetView NewRenderTargetsRHI[MaxSimultaneousRenderTargets];
	FRHIDepthRenderTargetView NewDepthStencilTarget;
	uint32_t NewNumUAVs;
	FUnorderedAccessViewRHIParamRef UAVs[MaxSimultaneousUAVs];

	inline FRHICommandSetRenderTargets(
		uint32_t InNewNumSimultaneousRenderTargets,
		const FRHIRenderTargetView* InNewRenderTargetsRHI,
		const FRHIDepthRenderTargetView* InNewDepthStencilTargetRHI,
		uint32_t InNewNumUAVs,
		const FUnorderedAccessViewRHIParamRef* InUAVs
		)
		: NewNumSimultaneousRenderTargets(InNewNumSimultaneousRenderTargets)
		, NewNumUAVs(InNewNumUAVs)

	{
		assert(InNewNumSimultaneousRenderTargets <= MaxSimultaneousRenderTargets && InNewNumUAVs <= MaxSimultaneousUAVs);
		for (uint32_t Index = 0; Index < NewNumSimultaneousRenderTargets; Index++)
		{
			NewRenderTargetsRHI[Index] = InNewRenderTargetsRHI[Index];
		}
		for (uint32_t Index = 0; Index < NewNumUAVs; Index++)
		{
			UAVs[Index] = InUAVs[Index];
		}
		if (InNewDepthStencilTargetRHI)
		{
			NewDepthStencilTarget = *InNewDepthStencilTargetRHI;
		}		
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandBeginRenderPass final : public FRHICommand<FRHICommandBeginRenderPass>
{
	FRHIRenderPassInfo Info;
	const char* Name;

	FRHICommandBeginRenderPass(const FRHIRenderPassInfo& InInfo, const char* InName)
		: Info(InInfo)
		, Name(InName)
	{
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandEndRenderPass final : public FRHICommand<FRHICommandEndRenderPass>
{
	FRHICommandEndRenderPass()
	{
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FLocalCmdListParallelRenderPass
{
	TRefCountPtr<struct FRHIParallelRenderPass> RenderPass;
};

struct FRHICommandBeginParallelRenderPass final : public FRHICommand<FRHICommandBeginParallelRenderPass>
{
	FRHIRenderPassInfo Info;
	FLocalCmdListParallelRenderPass* LocalRenderPass;
	const char* Name;

	FRHICommandBeginParallelRenderPass(const FRHIRenderPassInfo& InInfo, FLocalCmdListParallelRenderPass* InLocalRenderPass, const char* InName)
		: Info(InInfo)
		, LocalRenderPass(InLocalRenderPass)
		, Name(InName)
	{
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandEndParallelRenderPass final : public FRHICommand<FRHICommandEndParallelRenderPass>
{
	FLocalCmdListParallelRenderPass* LocalRenderPass;

	FRHICommandEndParallelRenderPass(FLocalCmdListParallelRenderPass* InLocalRenderPass)
		: LocalRenderPass(InLocalRenderPass)
	{
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FLocalCmdListRenderSubPass
{
	TRefCountPtr<struct FRHIRenderSubPass> RenderSubPass;
};

struct FRHICommandBeginRenderSubPass final : public FRHICommand<FRHICommandBeginRenderSubPass>
{
	FLocalCmdListParallelRenderPass* LocalRenderPass;
	FLocalCmdListRenderSubPass* LocalRenderSubPass;

	FRHICommandBeginRenderSubPass(FLocalCmdListParallelRenderPass* InLocalRenderPass, FLocalCmdListRenderSubPass* InLocalRenderSubPass)
		: LocalRenderPass(InLocalRenderPass)
		, LocalRenderSubPass(InLocalRenderSubPass)
	{
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandEndRenderSubPass final : public FRHICommand<FRHICommandEndRenderSubPass>
{
	FLocalCmdListParallelRenderPass* LocalRenderPass;
	FLocalCmdListRenderSubPass* LocalRenderSubPass;

	FRHICommandEndRenderSubPass(FLocalCmdListParallelRenderPass* InLocalRenderPass, FLocalCmdListRenderSubPass* InLocalRenderSubPass)
		: LocalRenderPass(InLocalRenderPass)
		, LocalRenderSubPass(InLocalRenderSubPass)
	{
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandBeginComputePass final : public FRHICommand<FRHICommandBeginComputePass>
{
	const char* Name;

	FRHICommandBeginComputePass(const char* InName)
		: Name(InName)
	{
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandEndComputePass final : public FRHICommand<FRHICommandEndComputePass>
{
	FRHICommandEndComputePass()
	{
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetRenderTargetsAndClear final : public FRHICommand<FRHICommandSetRenderTargetsAndClear>
{
	FRHISetRenderTargetsInfo RenderTargetsInfo;

	inline FRHICommandSetRenderTargetsAndClear(const FRHISetRenderTargetsInfo& InRenderTargetsInfo) :
		RenderTargetsInfo(InRenderTargetsInfo)
	{
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandBindClearMRTValues final : public FRHICommand<FRHICommandBindClearMRTValues>
{
	bool bClearColor;
	bool bClearDepth;
	bool bClearStencil;

	inline FRHICommandBindClearMRTValues(
		bool InbClearColor,
		bool InbClearDepth,
		bool InbClearStencil
		) 
		: bClearColor(InbClearColor)
		, bClearDepth(InbClearDepth)
		, bClearStencil(InbClearStencil)
	{
	}	

	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandEndDrawPrimitiveUP final : public FRHICommand<FRHICommandEndDrawPrimitiveUP>
{
	uint32_t PrimitiveType;
	uint32_t NumPrimitives;
	uint32_t NumVertices;
	uint32_t VertexDataStride;
	void* OutVertexData;

	inline FRHICommandEndDrawPrimitiveUP(uint32_t InPrimitiveType, uint32_t InNumPrimitives, uint32_t InNumVertices, uint32_t InVertexDataStride, void* InOutVertexData)
		: PrimitiveType(InPrimitiveType)
		, NumPrimitives(InNumPrimitives)
		, NumVertices(InNumVertices)
		, VertexDataStride(InVertexDataStride)
		, OutVertexData(InOutVertexData)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};


struct FRHICommandEndDrawIndexedPrimitiveUP final : public FRHICommand<FRHICommandEndDrawIndexedPrimitiveUP>
{
	uint32_t PrimitiveType;
	uint32_t NumPrimitives;
	uint32_t NumVertices;
	uint32_t VertexDataStride;
	void* OutVertexData;
	uint32_t MinVertexIndex;
	uint32_t NumIndices;
	uint32_t IndexDataStride;
	void* OutIndexData;

	inline FRHICommandEndDrawIndexedPrimitiveUP(uint32_t InPrimitiveType, uint32_t InNumPrimitives, uint32_t InNumVertices, uint32_t InVertexDataStride, void* InOutVertexData, uint32_t InMinVertexIndex, uint32_t InNumIndices, uint32_t InIndexDataStride, void* InOutIndexData)
		: PrimitiveType(InPrimitiveType)
		, NumPrimitives(InNumPrimitives)
		, NumVertices(InNumVertices)
		, VertexDataStride(InVertexDataStride)
		, OutVertexData(InOutVertexData)
		, MinVertexIndex(InMinVertexIndex)
		, NumIndices(InNumIndices)
		, IndexDataStride(InIndexDataStride)
		, OutIndexData(InOutIndexData)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template<ECmdList CmdListType>
struct FRHICommandSetComputeShader final : public FRHICommand<FRHICommandSetComputeShader<CmdListType>>
{
	FComputeShaderRHIParamRef ComputeShader;
	inline FRHICommandSetComputeShader(FComputeShaderRHIParamRef InComputeShader)
		: ComputeShader(InComputeShader)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template<ECmdList CmdListType>
struct FRHICommandSetComputePipelineState final : public FRHICommand<FRHICommandSetComputePipelineState<CmdListType>>
{
	FComputePipelineState* ComputePipelineState;
	inline FRHICommandSetComputePipelineState(FComputePipelineState* InComputePipelineState)
		: ComputePipelineState(InComputePipelineState)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetGraphicsPipelineState final : public FRHICommand<FRHICommandSetGraphicsPipelineState>
{
	FGraphicsPipelineState* GraphicsPipelineState;
	inline FRHICommandSetGraphicsPipelineState(FGraphicsPipelineState* InGraphicsPipelineState)
		: GraphicsPipelineState(InGraphicsPipelineState)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template<ECmdList CmdListType>
struct FRHICommandDispatchComputeShader final : public FRHICommand<FRHICommandDispatchComputeShader<CmdListType>>
{
	uint32_t ThreadGroupCountX;
	uint32_t ThreadGroupCountY;
	uint32_t ThreadGroupCountZ;
	inline FRHICommandDispatchComputeShader(uint32_t InThreadGroupCountX, uint32_t InThreadGroupCountY, uint32_t InThreadGroupCountZ)
		: ThreadGroupCountX(InThreadGroupCountX)
		, ThreadGroupCountY(InThreadGroupCountY)
		, ThreadGroupCountZ(InThreadGroupCountZ)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template<ECmdList CmdListType>
struct FRHICommandDispatchIndirectComputeShader final : public FRHICommand<FRHICommandDispatchIndirectComputeShader<CmdListType>>
{
	FVertexBufferRHIParamRef ArgumentBuffer;
	uint32_t ArgumentOffset;
	inline FRHICommandDispatchIndirectComputeShader(FVertexBufferRHIParamRef InArgumentBuffer, uint32_t InArgumentOffset)
		: ArgumentBuffer(InArgumentBuffer)
		, ArgumentOffset(InArgumentOffset)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandAutomaticCacheFlushAfterComputeShader final : public FRHICommand<FRHICommandAutomaticCacheFlushAfterComputeShader>
{
	bool bEnable;
	inline FRHICommandAutomaticCacheFlushAfterComputeShader(bool InbEnable)
		: bEnable(InbEnable)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandFlushComputeShaderCache final : public FRHICommand<FRHICommandFlushComputeShaderCache>
{
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandDrawPrimitiveIndirect final : public FRHICommand<FRHICommandDrawPrimitiveIndirect>
{
	FVertexBufferRHIParamRef ArgumentBuffer;
	uint32_t PrimitiveType;
	uint32_t ArgumentOffset;
	inline FRHICommandDrawPrimitiveIndirect(uint32_t InPrimitiveType, FVertexBufferRHIParamRef InArgumentBuffer, uint32_t InArgumentOffset)
		: ArgumentBuffer(InArgumentBuffer)
		, PrimitiveType(InPrimitiveType)
		, ArgumentOffset(InArgumentOffset)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandDrawIndexedIndirect final : public FRHICommand<FRHICommandDrawIndexedIndirect>
{
	FIndexBufferRHIParamRef IndexBufferRHI;
	uint32_t PrimitiveType;
	FStructuredBufferRHIParamRef ArgumentsBufferRHI;
	uint32_t DrawArgumentsIndex;
	uint32_t NumInstances;

	inline FRHICommandDrawIndexedIndirect(FIndexBufferRHIParamRef InIndexBufferRHI, uint32_t InPrimitiveType, FStructuredBufferRHIParamRef InArgumentsBufferRHI, uint32_t InDrawArgumentsIndex, uint32_t InNumInstances)
		: IndexBufferRHI(InIndexBufferRHI)
		, PrimitiveType(InPrimitiveType)
		, ArgumentsBufferRHI(InArgumentsBufferRHI)
		, DrawArgumentsIndex(InDrawArgumentsIndex)
		, NumInstances(InNumInstances)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandDrawIndexedPrimitiveIndirect final : public FRHICommand<FRHICommandDrawIndexedPrimitiveIndirect>
{
	FIndexBufferRHIParamRef IndexBuffer;
	FVertexBufferRHIParamRef ArgumentsBuffer;
	uint32_t PrimitiveType;
	uint32_t ArgumentOffset;

	inline FRHICommandDrawIndexedPrimitiveIndirect(uint32_t InPrimitiveType, FIndexBufferRHIParamRef InIndexBuffer, FVertexBufferRHIParamRef InArgumentsBuffer, uint32_t InArgumentOffset)
		: IndexBuffer(InIndexBuffer)
		, ArgumentsBuffer(InArgumentsBuffer)
		, PrimitiveType(InPrimitiveType)
		, ArgumentOffset(InArgumentOffset)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetDepthBounds final : public FRHICommand<FRHICommandSetDepthBounds>
{
	float MinDepth;
	float MaxDepth;

	inline FRHICommandSetDepthBounds(float InMinDepth, float InMaxDepth)
		: MinDepth(InMinDepth)
		, MaxDepth(InMaxDepth)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandClearTinyUAV final : public FRHICommand<FRHICommandClearTinyUAV>
{
	FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI;
	uint32_t Values[4];

	inline FRHICommandClearTinyUAV(FUnorderedAccessViewRHIParamRef InUnorderedAccessViewRHI, const uint32_t* InValues)
		: UnorderedAccessViewRHI(InUnorderedAccessViewRHI)
	{
		Values[0] = InValues[0];
		Values[1] = InValues[1];
		Values[2] = InValues[2];
		Values[3] = InValues[3];
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandCopyToResolveTarget final : public FRHICommand<FRHICommandCopyToResolveTarget>
{
	FResolveParams ResolveParams;
	FTextureRHIParamRef SourceTexture;
	FTextureRHIParamRef DestTexture;

	inline FRHICommandCopyToResolveTarget(FTextureRHIParamRef InSourceTexture, FTextureRHIParamRef InDestTexture, const FResolveParams& InResolveParams)
		: ResolveParams(InResolveParams)
		, SourceTexture(InSourceTexture)
		, DestTexture(InDestTexture)
	{
		ensure(SourceTexture);
		ensure(DestTexture);
		ensure(SourceTexture->GetTexture2D() || SourceTexture->GetTexture3D() || SourceTexture->GetTextureCube());
		ensure(DestTexture->GetTexture2D() || DestTexture->GetTexture3D() || DestTexture->GetTextureCube());
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandCopyTexture final : public FRHICommand<FRHICommandCopyTexture>
{
	FRHICopyTextureInfo CopyInfo;
	FTextureRHIParamRef SourceTexture;
	FTextureRHIParamRef DestTexture;

	inline FRHICommandCopyTexture(FTextureRHIParamRef InSourceTexture, FTextureRHIParamRef InDestTexture, const FRHICopyTextureInfo& InCopyInfo)
		: CopyInfo(InCopyInfo)
		, SourceTexture(InSourceTexture)
		, DestTexture(InDestTexture)
	{
		ensure(SourceTexture);
		ensure(DestTexture);
		ensure(SourceTexture->GetTexture2D() || SourceTexture->GetTexture3D() || SourceTexture->GetTextureCube());
		ensure(DestTexture->GetTexture2D() || DestTexture->GetTexture3D() || DestTexture->GetTextureCube());
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandTransitionTextures final : public FRHICommand<FRHICommandTransitionTextures>
{
	int32_t NumTextures;
	FTextureRHIParamRef* Textures; // Pointer to an array of textures, allocated inline with the command list
	EResourceTransitionAccess TransitionType;
	inline FRHICommandTransitionTextures(EResourceTransitionAccess InTransitionType, FTextureRHIParamRef* InTextures, int32_t InNumTextures)
		: NumTextures(InNumTextures)
		, Textures(InTextures)
		, TransitionType(InTransitionType)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandTransitionTexturesArray final : public FRHICommand<FRHICommandTransitionTexturesArray>
{	
	std::vector<FTextureRHIParamRef>& Textures;
	EResourceTransitionAccess TransitionType;
	inline FRHICommandTransitionTexturesArray(EResourceTransitionAccess InTransitionType, std::vector<FTextureRHIParamRef>& InTextures)
		: Textures(InTextures)
		, TransitionType(InTransitionType)
	{		
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template<ECmdList CmdListType>
struct FRHICommandTransitionUAVs final : public FRHICommand<FRHICommandTransitionUAVs<CmdListType>>
{
	int32_t NumUAVs;
	FUnorderedAccessViewRHIParamRef* UAVs; // Pointer to an array of UAVs, allocated inline with the command list
	EResourceTransitionAccess TransitionType;
	EResourceTransitionPipeline TransitionPipeline;
	FComputeFenceRHIParamRef WriteFence;

	inline FRHICommandTransitionUAVs(EResourceTransitionAccess InTransitionType, EResourceTransitionPipeline InTransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32_t InNumUAVs, FComputeFenceRHIParamRef InWriteFence)
		: NumUAVs(InNumUAVs)
		, UAVs(InUAVs)
		, TransitionType(InTransitionType)
		, TransitionPipeline(InTransitionPipeline)
		, WriteFence(InWriteFence)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template<ECmdList CmdListType>
struct FRHICommandSetAsyncComputeBudget final : public FRHICommand<FRHICommandSetAsyncComputeBudget<CmdListType>>
{
	EAsyncComputeBudget Budget;

	inline FRHICommandSetAsyncComputeBudget(EAsyncComputeBudget InBudget)
		: Budget(InBudget)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template<ECmdList CmdListType>
struct FRHICommandWaitComputeFence final : public FRHICommand<FRHICommandWaitComputeFence<CmdListType>>
{
	FComputeFenceRHIParamRef WaitFence;

	inline FRHICommandWaitComputeFence(FComputeFenceRHIParamRef InWaitFence)
		: WaitFence(InWaitFence)
	{		
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandClearColorTexture final : public FRHICommand<FRHICommandClearColorTexture>
{
	FTextureRHIParamRef Texture;
	FLinearColor Color;

	inline FRHICommandClearColorTexture(
		FTextureRHIParamRef InTexture,
		const FLinearColor& InColor
		)
		: Texture(InTexture)
		, Color(InColor)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandClearDepthStencilTexture final : public FRHICommand<FRHICommandClearDepthStencilTexture>
{
	FTextureRHIParamRef Texture;
	float Depth;
	uint32_t Stencil;
	EClearDepthStencil ClearDepthStencil;

	inline FRHICommandClearDepthStencilTexture(
		FTextureRHIParamRef InTexture,
		EClearDepthStencil InClearDepthStencil,
		float InDepth,
		uint32_t InStencil
	)
		: Texture(InTexture)
		, Depth(InDepth)
		, Stencil(InStencil)
		, ClearDepthStencil(InClearDepthStencil)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandClearColorTextures final : public FRHICommand<FRHICommandClearColorTextures>
{
	FLinearColor ColorArray[MaxSimultaneousRenderTargets];
	FTextureRHIParamRef Textures[MaxSimultaneousRenderTargets];
	int32_t NumClearColors;

	inline FRHICommandClearColorTextures(
		int32_t InNumClearColors,
		FTextureRHIParamRef* InTextures,
		const FLinearColor* InColorArray
		)
		: NumClearColors(InNumClearColors)
	{
		assert(InNumClearColors <= MaxSimultaneousRenderTargets);
		for (int32_t Index = 0; Index < InNumClearColors; Index++)
		{
			ColorArray[Index] = InColorArray[Index];
			Textures[Index] = InTextures[Index];
		}
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FComputedGraphicsPipelineState
{
	FGraphicsPipelineStateRHIRef GraphicsPipelineState;
	int32_t UseCount;
	FComputedGraphicsPipelineState()
		: UseCount(0)
	{
	}
};

struct FLocalGraphicsPipelineStateWorkArea
{
	FGraphicsPipelineStateInitializer Args;
	FComputedGraphicsPipelineState* ComputedGraphicsPipelineState;
#if DO_CHECK // the below variables are used in assert(), which can be enabled in Shipping builds (see Build.h)
	FRHICommandListBase* CheckCmdList;
	int32_t UID;
#endif

	inline FLocalGraphicsPipelineStateWorkArea(
		FRHICommandListBase* InCheckCmdList,
		const FGraphicsPipelineStateInitializer& Initializer
		)
		: Args(Initializer)
#if DO_CHECK
		, CheckCmdList(InCheckCmdList)
		, UID(InCheckCmdList->GetUID())
#endif
	{
		ComputedGraphicsPipelineState = new (InCheckCmdList->Alloc<FComputedGraphicsPipelineState>()) FComputedGraphicsPipelineState;
	}
};

struct FLocalGraphicsPipelineState
{
	FLocalGraphicsPipelineStateWorkArea* WorkArea;
	FGraphicsPipelineStateRHIRef BypassGraphicsPipelineState; // this is only used in the case of Bypass, should eventually be deleted

	FLocalGraphicsPipelineState()
		: WorkArea(nullptr)
	{
	}

	FLocalGraphicsPipelineState(const FLocalGraphicsPipelineState& Other)
		: WorkArea(Other.WorkArea)
		, BypassGraphicsPipelineState(Other.BypassGraphicsPipelineState)
	{
	}
};

struct FRHICommandBuildLocalGraphicsPipelineState final : public FRHICommand<FRHICommandBuildLocalGraphicsPipelineState>
{
	FLocalGraphicsPipelineStateWorkArea WorkArea;

	inline FRHICommandBuildLocalGraphicsPipelineState(
		FRHICommandListBase* CheckCmdList,
		const FGraphicsPipelineStateInitializer& Initializer
		)
		: WorkArea(CheckCmdList, Initializer)
	{
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandSetLocalGraphicsPipelineState final : public FRHICommand<FRHICommandSetLocalGraphicsPipelineState>
{
	FLocalGraphicsPipelineState LocalGraphicsPipelineState;
	inline FRHICommandSetLocalGraphicsPipelineState(FRHICommandListBase* CheckCmdList, FLocalGraphicsPipelineState& InLocalGraphicsPipelineState)
		: LocalGraphicsPipelineState(InLocalGraphicsPipelineState)
	{
		//assert(CheckCmdList == LocalGraphicsPipelineState.WorkArea->CheckCmdList && CheckCmdList->GetUID() == LocalGraphicsPipelineState.WorkArea->UID); // this PSO was not built for this particular commandlist
		LocalGraphicsPipelineState.WorkArea->ComputedGraphicsPipelineState->UseCount++;
	}

	 void Execute(FRHICommandListBase& CmdList);
};

struct FComputedUniformBuffer
{
	FUniformBufferRHIRef UniformBuffer;
	mutable int32_t UseCount;
	FComputedUniformBuffer()
		: UseCount(0)
	{
	}
};

struct FLocalUniformBufferWorkArea
{
	void* Contents;
	const FRHIUniformBufferLayout* Layout;
	FComputedUniformBuffer* ComputedUniformBuffer;
#if DO_CHECK // the below variables are used in assert(), which can be enabled in Shipping builds (see Build.h)
	FRHICommandListBase* CheckCmdList;
	int32_t UID;
#endif

	FLocalUniformBufferWorkArea(FRHICommandListBase* InCheckCmdList, const void* InContents, uint32_t ContentsSize, const FRHIUniformBufferLayout* InLayout)
		: Layout(InLayout)
#if DO_CHECK
		, CheckCmdList(InCheckCmdList)
		, UID(InCheckCmdList->GetUID())
#endif
	{
		assert(ContentsSize);
		Contents = InCheckCmdList->Alloc(ContentsSize, UNIFORM_BUFFER_STRUCT_ALIGNMENT);
		memcpy(Contents, InContents, ContentsSize);
		ComputedUniformBuffer = new (InCheckCmdList->Alloc<FComputedUniformBuffer>()) FComputedUniformBuffer;
	}
};

struct FLocalUniformBuffer
{
	FLocalUniformBufferWorkArea* WorkArea;
	FUniformBufferRHIRef BypassUniform; // this is only used in the case of Bypass, should eventually be deleted
	FLocalUniformBuffer()
		: WorkArea(nullptr)
	{
	}
	FLocalUniformBuffer(const FLocalUniformBuffer& Other)
		: WorkArea(Other.WorkArea)
		, BypassUniform(Other.BypassUniform)
	{
	}
	inline bool IsValid() const
	{
		return WorkArea || IsValidRef(BypassUniform);
	}
};

struct FRHICommandBuildLocalUniformBuffer final : public FRHICommand<FRHICommandBuildLocalUniformBuffer>
{
	FLocalUniformBufferWorkArea WorkArea;
	inline FRHICommandBuildLocalUniformBuffer(
		FRHICommandListBase* CheckCmdList,
		const void* Contents,
		uint32_t ContentsSize,
		const FRHIUniformBufferLayout& Layout
		)
		: WorkArea(CheckCmdList, Contents, ContentsSize, &Layout)

	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template <typename TShaderRHIParamRef>
struct FRHICommandSetLocalUniformBuffer final : public FRHICommand<FRHICommandSetLocalUniformBuffer<TShaderRHIParamRef> >
{
	TShaderRHIParamRef Shader;
	uint32_t BaseIndex;
	FLocalUniformBuffer LocalUniformBuffer;
	inline FRHICommandSetLocalUniformBuffer(FRHICommandListBase* CheckCmdList, TShaderRHIParamRef InShader, uint32_t InBaseIndex, const FLocalUniformBuffer& InLocalUniformBuffer)
		: Shader(InShader)
		, BaseIndex(InBaseIndex)
		, LocalUniformBuffer(InLocalUniformBuffer)

	{
		assert(CheckCmdList == LocalUniformBuffer.WorkArea->CheckCmdList && CheckCmdList->GetUID() == LocalUniformBuffer.WorkArea->UID); // this uniform buffer was not built for this particular commandlist
		LocalUniformBuffer.WorkArea->ComputedUniformBuffer->UseCount++;
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandBeginRenderQuery final : public FRHICommand<FRHICommandBeginRenderQuery>
{
	FRenderQueryRHIParamRef RenderQuery;

	inline FRHICommandBeginRenderQuery(FRenderQueryRHIParamRef InRenderQuery)
		: RenderQuery(InRenderQuery)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandEndRenderQuery final : public FRHICommand<FRHICommandEndRenderQuery>
{
	FRenderQueryRHIParamRef RenderQuery;

	inline FRHICommandEndRenderQuery(FRenderQueryRHIParamRef InRenderQuery)
		: RenderQuery(InRenderQuery)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template<ECmdList CmdListType>
struct FRHICommandSubmitCommandsHint final : public FRHICommand<FRHICommandSubmitCommandsHint<CmdListType>>
{
	inline FRHICommandSubmitCommandsHint()
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandBeginScene final : public FRHICommand<FRHICommandBeginScene>
{
	inline FRHICommandBeginScene()
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandEndScene final : public FRHICommand<FRHICommandEndScene>
{
	inline FRHICommandEndScene()
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandBeginFrame final : public FRHICommand<FRHICommandBeginFrame>
{
	inline FRHICommandBeginFrame()
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandEndFrame final : public FRHICommand<FRHICommandEndFrame>
{
	inline FRHICommandEndFrame()
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandBeginDrawingViewport final : public FRHICommand<FRHICommandBeginDrawingViewport>
{
	FViewportRHIParamRef Viewport;
	FTextureRHIParamRef RenderTargetRHI;

	inline FRHICommandBeginDrawingViewport(FViewportRHIParamRef InViewport, FTextureRHIParamRef InRenderTargetRHI)
		: Viewport(InViewport)
		, RenderTargetRHI(InRenderTargetRHI)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandEndDrawingViewport final : public FRHICommand<FRHICommandEndDrawingViewport>
{
	FViewportRHIParamRef Viewport;
	bool bPresent;
	bool bLockToVsync;

	inline FRHICommandEndDrawingViewport(FViewportRHIParamRef InViewport, bool InbPresent, bool InbLockToVsync)
		: Viewport(InViewport)
		, bPresent(InbPresent)
		, bLockToVsync(InbLockToVsync)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};

template<ECmdList CmdListType>
struct FRHICommandPushEvent final : public FRHICommand<FRHICommandPushEvent<CmdListType>>
{
	const char *Name;
	FColor Color;

	inline FRHICommandPushEvent(const char *InName, FColor InColor)
		: Name(InName)
		, Color(InColor)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);

	virtual void StoreDebugInfo(FRHICommandListDebugContext& Context)
	{
		Context.PushMarker(Name);
	};
};

template<ECmdList CmdListType>
struct FRHICommandPopEvent final : public FRHICommand<FRHICommandPopEvent<CmdListType>>
{
	 void Execute(FRHICommandListBase& CmdList);

	virtual void StoreDebugInfo(FRHICommandListDebugContext& Context)
	{
		Context.PopMarker();
	};
};

struct FRHICommandInvalidateCachedState final : public FRHICommand<FRHICommandInvalidateCachedState>
{
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandDiscardRenderTargets final : public FRHICommand<FRHICommandDiscardRenderTargets>
{
	uint32_t ColorBitMask;
	bool Depth;
	bool Stencil;

	inline FRHICommandDiscardRenderTargets(bool InDepth, bool InStencil, uint32_t InColorBitMask)
		: ColorBitMask(InColorBitMask)
		, Depth(InDepth)
		, Stencil(InStencil)
	{
	}
	
	 void Execute(FRHICommandListBase& CmdList);
};

struct FRHICommandDebugBreak final : public FRHICommand<FRHICommandDebugBreak>
{
	void Execute(FRHICommandListBase& CmdList)
	{
		//if (FPlatformMisc::IsDebuggerPresent())
		//{
			//UE_DEBUG_BREAK();
		//}
	}
};

struct FRHICommandUpdateTextureReference final : public FRHICommand<FRHICommandUpdateTextureReference>
{
	FTextureReferenceRHIParamRef TextureRef;
	FTextureRHIParamRef NewTexture;
	inline FRHICommandUpdateTextureReference(FTextureReferenceRHIParamRef InTextureRef, FTextureRHIParamRef InNewTexture)
		: TextureRef(InTextureRef)
		, NewTexture(InNewTexture)
	{
	}
	 void Execute(FRHICommandListBase& CmdList);
};


#define CMD_CONTEXT(Method) GetContext().Method
#define COMPUTE_CONTEXT(Method) GetComputeContext().Method

template<> void FRHICommandSetShaderParameter<FComputeShaderRHIParamRef, ECmdList::ECompute>::Execute(FRHICommandListBase& CmdList);
template<> void FRHICommandSetShaderUniformBuffer<FComputeShaderRHIParamRef, ECmdList::ECompute>::Execute(FRHICommandListBase& CmdList);
template<> void FRHICommandSetShaderTexture<FComputeShaderRHIParamRef, ECmdList::ECompute>::Execute(FRHICommandListBase& CmdList);
template<> void FRHICommandSetShaderResourceViewParameter<FComputeShaderRHIParamRef, ECmdList::ECompute>::Execute(FRHICommandListBase& CmdList);
template<> void FRHICommandSetShaderSampler<FComputeShaderRHIParamRef, ECmdList::ECompute>::Execute(FRHICommandListBase& CmdList);


class  FRHICommandList : public FRHICommandListBase
{
public:

	inline FRHICommandList(FRHIGPUMask GPUMask) : FRHICommandListBase(GPUMask) {}

	/** Custom new/delete with recycling */
	void* operator new(size_t Size);
	void operator delete(void *RawMemory);
	
	inline void BeginUpdateMultiFrameResource( FTextureRHIParamRef Texture)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIBeginUpdateMultiFrameResource)( Texture);
			return;
		}
		new (AllocCommand<FRHICommandBeginUpdateMultiFrameResource>()) FRHICommandBeginUpdateMultiFrameResource( Texture);
	}

	inline void EndUpdateMultiFrameResource(FTextureRHIParamRef Texture)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIEndUpdateMultiFrameResource)(Texture);
			return;
		}
		new (AllocCommand<FRHICommandEndUpdateMultiFrameResource>()) FRHICommandEndUpdateMultiFrameResource(Texture);
	}

	inline void BeginUpdateMultiFrameResource(FUnorderedAccessViewRHIParamRef UAV)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIBeginUpdateMultiFrameResource)(UAV);
			return;
		}
		new (AllocCommand<FRHICommandBeginUpdateMultiFrameUAV>()) FRHICommandBeginUpdateMultiFrameUAV(UAV);
	}

	inline void EndUpdateMultiFrameResource(FUnorderedAccessViewRHIParamRef UAV)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIEndUpdateMultiFrameResource)(UAV);
			return;
		}
		new (AllocCommand<FRHICommandEndUpdateMultiFrameUAV>()) FRHICommandEndUpdateMultiFrameUAV(UAV);
	}

	//DEPRECATED(4.20, "BuildLocalGraphicsPipelineState is deprecated. Use PipelineStateCache::GetAndOrCreateGraphicsPipelineState() instead.")
	inline FLocalGraphicsPipelineState BuildLocalGraphicsPipelineState(
		const FGraphicsPipelineStateInitializer& Initializer
		)
	{
		//assert(IsOutsideRenderPass());
		FLocalGraphicsPipelineState Result;
		if (Bypass())
		{
			Result.BypassGraphicsPipelineState = RHICreateGraphicsPipelineState(Initializer);
		}
		else
		{
			auto* Cmd = new (AllocCommand<FRHICommandBuildLocalGraphicsPipelineState>()) FRHICommandBuildLocalGraphicsPipelineState(this, Initializer);
			Result.WorkArea = &Cmd->WorkArea;
		}
		return Result;
	}

	//DEPRECATED(4.20, "SetLocalGraphicsPipelineState is deprecated. Use SetGraphicsPipelineState() instead.")
	inline void SetLocalGraphicsPipelineState(FLocalGraphicsPipelineState LocalGraphicsPipelineState)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetGraphicsPipelineState)(LocalGraphicsPipelineState.BypassGraphicsPipelineState);
			return;
		}
		new (AllocCommand<FRHICommandSetLocalGraphicsPipelineState>()) FRHICommandSetLocalGraphicsPipelineState(this, LocalGraphicsPipelineState);
	}
	
	inline FLocalUniformBuffer BuildLocalUniformBuffer(const void* Contents, uint32_t ContentsSize, const FRHIUniformBufferLayout& Layout)
	{
		//assert(IsOutsideRenderPass());
		FLocalUniformBuffer Result;
		if (Bypass())
		{
			Result.BypassUniform = RHICreateUniformBuffer(Contents, Layout, UniformBuffer_SingleFrame);
		}
		else
		{
			assert(Contents && ContentsSize && (&Layout != nullptr));
			auto* Cmd = new (AllocCommand<FRHICommandBuildLocalUniformBuffer>()) FRHICommandBuildLocalUniformBuffer(this, Contents, ContentsSize, Layout);
			Result.WorkArea = &Cmd->WorkArea;
		}
		return Result;
	}

	template <typename TShaderRHIParamRef>
	inline void SetLocalShaderUniformBuffer(TShaderRHIParamRef Shader, uint32_t BaseIndex, const FLocalUniformBuffer& UniformBuffer)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetShaderUniformBuffer)(Shader, BaseIndex, UniformBuffer.BypassUniform);
			return;
		}
		new (AllocCommand<FRHICommandSetLocalUniformBuffer<TShaderRHIParamRef> >()) FRHICommandSetLocalUniformBuffer<TShaderRHIParamRef>(this, Shader, BaseIndex, UniformBuffer);
	}

	template <typename TShaderRHI>
	inline void SetLocalShaderUniformBuffer(TRefCountPtr<TShaderRHI>& Shader, uint32_t BaseIndex, const FLocalUniformBuffer& UniformBuffer)
	{
		SetLocalShaderUniformBuffer(Shader.GetReference(), BaseIndex, UniformBuffer);
	}

	template <typename TShaderRHI>
	inline void SetShaderUniformBuffer(TShaderRHI* Shader, uint32_t BaseIndex, FUniformBufferRHIParamRef UniformBuffer)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetShaderUniformBuffer)(Shader, BaseIndex, UniformBuffer);
			return;
		}
		new (AllocCommand<FRHICommandSetShaderUniformBuffer<TShaderRHI*, ECmdList::EGfx> >()) FRHICommandSetShaderUniformBuffer<TShaderRHI*, ECmdList::EGfx>(Shader, BaseIndex, UniformBuffer);
	}
	template <typename TShaderRHI>
	inline void SetShaderUniformBuffer(TRefCountPtr<TShaderRHI>& Shader, uint32_t BaseIndex, FUniformBufferRHIParamRef UniformBuffer)
	{
		SetShaderUniformBuffer(Shader.GetReference(), BaseIndex, UniformBuffer);
	}

	template <typename TShaderRHI>
	inline void SetShaderParameter(TShaderRHI* Shader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetShaderParameter)(Shader, BufferIndex, BaseIndex, NumBytes, NewValue);
			return;
		}
		void* UseValue = Alloc(NumBytes, 16);
		FMemory::Memcpy(UseValue, NewValue, NumBytes);
		new (AllocCommand<FRHICommandSetShaderParameter<TShaderRHI*, ECmdList::EGfx> >()) FRHICommandSetShaderParameter<TShaderRHI*, ECmdList::EGfx>(Shader, BufferIndex, BaseIndex, NumBytes, UseValue);
	}
	template <typename TShaderRHI>
	inline void SetShaderParameter(TRefCountPtr<TShaderRHI>& Shader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue)
	{
		SetShaderParameter(Shader.GetReference(), BufferIndex, BaseIndex, NumBytes, NewValue);
	}

	template <typename TShaderRHIParamRef>
	inline void SetShaderTexture(TShaderRHIParamRef Shader, uint32_t TextureIndex, FTextureRHIParamRef Texture)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetShaderTexture)(Shader, TextureIndex, Texture);
			return;
		}
		new (AllocCommand<FRHICommandSetShaderTexture<TShaderRHIParamRef, ECmdList::EGfx> >()) FRHICommandSetShaderTexture<TShaderRHIParamRef, ECmdList::EGfx>(Shader, TextureIndex, Texture);
	}

	template <typename TShaderRHI>
	inline void SetShaderTexture(TRefCountPtr<TShaderRHI>& Shader, uint32_t TextureIndex, FTextureRHIParamRef Texture)
	{
		SetShaderTexture(Shader.GetReference(), TextureIndex, Texture);
	}

	template <typename TShaderRHIParamRef>
	inline void SetShaderResourceViewParameter(TShaderRHIParamRef Shader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetShaderResourceViewParameter)(Shader, SamplerIndex, SRV);
			return;
		}
		new (AllocCommand<FRHICommandSetShaderResourceViewParameter<TShaderRHIParamRef, ECmdList::EGfx> >()) FRHICommandSetShaderResourceViewParameter<TShaderRHIParamRef, ECmdList::EGfx>(Shader, SamplerIndex, SRV);
	}

	template <typename TShaderRHI>
	inline void SetShaderResourceViewParameter(TRefCountPtr<TShaderRHI>& Shader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV)
	{
		SetShaderResourceViewParameter(Shader.GetReference(), SamplerIndex, SRV);
	}

	template <typename TShaderRHIParamRef>
	inline void SetShaderSampler(TShaderRHIParamRef Shader, uint32_t SamplerIndex, FSamplerStateRHIParamRef State)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetShaderSampler)(Shader, SamplerIndex, State);
			return;
		}
		new (AllocCommand<FRHICommandSetShaderSampler<TShaderRHIParamRef, ECmdList::EGfx> >()) FRHICommandSetShaderSampler<TShaderRHIParamRef, ECmdList::EGfx>(Shader, SamplerIndex, State);
	}

	template <typename TShaderRHI>
	inline void SetShaderSampler(TRefCountPtr<TShaderRHI>& Shader, uint32_t SamplerIndex, FSamplerStateRHIParamRef State)
	{
		SetShaderSampler(Shader.GetReference(), SamplerIndex, State);
	}

	inline void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHISetUAVParameter)(Shader, UAVIndex, UAV);
			return;
		}
		new (AllocCommand<FRHICommandSetUAVParameter<FComputeShaderRHIParamRef, ECmdList::EGfx> >()) FRHICommandSetUAVParameter<FComputeShaderRHIParamRef, ECmdList::EGfx>(Shader, UAVIndex, UAV);
	}

	inline void SetUAVParameter(TRefCountPtr<FRHIComputeShader>& Shader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV)
	{
		SetUAVParameter(Shader.GetReference(), UAVIndex, UAV);
	}

	inline void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32_t InitialCount)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHISetUAVParameter)(Shader, UAVIndex, UAV, InitialCount);
			return;
		}
		new (AllocCommand<FRHICommandSetUAVParameter_IntialCount<FComputeShaderRHIParamRef, ECmdList::EGfx> >()) FRHICommandSetUAVParameter_IntialCount<FComputeShaderRHIParamRef, ECmdList::EGfx>(Shader, UAVIndex, UAV, InitialCount);
	}

	inline void SetUAVParameter(TRefCountPtr<FRHIComputeShader>& Shader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32_t InitialCount)
	{
		SetUAVParameter(Shader.GetReference(), UAVIndex, UAV, InitialCount);
	}

	inline void SetBlendFactor(const FLinearColor& BlendFactor = FLinearColor::White)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetBlendFactor)(BlendFactor);
			return;
		}
		new (AllocCommand<FRHICommandSetBlendFactor>()) FRHICommandSetBlendFactor(BlendFactor);
	}

	inline void DrawPrimitive(uint32_t PrimitiveType, uint32_t BaseVertexIndex, uint32_t NumPrimitives, uint32_t NumInstances)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIDrawPrimitive)(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
			return;
		}
		new (AllocCommand<FRHICommandDrawPrimitive>()) FRHICommandDrawPrimitive(PrimitiveType, BaseVertexIndex, NumPrimitives, NumInstances);
	}

	inline void DrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32_t PrimitiveType, int32_t BaseVertexIndex, uint32_t FirstInstance, uint32_t NumVertices, uint32_t StartIndex, uint32_t NumPrimitives, uint32_t NumInstances)
	{
		if (!IndexBuffer)
		{
			//UE_LOG(LogRHI, Fatal, TEXT("Tried to call DrawIndexedPrimitive with null IndexBuffer!"));
			ERROR_INFO("Tried to call DrawIndexedPrimitive with null IndexBuffer!");
		}

		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIDrawIndexedPrimitive)(IndexBuffer, PrimitiveType, BaseVertexIndex, FirstInstance, NumVertices, StartIndex, NumPrimitives, NumInstances);
			return;
		}
		new (AllocCommand<FRHICommandDrawIndexedPrimitive>()) FRHICommandDrawIndexedPrimitive(IndexBuffer, PrimitiveType, BaseVertexIndex, FirstInstance, NumVertices, StartIndex, NumPrimitives, NumInstances);
	}

	inline void SetStreamSource(uint32_t StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32_t Offset)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHISetStreamSource)(StreamIndex, VertexBuffer, Offset);
			return;
		}
		new (AllocCommand<FRHICommandSetStreamSource>()) FRHICommandSetStreamSource(StreamIndex, VertexBuffer, Offset);
	}

	inline void SetStencilRef(uint32_t StencilRef)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetStencilRef)(StencilRef);
			return;
		}

		new(AllocCommand<FRHICommandSetStencilRef>()) FRHICommandSetStencilRef(StencilRef);
	}

	inline void SetViewport(uint32_t MinX, uint32_t MinY, float MinZ, uint32_t MaxX, uint32_t MaxY, float MaxZ)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetViewport)(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
			return;
		}
		new (AllocCommand<FRHICommandSetViewport>()) FRHICommandSetViewport(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
	}

	inline void SetStereoViewport(uint32_t LeftMinX, uint32_t RightMinX, uint32_t LeftMinY, uint32_t RightMinY, float MinZ, uint32_t LeftMaxX, uint32_t RightMaxX, uint32_t LeftMaxY, uint32_t RightMaxY, float MaxZ)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetStereoViewport)(LeftMinX, RightMinX, LeftMinY, RightMinY, MinZ, LeftMaxX, RightMaxX, LeftMaxY, RightMaxY, MaxZ);
			return;
		}
		new (AllocCommand<FRHICommandSetStereoViewport>()) FRHICommandSetStereoViewport(LeftMinX, RightMinX, LeftMinY, RightMinY, MinZ, LeftMaxX, RightMaxX, LeftMaxY, RightMaxY, MaxZ);
	}

	inline void SetScissorRect(bool bEnable, uint32_t MinX, uint32_t MinY, uint32_t MaxX, uint32_t MaxY)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetScissorRect)(bEnable, MinX, MinY, MaxX, MaxY);
			return;
		}
		new (AllocCommand<FRHICommandSetScissorRect>()) FRHICommandSetScissorRect(bEnable, MinX, MinY, MaxX, MaxY);
	}

	void ApplyCachedRenderTargets(
		FGraphicsPipelineStateInitializer& GraphicsPSOInit
		)
	{
		GraphicsPSOInit.RenderTargetsEnabled = PSOContext.CachedNumSimultanousRenderTargets;

		for (uint32_t i = 0; i < GraphicsPSOInit.RenderTargetsEnabled; ++i)
		{
			if (PSOContext.CachedRenderTargets[i].Texture)
			{
				GraphicsPSOInit.RenderTargetFormats[i] = PSOContext.CachedRenderTargets[i].Texture->GetFormat();
				GraphicsPSOInit.RenderTargetFlags[i] = PSOContext.CachedRenderTargets[i].Texture->GetFlags();
			}
			else
			{
				GraphicsPSOInit.RenderTargetFormats[i] = PF_Unknown;
			}

			GraphicsPSOInit.RenderTargetLoadActions[i] = PSOContext.CachedRenderTargets[i].LoadAction;
			GraphicsPSOInit.RenderTargetStoreActions[i] = PSOContext.CachedRenderTargets[i].StoreAction;

			if (GraphicsPSOInit.RenderTargetFormats[i] != PF_Unknown)
			{
				GraphicsPSOInit.NumSamples = PSOContext.CachedRenderTargets[i].Texture->GetNumSamples();
			}
		}

		if (PSOContext.CachedDepthStencilTarget.Texture)
		{
			GraphicsPSOInit.DepthStencilTargetFormat = PSOContext.CachedDepthStencilTarget.Texture->GetFormat();
			GraphicsPSOInit.DepthStencilTargetFlag = PSOContext.CachedDepthStencilTarget.Texture->GetFlags();
		}
		else
		{
			GraphicsPSOInit.DepthStencilTargetFormat = PF_Unknown;
		}

		GraphicsPSOInit.DepthTargetLoadAction = PSOContext.CachedDepthStencilTarget.DepthLoadAction;
		GraphicsPSOInit.DepthTargetStoreAction = PSOContext.CachedDepthStencilTarget.DepthStoreAction;
		GraphicsPSOInit.StencilTargetLoadAction = PSOContext.CachedDepthStencilTarget.StencilLoadAction;
		GraphicsPSOInit.StencilTargetStoreAction = PSOContext.CachedDepthStencilTarget.GetStencilStoreAction();
		GraphicsPSOInit.DepthStencilAccess = PSOContext.CachedDepthStencilTarget.GetDepthStencilAccess();

		if (GraphicsPSOInit.DepthStencilTargetFormat != PF_Unknown)
		{
			GraphicsPSOInit.NumSamples = PSOContext.CachedDepthStencilTarget.Texture->GetNumSamples();
		}
	}

	inline void SetRenderTargets(
		uint32_t NewNumSimultaneousRenderTargets,
		const FRHIRenderTargetView* NewRenderTargetsRHI,
		const FRHIDepthRenderTargetView* NewDepthStencilTargetRHI,
		uint32_t NewNumUAVs,
		const FUnorderedAccessViewRHIParamRef* UAVs
		)
	{
		assert(IsOutsideRenderPass());
		CacheActiveRenderTargets(
			NewNumSimultaneousRenderTargets, 
			NewRenderTargetsRHI, 
			NewDepthStencilTargetRHI
			);

		if (Bypass())
		{
			CMD_CONTEXT(RHISetRenderTargets)(
				NewNumSimultaneousRenderTargets,
				NewRenderTargetsRHI,
				NewDepthStencilTargetRHI,
				NewNumUAVs,
				UAVs);
			return;
		}
		new (AllocCommand<FRHICommandSetRenderTargets>()) FRHICommandSetRenderTargets(
			NewNumSimultaneousRenderTargets,
			NewRenderTargetsRHI,
			NewDepthStencilTargetRHI,
			NewNumUAVs,
			UAVs);
	}

	inline void SetRenderTargetsAndClear(const FRHISetRenderTargetsInfo& RenderTargetsInfo)
	{
		assert(IsOutsideRenderPass());
		CacheActiveRenderTargets(
			RenderTargetsInfo.NumColorRenderTargets, 
			RenderTargetsInfo.ColorRenderTarget, 
			&RenderTargetsInfo.DepthStencilRenderTarget
			);

		if (Bypass())
		{
			CMD_CONTEXT(RHISetRenderTargetsAndClear)(RenderTargetsInfo);
			return;
		}
		new (AllocCommand<FRHICommandSetRenderTargetsAndClear>()) FRHICommandSetRenderTargetsAndClear(RenderTargetsInfo);
	}	

	inline void BindClearMRTValues(bool bClearColor, bool bClearDepth, bool bClearStencil)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIBindClearMRTValues)(bClearColor, bClearDepth, bClearStencil);
			return;
		}
		new (AllocCommand<FRHICommandBindClearMRTValues>()) FRHICommandBindClearMRTValues(bClearColor, bClearDepth, bClearStencil);
	}	

	inline void BeginDrawPrimitiveUP(uint32_t PrimitiveType, uint32_t NumPrimitives, uint32_t NumVertices, uint32_t VertexDataStride, void*& OutVertexData)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIBeginDrawPrimitiveUP)(PrimitiveType, NumPrimitives, NumVertices, VertexDataStride, OutVertexData);
			return;
		}
		checkf(!DrawUPData.OutVertexData && NumVertices * VertexDataStride > 0,
			TEXT("Data: 0x%x, NumVerts:%i, Stride:%i"), (void*)DrawUPData.OutVertexData, NumVertices, VertexDataStride);

		OutVertexData = Alloc(NumVertices * VertexDataStride, 16);

		DrawUPData.PrimitiveType = PrimitiveType;
		DrawUPData.NumPrimitives = NumPrimitives;
		DrawUPData.NumVertices = NumVertices;
		DrawUPData.VertexDataStride = VertexDataStride;
		DrawUPData.OutVertexData = OutVertexData;
	}

	inline void EndDrawPrimitiveUP()
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIEndDrawPrimitiveUP)();
			return;
		}
		assert(DrawUPData.OutVertexData && DrawUPData.NumVertices);
		new (AllocCommand<FRHICommandEndDrawPrimitiveUP>()) FRHICommandEndDrawPrimitiveUP(DrawUPData.PrimitiveType, DrawUPData.NumPrimitives, DrawUPData.NumVertices, DrawUPData.VertexDataStride, DrawUPData.OutVertexData);

		DrawUPData.OutVertexData = nullptr;
		DrawUPData.NumVertices = 0;
	}

	inline void BeginDrawIndexedPrimitiveUP(uint32_t PrimitiveType, uint32_t NumPrimitives, uint32_t NumVertices, uint32_t VertexDataStride, void*& OutVertexData, uint32_t MinVertexIndex, uint32_t NumIndices, uint32_t IndexDataStride, void*& OutIndexData)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIBeginDrawIndexedPrimitiveUP)(PrimitiveType, NumPrimitives, NumVertices, VertexDataStride, OutVertexData, MinVertexIndex, NumIndices, IndexDataStride, OutIndexData);
			return;
		}
		assert(!DrawUPData.OutVertexData && !DrawUPData.OutIndexData && NumVertices * VertexDataStride > 0 && NumIndices * IndexDataStride > 0);

		OutVertexData = Alloc(NumVertices * VertexDataStride, 16);
		OutIndexData = Alloc(NumIndices * IndexDataStride, 16);

		DrawUPData.PrimitiveType = PrimitiveType;
		DrawUPData.NumPrimitives = NumPrimitives;
		DrawUPData.NumVertices = NumVertices;
		DrawUPData.VertexDataStride = VertexDataStride;
		DrawUPData.OutVertexData = OutVertexData;
		DrawUPData.MinVertexIndex = MinVertexIndex;
		DrawUPData.NumIndices = NumIndices;
		DrawUPData.IndexDataStride = IndexDataStride;
		DrawUPData.OutIndexData = OutIndexData;

	}

	inline void EndDrawIndexedPrimitiveUP()
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIEndDrawIndexedPrimitiveUP)();
			return;
		}
		assert(DrawUPData.OutVertexData && DrawUPData.OutIndexData && DrawUPData.NumIndices && DrawUPData.NumVertices);

		new (AllocCommand<FRHICommandEndDrawIndexedPrimitiveUP>()) FRHICommandEndDrawIndexedPrimitiveUP(DrawUPData.PrimitiveType, DrawUPData.NumPrimitives, DrawUPData.NumVertices, DrawUPData.VertexDataStride, DrawUPData.OutVertexData, DrawUPData.MinVertexIndex, DrawUPData.NumIndices, DrawUPData.IndexDataStride, DrawUPData.OutIndexData);

		DrawUPData.OutVertexData = nullptr;
		DrawUPData.OutIndexData = nullptr;
		DrawUPData.NumIndices = 0;
		DrawUPData.NumVertices = 0;
	}

	inline void SetComputeShader(FComputeShaderRHIParamRef ComputeShader)
	{
		ComputeShader->UpdateStats();
		if (Bypass())
		{
			CMD_CONTEXT(RHISetComputeShader)(ComputeShader);
			return;
		}
		new (AllocCommand<FRHICommandSetComputeShader<ECmdList::EGfx>>()) FRHICommandSetComputeShader<ECmdList::EGfx>(ComputeShader);
	}

	inline void SetComputePipelineState(class FComputePipelineState* ComputePipelineState)
	{
		if (Bypass())
		{
			extern  FRHIComputePipelineState* ExecuteSetComputePipelineState(class FComputePipelineState* ComputePipelineState);
			FRHIComputePipelineState* RHIComputePipelineState = ExecuteSetComputePipelineState(ComputePipelineState);
			CMD_CONTEXT(RHISetComputePipelineState)(RHIComputePipelineState);
			return;
		}
		new (AllocCommand<FRHICommandSetComputePipelineState<ECmdList::EGfx>>()) FRHICommandSetComputePipelineState<ECmdList::EGfx>(ComputePipelineState);
	}

	inline void SetGraphicsPipelineState(class FGraphicsPipelineState* GraphicsPipelineState)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			extern  FRHIGraphicsPipelineState* ExecuteSetGraphicsPipelineState(class FGraphicsPipelineState* GraphicsPipelineState);
			FRHIGraphicsPipelineState* RHIGraphicsPipelineState = ExecuteSetGraphicsPipelineState(GraphicsPipelineState);
			CMD_CONTEXT(RHISetGraphicsPipelineState)(RHIGraphicsPipelineState);
			return;
		}
		new (AllocCommand<FRHICommandSetGraphicsPipelineState>()) FRHICommandSetGraphicsPipelineState(GraphicsPipelineState);
	}

	inline void DispatchComputeShader(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIDispatchComputeShader)(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
			return;
		}
		new (AllocCommand<FRHICommandDispatchComputeShader<ECmdList::EGfx>>()) FRHICommandDispatchComputeShader<ECmdList::EGfx>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	inline void DispatchIndirectComputeShader(FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIDispatchIndirectComputeShader)(ArgumentBuffer, ArgumentOffset);
			return;
		}
		new (AllocCommand<FRHICommandDispatchIndirectComputeShader<ECmdList::EGfx>>()) FRHICommandDispatchIndirectComputeShader<ECmdList::EGfx>(ArgumentBuffer, ArgumentOffset);
	}

	inline void AutomaticCacheFlushAfterComputeShader(bool bEnable)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIAutomaticCacheFlushAfterComputeShader)(bEnable);
			return;
		}
		new (AllocCommand<FRHICommandAutomaticCacheFlushAfterComputeShader>()) FRHICommandAutomaticCacheFlushAfterComputeShader(bEnable);
	}

	inline void FlushComputeShaderCache()
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIFlushComputeShaderCache)();
			return;
		}
		new (AllocCommand<FRHICommandFlushComputeShaderCache>()) FRHICommandFlushComputeShaderCache();
	}

	inline void DrawPrimitiveIndirect(uint32_t PrimitiveType, FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIDrawPrimitiveIndirect)(PrimitiveType, ArgumentBuffer, ArgumentOffset);
			return;
		}
		new (AllocCommand<FRHICommandDrawPrimitiveIndirect>()) FRHICommandDrawPrimitiveIndirect(PrimitiveType, ArgumentBuffer, ArgumentOffset);
	}

	inline void DrawIndexedIndirect(FIndexBufferRHIParamRef IndexBufferRHI, uint32_t PrimitiveType, FStructuredBufferRHIParamRef ArgumentsBufferRHI, uint32_t DrawArgumentsIndex, uint32_t NumInstances)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIDrawIndexedIndirect)(IndexBufferRHI, PrimitiveType, ArgumentsBufferRHI, DrawArgumentsIndex, NumInstances);
			return;
		}
		new (AllocCommand<FRHICommandDrawIndexedIndirect>()) FRHICommandDrawIndexedIndirect(IndexBufferRHI, PrimitiveType, ArgumentsBufferRHI, DrawArgumentsIndex, NumInstances);
	}

	inline void DrawIndexedPrimitiveIndirect(uint32_t PrimitiveType, FIndexBufferRHIParamRef IndexBuffer, FVertexBufferRHIParamRef ArgumentsBuffer, uint32_t ArgumentOffset)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIDrawIndexedPrimitiveIndirect)(PrimitiveType, IndexBuffer, ArgumentsBuffer, ArgumentOffset);
			return;
		}
		new (AllocCommand<FRHICommandDrawIndexedPrimitiveIndirect>()) FRHICommandDrawIndexedPrimitiveIndirect(PrimitiveType, IndexBuffer, ArgumentsBuffer, ArgumentOffset);
	}

	inline void SetDepthBounds(float MinDepth, float MaxDepth)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHISetDepthBounds)(MinDepth, MaxDepth);
			return;
		}
		new (AllocCommand<FRHICommandSetDepthBounds>()) FRHICommandSetDepthBounds(MinDepth, MaxDepth);
	}

	inline void CopyToResolveTarget(FTextureRHIParamRef SourceTextureRHI, FTextureRHIParamRef DestTextureRHI, const FResolveParams& ResolveParams)
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHICopyToResolveTarget)(SourceTextureRHI, DestTextureRHI, ResolveParams);
			return;
		}
		new (AllocCommand<FRHICommandCopyToResolveTarget>()) FRHICommandCopyToResolveTarget(SourceTextureRHI, DestTextureRHI, ResolveParams);
	}

	//DEPRECATED(4.20, "This signature of CopyToResolveTarget is deprecated. Please use the new one instead.")
	inline void CopyToResolveTarget(FTextureRHIParamRef SourceTextureRHI, FTextureRHIParamRef DestTextureRHI, bool bKeepOriginalSurface, const FResolveParams& ResolveParams)
	{
		CopyToResolveTarget(SourceTextureRHI, DestTextureRHI, ResolveParams);
	}

	inline void CopyTexture(FTextureRHIParamRef SourceTextureRHI, FTextureRHIParamRef DestTextureRHI, const FRHICopyTextureInfo& CopyInfo)
	{
		assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHICopyTexture)(SourceTextureRHI, DestTextureRHI, CopyInfo);
			return;
		}
		new (AllocCommand<FRHICommandCopyTexture>()) FRHICommandCopyTexture(SourceTextureRHI, DestTextureRHI, CopyInfo);
	}

	inline void ClearTinyUAV(FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI, const uint32_t(&Values)[4])
	{
		//assert(IsOutsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIClearTinyUAV)(UnorderedAccessViewRHI, Values);
			return;
		}
		new (AllocCommand<FRHICommandClearTinyUAV>()) FRHICommandClearTinyUAV(UnorderedAccessViewRHI, Values);
	}

	inline void BeginRenderQuery(FRenderQueryRHIParamRef RenderQuery)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIBeginRenderQuery)(RenderQuery);
			return;
		}
		new (AllocCommand<FRHICommandBeginRenderQuery>()) FRHICommandBeginRenderQuery(RenderQuery);
	}
	inline void EndRenderQuery(FRenderQueryRHIParamRef RenderQuery)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIEndRenderQuery)(RenderQuery);
			return;
		}
		new (AllocCommand<FRHICommandEndRenderQuery>()) FRHICommandEndRenderQuery(RenderQuery);
	}

	inline void SubmitCommandsHint()
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHISubmitCommandsHint)();
			return;
		}
		new (AllocCommand<FRHICommandSubmitCommandsHint<ECmdList::EGfx>>()) FRHICommandSubmitCommandsHint<ECmdList::EGfx>();
	}

	inline void TransitionResource(EResourceTransitionAccess TransitionType, FTextureRHIParamRef InTexture)
	{
		FTextureRHIParamRef Texture = InTexture;
		assert(Texture == nullptr || Texture->IsCommitted());
		if (Bypass())
		{
			CMD_CONTEXT(RHITransitionResources)(TransitionType, &Texture, 1);
			return;
		}

		// Allocate space to hold the single texture pointer inline in the command list itself.
		FTextureRHIParamRef* TextureArray = (FTextureRHIParamRef*)Alloc(sizeof(FTextureRHIParamRef), alignof(FTextureRHIParamRef));
		TextureArray[0] = Texture;
		new (AllocCommand<FRHICommandTransitionTextures>()) FRHICommandTransitionTextures(TransitionType, TextureArray, 1);
	}

	inline void TransitionResources(EResourceTransitionAccess TransitionType, FTextureRHIParamRef* InTextures, int32_t NumTextures)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHITransitionResources)(TransitionType, InTextures, NumTextures);
			return;
		}

		// Allocate space to hold the list of textures inline in the command list itself.
		FTextureRHIParamRef* InlineTextureArray = (FTextureRHIParamRef*)Alloc(sizeof(FTextureRHIParamRef) * NumTextures, alignof(FTextureRHIParamRef));
		for (int32_t Index = 0; Index < NumTextures; ++Index)
		{
			InlineTextureArray[Index] = InTextures[Index];
		}

		new (AllocCommand<FRHICommandTransitionTextures>()) FRHICommandTransitionTextures(TransitionType, InlineTextureArray, NumTextures);
	}

	inline void TransitionResourceArrayNoCopy(EResourceTransitionAccess TransitionType, std::vector<FTextureRHIParamRef>& InTextures)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHITransitionResources)(TransitionType, &InTextures[0], InTextures.size());
			return;
		}
		new (AllocCommand<FRHICommandTransitionTexturesArray>()) FRHICommandTransitionTexturesArray(TransitionType, InTextures);
	}

	inline void TransitionResource(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef InUAV, FComputeFenceRHIParamRef WriteFence)
	{
		assert(InUAV == nullptr || InUAV->IsCommitted());
		FUnorderedAccessViewRHIParamRef UAV = InUAV;
		if (Bypass())
		{
			CMD_CONTEXT(RHITransitionResources)(TransitionType, TransitionPipeline, &UAV, 1, WriteFence);
			return;
		}

		// Allocate space to hold the single UAV pointer inline in the command list itself.
		FUnorderedAccessViewRHIParamRef* UAVArray = (FUnorderedAccessViewRHIParamRef*)Alloc(sizeof(FUnorderedAccessViewRHIParamRef), alignof(FUnorderedAccessViewRHIParamRef));
		UAVArray[0] = UAV;
		new (AllocCommand<FRHICommandTransitionUAVs<ECmdList::EGfx>>()) FRHICommandTransitionUAVs<ECmdList::EGfx>(TransitionType, TransitionPipeline, UAVArray, 1, WriteFence);
	}

	inline void TransitionResource(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef InUAV)
	{
		TransitionResource(TransitionType, TransitionPipeline, InUAV, nullptr);
	}

	inline void TransitionResources(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32_t NumUAVs, FComputeFenceRHIParamRef WriteFence)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHITransitionResources)(TransitionType, TransitionPipeline, InUAVs, NumUAVs, WriteFence);
			return;
		}

		// Allocate space to hold the list UAV pointers inline in the command list itself.
		FUnorderedAccessViewRHIParamRef* UAVArray = (FUnorderedAccessViewRHIParamRef*)Alloc(sizeof(FUnorderedAccessViewRHIParamRef) * NumUAVs, alignof(FUnorderedAccessViewRHIParamRef));
		for (int32_t Index = 0; Index < NumUAVs; ++Index)
		{
			UAVArray[Index] = InUAVs[Index];
		}

		new (AllocCommand<FRHICommandTransitionUAVs<ECmdList::EGfx>>()) FRHICommandTransitionUAVs<ECmdList::EGfx>(TransitionType, TransitionPipeline, UAVArray, NumUAVs, WriteFence);
	}

	inline void TransitionResources(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32_t NumUAVs)
	{
		TransitionResources(TransitionType, TransitionPipeline, InUAVs, NumUAVs, nullptr);
	}

	inline void WaitComputeFence(FComputeFenceRHIParamRef WaitFence)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIWaitComputeFence)(WaitFence);
			return;
		}
		new (AllocCommand<FRHICommandWaitComputeFence<ECmdList::EGfx>>()) FRHICommandWaitComputeFence<ECmdList::EGfx>(WaitFence);
	}

	inline void BeginRenderPass(const FRHIRenderPassInfo& InInfo, const char* Name)
	{
		assert(!IsInsideRenderPass());
		assert(!IsInsideComputePass());

		if (Bypass())
		{
			CMD_CONTEXT(RHIBeginRenderPass)(InInfo, Name);
		}
		else
		{
			char* NameCopy  = AllocString(Name);
			new (AllocCommand<FRHICommandBeginRenderPass>()) FRHICommandBeginRenderPass(InInfo, NameCopy);
		}
		Data.bInsideRenderPass = true;

		CacheActiveRenderTargets(InInfo);
		Data.bInsideRenderPass = true;
	}

	void EndRenderPass()
	{
		assert(IsInsideRenderPass());
		assert(!IsInsideComputePass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIEndRenderPass)();
		}
		else
		{
			new (AllocCommand<FRHICommandEndRenderPass>()) FRHICommandEndRenderPass();
		}
		Data.bInsideRenderPass = false;
	}

	inline void BeginComputePass(const char* Name)
	{
		assert(!IsInsideRenderPass());
		assert(!IsInsideComputePass());

		if (Bypass())
		{
			CMD_CONTEXT(RHIBeginComputePass)(Name);
		}
		else
		{
			char* NameCopy  = AllocString(Name);
			new (AllocCommand<FRHICommandBeginComputePass>()) FRHICommandBeginComputePass(NameCopy);
		}
		Data.bInsideComputePass = true;

		Data.bInsideComputePass = true;
	}

	void EndComputePass()
	{
		assert(IsInsideComputePass());
		assert(!IsInsideRenderPass());
		if (Bypass())
		{
			CMD_CONTEXT(RHIEndComputePass)();
		}
		else
		{
			new (AllocCommand<FRHICommandEndComputePass>()) FRHICommandEndComputePass();
		}
		Data.bInsideComputePass = false;
	}

	// These 6 are special in that they must be called on the immediate command list and they force a flush only when we are not doing RHI thread
	void BeginScene();
	void EndScene();
	void BeginDrawingViewport(FViewportRHIParamRef Viewport, FTextureRHIParamRef RenderTargetRHI);
	void EndDrawingViewport(FViewportRHIParamRef Viewport, bool bPresent, bool bLockToVsync);
	void BeginFrame();
	void EndFrame();

	inline void PushEvent(const char* Name, FColor Color)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIPushEvent)(Name, Color);
			return;
		}
		char* NameCopy  = AllocString(Name);
		new (AllocCommand<FRHICommandPushEvent<ECmdList::EGfx>>()) FRHICommandPushEvent<ECmdList::EGfx>(NameCopy, Color);
	}

	inline void PopEvent()
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIPopEvent)();
			return;
		}
		new (AllocCommand<FRHICommandPopEvent<ECmdList::EGfx>>()) FRHICommandPopEvent<ECmdList::EGfx>();
	}
	
	inline void RHIInvalidateCachedState()
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIInvalidateCachedState)();
			return;
		}
		new (AllocCommand<FRHICommandInvalidateCachedState>()) FRHICommandInvalidateCachedState();
	}

	inline void DiscardRenderTargets(bool Depth, bool Stencil, uint32_t ColorBitMask)
	{
		if (Bypass())
		{
			CMD_CONTEXT(RHIDiscardRenderTargets)(Depth, Stencil, ColorBitMask);
			return;
		}
		new (AllocCommand<FRHICommandDiscardRenderTargets>()) FRHICommandDiscardRenderTargets(Depth, Stencil, ColorBitMask);
	}
	
	inline void BreakPoint()
	{
#if !UE_BUILD_SHIPPING
		if (Bypass())
		{
		/*	if (FPlatformMisc::IsDebuggerPresent())
			{
				UE_DEBUG_BREAK();
			}*/
			return;
		}
		new (AllocCommand<FRHICommandDebugBreak>()) FRHICommandDebugBreak();
#endif
	}
};

class  FRHIAsyncComputeCommandList : public FRHICommandListBase
{
public:

	FRHIAsyncComputeCommandList() : FRHICommandListBase(FRHIGPUMask::All()) {}

	/** Custom new/delete with recycling */
	void* operator new(size_t Size);
	void operator delete(void *RawMemory);

	inline void SetShaderUniformBuffer(FComputeShaderRHIParamRef Shader, uint32_t BaseIndex, FUniformBufferRHIParamRef UniformBuffer)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHISetShaderUniformBuffer)(Shader, BaseIndex, UniformBuffer);
			return;
		}
		new (AllocCommand<FRHICommandSetShaderUniformBuffer<FComputeShaderRHIParamRef, ECmdList::ECompute> >()) FRHICommandSetShaderUniformBuffer<FComputeShaderRHIParamRef, ECmdList::ECompute>(Shader, BaseIndex, UniformBuffer);		
	}
	
	inline void SetShaderUniformBuffer(FComputeShaderRHIRef& Shader, uint32_t BaseIndex, FUniformBufferRHIParamRef UniformBuffer)
	{
		SetShaderUniformBuffer(Shader.GetReference(), BaseIndex, UniformBuffer);
	}

	inline void SetShaderParameter(FComputeShaderRHIParamRef Shader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHISetShaderParameter)(Shader, BufferIndex, BaseIndex, NumBytes, NewValue);
			return;
		}
		void* UseValue = Alloc(NumBytes, 16);
		memcpy(UseValue, NewValue, NumBytes);
		new (AllocCommand<FRHICommandSetShaderParameter<FComputeShaderRHIParamRef, ECmdList::ECompute> >()) FRHICommandSetShaderParameter<FComputeShaderRHIParamRef, ECmdList::ECompute>(Shader, BufferIndex, BaseIndex, NumBytes, UseValue);
	}
	
	inline void SetShaderParameter(FComputeShaderRHIRef& Shader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue)
	{
		SetShaderParameter(Shader.GetReference(), BufferIndex, BaseIndex, NumBytes, NewValue);
	}
	
	inline void SetShaderTexture(FComputeShaderRHIParamRef Shader, uint32_t TextureIndex, FTextureRHIParamRef Texture)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHISetShaderTexture)(Shader, TextureIndex, Texture);
			return;
		}
		new (AllocCommand<FRHICommandSetShaderTexture<FComputeShaderRHIParamRef, ECmdList::ECompute> >()) FRHICommandSetShaderTexture<FComputeShaderRHIParamRef, ECmdList::ECompute>(Shader, TextureIndex, Texture);
	}
	
	inline void SetShaderResourceViewParameter(FComputeShaderRHIParamRef Shader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHISetShaderResourceViewParameter)(Shader, SamplerIndex, SRV);
			return;
		}
		new (AllocCommand<FRHICommandSetShaderResourceViewParameter<FComputeShaderRHIParamRef, ECmdList::ECompute> >()) FRHICommandSetShaderResourceViewParameter<FComputeShaderRHIParamRef, ECmdList::ECompute>(Shader, SamplerIndex, SRV);
	}
	
	inline void SetShaderSampler(FComputeShaderRHIParamRef Shader, uint32_t SamplerIndex, FSamplerStateRHIParamRef State)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHISetShaderSampler)(Shader, SamplerIndex, State);
			return;
		}
		new (AllocCommand<FRHICommandSetShaderSampler<FComputeShaderRHIParamRef, ECmdList::ECompute> >()) FRHICommandSetShaderSampler<FComputeShaderRHIParamRef, ECmdList::ECompute>(Shader, SamplerIndex, State);
	}

	inline void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHISetUAVParameter)(Shader, UAVIndex, UAV);
			return;
		}
		new (AllocCommand<FRHICommandSetUAVParameter<FComputeShaderRHIParamRef, ECmdList::ECompute> >()) FRHICommandSetUAVParameter<FComputeShaderRHIParamRef, ECmdList::ECompute>(Shader, UAVIndex, UAV);
	}

	inline void SetUAVParameter(FComputeShaderRHIParamRef Shader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32_t InitialCount)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHISetUAVParameter)(Shader, UAVIndex, UAV, InitialCount);
			return;
		}
		new (AllocCommand<FRHICommandSetUAVParameter_IntialCount<FComputeShaderRHIParamRef, ECmdList::ECompute> >()) FRHICommandSetUAVParameter_IntialCount<FComputeShaderRHIParamRef, ECmdList::ECompute>(Shader, UAVIndex, UAV, InitialCount);
	}
	
	inline void SetComputeShader(FComputeShaderRHIParamRef ComputeShader)
	{
		ComputeShader->UpdateStats();
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHISetComputeShader)(ComputeShader);
			return;
		}
		new (AllocCommand<FRHICommandSetComputeShader<ECmdList::ECompute> >()) FRHICommandSetComputeShader<ECmdList::ECompute>(ComputeShader);
	}

	inline void SetComputePipelineState(FComputePipelineState* ComputePipelineState)
	{
		if (Bypass())
		{
			extern FRHIComputePipelineState* ExecuteSetComputePipelineState(FComputePipelineState* ComputePipelineState);
			FRHIComputePipelineState* RHIComputePipelineState = ExecuteSetComputePipelineState(ComputePipelineState);
			COMPUTE_CONTEXT(RHISetComputePipelineState)(RHIComputePipelineState);
			return;
		}
		new (AllocCommand<FRHICommandSetComputePipelineState<ECmdList::ECompute> >()) FRHICommandSetComputePipelineState<ECmdList::ECompute>(ComputePipelineState);
	}

	inline void SetAsyncComputeBudget(EAsyncComputeBudget Budget)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHISetAsyncComputeBudget)(Budget);
			return;
		}
		new (AllocCommand<FRHICommandSetAsyncComputeBudget<ECmdList::ECompute>>()) FRHICommandSetAsyncComputeBudget<ECmdList::ECompute>(Budget);
	}

	inline void DispatchComputeShader(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHIDispatchComputeShader)(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
			return;
		}
		new (AllocCommand<FRHICommandDispatchComputeShader<ECmdList::ECompute> >()) FRHICommandDispatchComputeShader<ECmdList::ECompute>(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	inline void DispatchIndirectComputeShader(FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHIDispatchIndirectComputeShader)(ArgumentBuffer, ArgumentOffset);
			return;
		}
		new (AllocCommand<FRHICommandDispatchIndirectComputeShader<ECmdList::ECompute> >()) FRHICommandDispatchIndirectComputeShader<ECmdList::ECompute>(ArgumentBuffer, ArgumentOffset);
	}

	inline void TransitionResource(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef InUAV, FComputeFenceRHIParamRef WriteFence)
	{
		FUnorderedAccessViewRHIParamRef UAV = InUAV;
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHITransitionResources)(TransitionType, TransitionPipeline, &UAV, 1, WriteFence);
			return;
		}

		// Allocate space to hold the single UAV pointer inline in the command list itself.
		FUnorderedAccessViewRHIParamRef* UAVArray = (FUnorderedAccessViewRHIParamRef*)Alloc(sizeof(FUnorderedAccessViewRHIParamRef), alignof(FUnorderedAccessViewRHIParamRef));
		UAVArray[0] = UAV;
		new (AllocCommand<FRHICommandTransitionUAVs<ECmdList::ECompute> >()) FRHICommandTransitionUAVs<ECmdList::ECompute>(TransitionType, TransitionPipeline, UAVArray, 1, WriteFence);
	}

	inline void TransitionResource(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef InUAV)
	{
		TransitionResource(TransitionType, TransitionPipeline, InUAV, nullptr);
	}

	inline void TransitionResources(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32_t NumUAVs, FComputeFenceRHIParamRef WriteFence)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHITransitionResources)(TransitionType, TransitionPipeline, InUAVs, NumUAVs, WriteFence);
			return;
		}

		// Allocate space to hold the list UAV pointers inline in the command list itself.
		FUnorderedAccessViewRHIParamRef* UAVArray = (FUnorderedAccessViewRHIParamRef*)Alloc(sizeof(FUnorderedAccessViewRHIParamRef) * NumUAVs, alignof(FUnorderedAccessViewRHIParamRef));
		for (int32_t Index = 0; Index < NumUAVs; ++Index)
		{
			UAVArray[Index] = InUAVs[Index];
		}

		new (AllocCommand<FRHICommandTransitionUAVs<ECmdList::ECompute> >()) FRHICommandTransitionUAVs<ECmdList::ECompute>(TransitionType, TransitionPipeline, UAVArray, NumUAVs, WriteFence);
	}

	inline void TransitionResources(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32_t NumUAVs)
	{
		TransitionResources(TransitionType, TransitionPipeline, InUAVs, NumUAVs, nullptr);
	}
	
	inline void PushEvent(const char* Name, FColor Color)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHIPushEvent)(Name, Color);
			return;
		}
		char* NameCopy  = AllocString(Name);
		new (AllocCommand<FRHICommandPushEvent<ECmdList::ECompute> >()) FRHICommandPushEvent<ECmdList::ECompute>(NameCopy, Color);
	}

	inline void PopEvent()
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHIPopEvent)();
			return;
		}
		new (AllocCommand<FRHICommandPopEvent<ECmdList::ECompute> >()) FRHICommandPopEvent<ECmdList::ECompute>();
	}

	inline void BreakPoint()
	{
#if !UE_BUILD_SHIPPING
		if (Bypass())
		{
		/*	if (FPlatformMisc::IsDebuggerPresent())
			{
				UE_DEBUG_BREAK();
			}*/
			return;
		}
		new (AllocCommand<FRHICommandDebugBreak>()) FRHICommandDebugBreak();
#endif
	}

	inline void SubmitCommandsHint()
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHISubmitCommandsHint)();
			return;
		}
		new (AllocCommand<FRHICommandSubmitCommandsHint<ECmdList::ECompute>>()) FRHICommandSubmitCommandsHint<ECmdList::ECompute>();
	}

	inline void WaitComputeFence(FComputeFenceRHIParamRef WaitFence)
	{
		if (Bypass())
		{
			COMPUTE_CONTEXT(RHIWaitComputeFence)(WaitFence);
			return;
		}
		new (AllocCommand<FRHICommandWaitComputeFence<ECmdList::ECompute>>()) FRHICommandWaitComputeFence<ECmdList::ECompute>(WaitFence);
	}
};

namespace EImmediateFlushType
{
	enum Type
	{ 
		WaitForOutstandingTasksOnly = 0, 
		DispatchToRHIThread, 
		WaitForDispatchToRHIThread,
		FlushRHIThread,
		FlushRHIThreadFlushResources,
		FlushRHIThreadFlushResourcesFlushDeferredDeletes
	};
};

class FScopedRHIThreadStaller
{
	class FRHICommandListImmediate* Immed; // non-null if we need to unstall
public:
	FScopedRHIThreadStaller(class FRHICommandListImmediate& InImmed);
	~FScopedRHIThreadStaller();
};

class  FRHICommandListImmediate : public FRHICommandList
{
	template <typename LAMBDA>
	struct TRHILambdaCommand : public FRHICommandBase
	{
		LAMBDA Lambda;

		TRHILambdaCommand(LAMBDA&& InLambda)
			: Lambda(Forward<LAMBDA>(InLambda))
		{}

		void ExecuteAndDestruct(FRHICommandListBase& CmdList, FRHICommandListDebugContext&) override final
		{
			Lambda(*static_cast<FRHICommandListImmediate*>(&CmdList));
			Lambda.~LAMBDA();
		}
	};

	friend class FRHICommandListExecutor;
	FRHICommandListImmediate()
		: FRHICommandList(FRHIGPUMask::All())
	{
		Data.Type = FRHICommandListBase::FCommonData::ECmdListType::Immediate;
	}
	~FRHICommandListImmediate()
	{
		assert(!HasCommands());
	}
public:

	void ImmediateFlush(EImmediateFlushType::Type FlushType);
	bool StallRHIThread();
	void UnStallRHIThread();
	static bool IsStalled();

	//void SetCurrentStat(TStatId Stat);

	/** Dispatch current work and change the GPUMask. */
	void SetGPUMask(FRHIGPUMask InGPUMask);

	static FGraphEventRef RenderThreadTaskFence();
	static FGraphEventArray& GetRenderThreadTaskArray();
	static void WaitOnRenderThreadTaskFence(FGraphEventRef& Fence);
	static bool AnyRenderThreadTasksOutstanding();
	FGraphEventRef RHIThreadFence(bool bSetLockFence = false);

	//Queue the given async compute commandlists in order with the current immediate commandlist
	void QueueAsyncCompute(FRHIAsyncComputeCommandList& RHIComputeCmdList);

	template <typename LAMBDA>
	inline bool EnqueueLambda(bool bRunOnCurrentThread, LAMBDA&& Lambda)
	{
		if (bRunOnCurrentThread)
		{
			Lambda(*this);
			return false;
		}
		else
		{
			new (AllocCommand<TRHILambdaCommand<LAMBDA>>()) TRHILambdaCommand<LAMBDA>(Forward<LAMBDA>(Lambda));
			return true;
		}
	}

	template <typename LAMBDA>
	inline bool EnqueueLambda(LAMBDA&& Lambda)
	{
		return EnqueueLambda(Bypass(), Forward<LAMBDA>(Lambda));
	}

	inline FSamplerStateRHIRef CreateSamplerState(const FSamplerStateInitializerRHI& Initializer)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return RHICreateSamplerState(Initializer);
	}
	
	inline FRasterizerStateRHIRef CreateRasterizerState(const FRasterizerStateInitializerRHI& Initializer)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return RHICreateRasterizerState(Initializer);
	}
	
	inline FDepthStencilStateRHIRef CreateDepthStencilState(const FDepthStencilStateInitializerRHI& Initializer)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return RHICreateDepthStencilState(Initializer);
	}
	
	inline FBlendStateRHIRef CreateBlendState(const FBlendStateInitializerRHI& Initializer)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return RHICreateBlendState(Initializer);
	}
	
	inline FVertexDeclarationRHIRef CreateVertexDeclaration(const FVertexDeclarationElementList& Elements)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateVertexDeclaration_RenderThread(*this, Elements);
	}
	
	inline FPixelShaderRHIRef CreatePixelShader(const std::vector<uint8_t>& Code)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreatePixelShader_RenderThread(*this, Code);
	}
	
	inline FPixelShaderRHIRef CreatePixelShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreatePixelShader_RenderThread(*this, Library, Hash);
	}
	
	inline FVertexShaderRHIRef CreateVertexShader(const std::vector<uint8_t>& Code)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateVertexShader_RenderThread(*this, Code);
	}
	
	inline FVertexShaderRHIRef CreateVertexShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateVertexShader_RenderThread(*this, Library, Hash);
	}
	
	inline FHullShaderRHIRef CreateHullShader(const std::vector<uint8_t>& Code)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateHullShader_RenderThread(*this, Code);
	}
	
	inline FHullShaderRHIRef CreateHullShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateHullShader_RenderThread(*this, Library, Hash);
	}
	
	inline FDomainShaderRHIRef CreateDomainShader(const std::vector<uint8_t>& Code)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateDomainShader_RenderThread(*this, Code);
	}
	
	inline FDomainShaderRHIRef CreateDomainShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateDomainShader_RenderThread(*this, Library, Hash);
	}
	
	inline FGeometryShaderRHIRef CreateGeometryShader(const std::vector<uint8_t>& Code)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateGeometryShader_RenderThread(*this, Code);
	}
	
	inline FGeometryShaderRHIRef CreateGeometryShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateGeometryShader_RenderThread(*this, Library, Hash);
	}
	
	inline FGeometryShaderRHIRef CreateGeometryShaderWithStreamOutput(const std::vector<uint8_t>& Code, const FStreamOutElementList& ElementList, uint32_t NumStrides, const uint32_t* Strides, int32_t RasterizedStream)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateGeometryShaderWithStreamOutput_RenderThread(*this, Code, ElementList, NumStrides, Strides, RasterizedStream);
	}
	
	inline FGeometryShaderRHIRef CreateGeometryShaderWithStreamOutput(const FStreamOutElementList& ElementList, uint32_t NumStrides, const uint32_t* Strides, int32_t RasterizedStream, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateGeometryShaderWithStreamOutput_RenderThread(*this, ElementList, NumStrides, Strides, RasterizedStream, Library, Hash);
	}
	
	inline FComputeShaderRHIRef CreateComputeShader(const std::vector<uint8_t>& Code)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateComputeShader_RenderThread(*this, Code);
	}
	
	inline FComputeShaderRHIRef CreateComputeShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return GDynamicRHI->CreateComputeShader_RenderThread(*this, Library, Hash);
	}
	
	inline FComputeFenceRHIRef CreateComputeFence(const std::string& Name)
	{		
		return GDynamicRHI->RHICreateComputeFence(Name);
	}	

	inline FGPUFenceRHIRef CreateGPUFence(const std::string& Name)
	{
		return GDynamicRHI->RHICreateGPUFence(Name);
	}

	inline FStagingBufferRHIRef CreateStagingBuffer()
	{
		return GDynamicRHI->RHICreateStagingBuffer();
	}

	inline FBoundShaderStateRHIRef CreateBoundShaderState(FVertexDeclarationRHIParamRef VertexDeclaration, FVertexShaderRHIParamRef VertexShader, FHullShaderRHIParamRef HullShader, FDomainShaderRHIParamRef DomainShader, FPixelShaderRHIParamRef PixelShader, FGeometryShaderRHIParamRef GeometryShader)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return RHICreateBoundShaderState(VertexDeclaration, VertexShader, HullShader, DomainShader, PixelShader, GeometryShader);
	}

	inline FGraphicsPipelineStateRHIRef CreateGraphicsPipelineState(const FGraphicsPipelineStateInitializer& Initializer)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return RHICreateGraphicsPipelineState(Initializer);
	}

	inline TRefCountPtr<FRHIComputePipelineState> CreateComputePipelineState(FRHIComputeShader* ComputeShader)
	{
		LLM_SCOPE(ELLMTag::Shaders);
		return RHICreateComputePipelineState(ComputeShader);
	}

	inline FUniformBufferRHIRef CreateUniformBuffer(const void* Contents, const FRHIUniformBufferLayout& Layout, EUniformBufferUsage Usage)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return RHICreateUniformBuffer(Contents, Layout, Usage);
	}
	
	inline FIndexBufferRHIRef CreateAndLockIndexBuffer(uint32_t Stride, uint32_t Size, uint32_t InUsage, FRHIResourceCreateInfo& CreateInfo, void*& OutDataBuffer)
	{
		LLM_SCOPE(ELLMTag::Meshes);
		return GDynamicRHI->CreateAndLockIndexBuffer_RenderThread(*this, Stride, Size, InUsage, CreateInfo, OutDataBuffer);
	}
	
	inline FIndexBufferRHIRef CreateIndexBuffer(uint32_t Stride, uint32_t Size, uint32_t InUsage, FRHIResourceCreateInfo& CreateInfo)
	{
		LLM_SCOPE(ELLMTag::Meshes);
		return GDynamicRHI->CreateIndexBuffer_RenderThread(*this, Stride, Size, InUsage, CreateInfo);
	}
	
	inline void* LockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer, uint32_t Offset, uint32_t SizeRHI, EResourceLockMode LockMode)
	{
		LLM_SCOPE(ELLMTag::Meshes);
		return GDynamicRHI->LockIndexBuffer_RenderThread(*this, IndexBuffer, Offset, SizeRHI, LockMode);
	}
	
	inline void UnlockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer)
	{
		GDynamicRHI->UnlockIndexBuffer_RenderThread(*this, IndexBuffer);
	}
	
	inline FVertexBufferRHIRef CreateAndLockVertexBuffer(uint32_t Size, uint32_t InUsage, FRHIResourceCreateInfo& CreateInfo, void*& OutDataBuffer)
	{
		LLM_SCOPE(ELLMTag::Meshes);
		return GDynamicRHI->CreateAndLockVertexBuffer_RenderThread(*this, Size, InUsage, CreateInfo, OutDataBuffer);
	}

	inline FVertexBufferRHIRef CreateVertexBuffer(uint32_t Size, uint32_t InUsage, FRHIResourceCreateInfo& CreateInfo)
	{
		LLM_SCOPE(ELLMTag::Meshes);
		return GDynamicRHI->CreateVertexBuffer_RenderThread(*this, Size, InUsage, CreateInfo);
	}
	
	inline void* LockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer, uint32_t Offset, uint32_t SizeRHI, EResourceLockMode LockMode)
	{
		LLM_SCOPE(ELLMTag::Meshes);
		return GDynamicRHI->LockVertexBuffer_RenderThread(*this, VertexBuffer, Offset, SizeRHI, LockMode);
	}
	
	inline void UnlockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer)
	{
		GDynamicRHI->UnlockVertexBuffer_RenderThread(*this, VertexBuffer);
	}
	
	inline void CopyVertexBuffer(FVertexBufferRHIParamRef SourceBuffer,FVertexBufferRHIParamRef DestBuffer)
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_CopyVertexBuffer_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);  
		GDynamicRHI->RHICopyVertexBuffer(SourceBuffer,DestBuffer);
	}

	inline FStructuredBufferRHIRef CreateStructuredBuffer(uint32_t Stride, uint32_t Size, uint32_t InUsage, FRHIResourceCreateInfo& CreateInfo)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->CreateStructuredBuffer_RenderThread(*this, Stride, Size, InUsage, CreateInfo);
	}
	
	inline void* LockStructuredBuffer(FStructuredBufferRHIParamRef StructuredBuffer, uint32_t Offset, uint32_t SizeRHI, EResourceLockMode LockMode)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_LockStructuredBuffer_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		 
		return GDynamicRHI->RHILockStructuredBuffer(StructuredBuffer, Offset, SizeRHI, LockMode);
	}
	
	inline void UnlockStructuredBuffer(FStructuredBufferRHIParamRef StructuredBuffer)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_UnlockStructuredBuffer_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		  
		GDynamicRHI->RHIUnlockStructuredBuffer(StructuredBuffer);
	}
	
	inline FUnorderedAccessViewRHIRef CreateUnorderedAccessView(FStructuredBufferRHIParamRef StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->RHICreateUnorderedAccessView_RenderThread(*this, StructuredBuffer, bUseUAVCounter, bAppendBuffer);
	}
	
	inline FUnorderedAccessViewRHIRef CreateUnorderedAccessView(FTextureRHIParamRef Texture, uint32_t MipLevel)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->RHICreateUnorderedAccessView_RenderThread(*this, Texture, MipLevel);
	}
	
	inline FUnorderedAccessViewRHIRef CreateUnorderedAccessView(FVertexBufferRHIParamRef VertexBuffer, uint8_t Format)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->RHICreateUnorderedAccessView_RenderThread(*this, VertexBuffer, Format);
	}

	inline FUnorderedAccessViewRHIRef CreateUnorderedAccessView(FIndexBufferRHIParamRef IndexBuffer, uint8_t Format)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->RHICreateUnorderedAccessView_RenderThread(*this, IndexBuffer, Format);
	}

	inline FShaderResourceViewRHIRef CreateShaderResourceView(FStructuredBufferRHIParamRef StructuredBuffer)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->RHICreateShaderResourceView_RenderThread(*this, StructuredBuffer);
	}
	
	inline FShaderResourceViewRHIRef CreateShaderResourceView(FVertexBufferRHIParamRef VertexBuffer, uint32_t Stride, uint8_t Format)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->CreateShaderResourceView_RenderThread(*this, VertexBuffer, Stride, Format);
	}
	
	inline FShaderResourceViewRHIRef CreateShaderResourceView(FIndexBufferRHIParamRef Buffer)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->CreateShaderResourceView_RenderThread(*this, Buffer);
	}
	
	inline uint64_t CalcTexture2DPlatformSize(uint32_t SizeX, uint32_t SizeY, uint8_t Format, uint32_t NumMips, uint32_t NumSamples, uint32_t Flags, uint32_t& OutAlign)
	{
		return RHICalcTexture2DPlatformSize(SizeX, SizeY, Format, NumMips, NumSamples, Flags, OutAlign);
	}
	
	inline uint64_t CalcTexture3DPlatformSize(uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ, uint8_t Format, uint32_t NumMips, uint32_t Flags, uint32_t& OutAlign)
	{
		return RHICalcTexture3DPlatformSize(SizeX, SizeY, SizeZ, Format, NumMips, Flags, OutAlign);
	}
	
	inline uint64_t CalcTextureCubePlatformSize(uint32_t Size, uint8_t Format, uint32_t NumMips, uint32_t Flags, uint32_t& OutAlign)
	{
		return RHICalcTextureCubePlatformSize(Size, Format, NumMips, Flags, OutAlign);
	}
	
	inline void GetTextureMemoryStats(FTextureMemoryStats& OutStats)
	{
		RHIGetTextureMemoryStats(OutStats);
	}
	
	inline bool GetTextureMemoryVisualizeData(FColor* TextureData,int32_t SizeX,int32_t SizeY,int32_t Pitch,int32_t PixelSize)
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_GetTextureMemoryVisualizeData_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		return GDynamicRHI->RHIGetTextureMemoryVisualizeData(TextureData,SizeX,SizeY,Pitch,PixelSize);
	}
	
	inline FTextureReferenceRHIRef CreateTextureReference(FLastRenderTimeContainer* LastRenderTime)
	{
		LLM_SCOPE(ELLMTag::Textures);
		return GDynamicRHI->RHICreateTextureReference_RenderThread(*this, LastRenderTime);
	}
	
	inline FTexture2DRHIRef CreateTexture2D(uint32_t SizeX, uint32_t SizeY, uint8_t Format, uint32_t NumMips, uint32_t NumSamples, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
	{
		LLM_SCOPE((Flags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable)) != 0 ? ELLMTag::RenderTargets : ELLMTag::Textures);
		return GDynamicRHI->RHICreateTexture2D_RenderThread(*this, SizeX, SizeY, Format, NumMips, NumSamples, Flags, CreateInfo);
	}

	inline FTexture2DRHIRef CreateTextureExternal2D(uint32_t SizeX, uint32_t SizeY, uint8_t Format, uint32_t NumMips, uint32_t NumSamples, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
	{
		return GDynamicRHI->RHICreateTextureExternal2D_RenderThread(*this, SizeX, SizeY, Format, NumMips, NumSamples, Flags, CreateInfo);
	}

	inline FStructuredBufferRHIRef CreateRTWriteMaskBuffer(FTexture2DRHIRef RenderTarget)
	{
		LLM_SCOPE(ELLMTag::RenderTargets);
		return GDynamicRHI->RHICreateRTWriteMaskBuffer(RenderTarget);
	}

	inline FTexture2DRHIRef AsyncCreateTexture2D(uint32_t SizeX, uint32_t SizeY, uint8_t Format, uint32_t NumMips, uint32_t Flags, void** InitialMipData, uint32_t NumInitialMips)
	{
		LLM_SCOPE((Flags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable)) != 0 ? ELLMTag::RenderTargets : ELLMTag::Textures);
		return GDynamicRHI->RHIAsyncCreateTexture2D(SizeX, SizeY, Format, NumMips, Flags, InitialMipData, NumInitialMips);
	}
	
	inline void CopySharedMips(FTexture2DRHIParamRef DestTexture2D, FTexture2DRHIParamRef SrcTexture2D)
	{
		LLM_SCOPE(ELLMTag::Textures);
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_CopySharedMips_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		 
		return GDynamicRHI->RHICopySharedMips(DestTexture2D, SrcTexture2D);
	}
	
	inline FTexture2DArrayRHIRef CreateTexture2DArray(uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ, uint8_t Format, uint32_t NumMips, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
	{
		LLM_SCOPE((Flags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable)) != 0 ? ELLMTag::RenderTargets : ELLMTag::Textures);
		return GDynamicRHI->RHICreateTexture2DArray_RenderThread(*this, SizeX, SizeY, SizeZ, Format, NumMips, Flags, CreateInfo);
	}
	
	inline FTexture3DRHIRef CreateTexture3D(uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ, uint8_t Format, uint32_t NumMips, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
	{
		LLM_SCOPE((Flags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable)) != 0 ? ELLMTag::RenderTargets : ELLMTag::Textures);
		return GDynamicRHI->RHICreateTexture3D_RenderThread(*this, SizeX, SizeY, SizeZ, Format, NumMips, Flags, CreateInfo);
	}
	
	inline void GetResourceInfo(FTextureRHIParamRef Ref, FRHIResourceInfo& OutInfo)
	{
		return RHIGetResourceInfo(Ref, OutInfo);
	}
	
	inline FShaderResourceViewRHIRef CreateShaderResourceView(FTexture2DRHIParamRef Texture2DRHI, uint8_t MipLevel)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->RHICreateShaderResourceView_RenderThread(*this, Texture2DRHI, MipLevel);
	}
	
	inline FShaderResourceViewRHIRef CreateShaderResourceView(FTexture2DRHIParamRef Texture2DRHI, uint8_t MipLevel, uint8_t NumMipLevels, uint8_t Format)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->RHICreateShaderResourceView_RenderThread(*this, Texture2DRHI, MipLevel, NumMipLevels, Format);
	}
	
	inline FShaderResourceViewRHIRef CreateShaderResourceView(FTexture3DRHIParamRef Texture3DRHI, uint8_t MipLevel)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->RHICreateShaderResourceView_RenderThread(*this, Texture3DRHI, MipLevel);
	}

	inline FShaderResourceViewRHIRef CreateShaderResourceView(FTexture2DArrayRHIParamRef Texture2DArrayRHI, uint8_t MipLevel)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->RHICreateShaderResourceView_RenderThread(*this, Texture2DArrayRHI, MipLevel);
	}

	inline FShaderResourceViewRHIRef CreateShaderResourceView(FTextureCubeRHIParamRef TextureCubeRHI, uint8_t MipLevel)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		return GDynamicRHI->RHICreateShaderResourceView_RenderThread(*this, TextureCubeRHI, MipLevel);
	}

	inline void GenerateMips(FTextureRHIParamRef Texture)
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_GenerateMips_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread); return GDynamicRHI->RHIGenerateMips(Texture);
	}
	
	inline uint32_t ComputeMemorySize(FTextureRHIParamRef TextureRHI)
	{
		return RHIComputeMemorySize(TextureRHI);
	}
	
	inline FTexture2DRHIRef AsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, int32_t NewMipCount, int32_t NewSizeX, int32_t NewSizeY, FThreadSafeCounter* RequestStatus)
	{
		LLM_SCOPE(ELLMTag::Textures);
		return GDynamicRHI->AsyncReallocateTexture2D_RenderThread(*this, Texture2D, NewMipCount, NewSizeX, NewSizeY, RequestStatus);
	}
	
	inline ETextureReallocationStatus FinalizeAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted)
	{
		LLM_SCOPE(ELLMTag::Textures);
		return GDynamicRHI->FinalizeAsyncReallocateTexture2D_RenderThread(*this, Texture2D, bBlockUntilCompleted);
	}
	
	inline ETextureReallocationStatus CancelAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted)
	{
		return GDynamicRHI->CancelAsyncReallocateTexture2D_RenderThread(*this, Texture2D, bBlockUntilCompleted);
	}
	
	inline void* LockTexture2D(FTexture2DRHIParamRef Texture, uint32_t MipIndex, EResourceLockMode LockMode, uint32_t& DestStride, bool bLockWithinMiptail, bool bFlushRHIThread = true)
	{
		LLM_SCOPE(ELLMTag::Textures);
		return GDynamicRHI->LockTexture2D_RenderThread(*this, Texture, MipIndex, LockMode, DestStride, bLockWithinMiptail, bFlushRHIThread);
	}
	
	inline void UnlockTexture2D(FTexture2DRHIParamRef Texture, uint32_t MipIndex, bool bLockWithinMiptail, bool bFlushRHIThread = true)
	{		
		GDynamicRHI->UnlockTexture2D_RenderThread(*this, Texture, MipIndex, bLockWithinMiptail, bFlushRHIThread);
	}
	
	inline void* LockTexture2DArray(FTexture2DArrayRHIParamRef Texture, uint32_t TextureIndex, uint32_t MipIndex, EResourceLockMode LockMode, uint32_t& DestStride, bool bLockWithinMiptail)
	{
		LLM_SCOPE(ELLMTag::Textures);
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_LockTexture2DArray_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);  
		return GDynamicRHI->RHILockTexture2DArray(Texture, TextureIndex, MipIndex, LockMode, DestStride, bLockWithinMiptail);
	}
	
	inline void UnlockTexture2DArray(FTexture2DArrayRHIParamRef Texture, uint32_t TextureIndex, uint32_t MipIndex, bool bLockWithinMiptail)
	{
		LLM_SCOPE(ELLMTag::Textures);
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_UnlockTexture2DArray_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);   
		GDynamicRHI->RHIUnlockTexture2DArray(Texture, TextureIndex, MipIndex, bLockWithinMiptail);
	}
	
	inline void UpdateTexture2D(FTexture2DRHIParamRef Texture, uint32_t MipIndex, const struct FUpdateTextureRegion2D& UpdateRegion, uint32_t SourcePitch, const uint8_t* SourceData)
	{		
		checkf(UpdateRegion.DestX + UpdateRegion.Width <= Texture->GetSizeX(), TEXT("UpdateTexture2D out of bounds on X. Texture: %s, %i, %i, %i"), *Texture->GetName().ToString(), UpdateRegion.DestX, UpdateRegion.Width, Texture->GetSizeX());
		checkf(UpdateRegion.DestY + UpdateRegion.Height <= Texture->GetSizeY(), TEXT("UpdateTexture2D out of bounds on Y. Texture: %s, %i, %i, %i"), *Texture->GetName().ToString(), UpdateRegion.DestY, UpdateRegion.Height, Texture->GetSizeY());
		LLM_SCOPE(ELLMTag::Textures);
		GDynamicRHI->UpdateTexture2D_RenderThread(*this, Texture, MipIndex, UpdateRegion, SourcePitch, SourceData);
	}

	inline FUpdateTexture3DData BeginUpdateTexture3D(FTexture3DRHIParamRef Texture, uint32_t MipIndex, const struct FUpdateTextureRegion3D& UpdateRegion)
	{
		checkf(UpdateRegion.DestX + UpdateRegion.Width <= Texture->GetSizeX(), TEXT("UpdateTexture3D out of bounds on X. Texture: %s, %i, %i, %i"), *Texture->GetName().ToString(), UpdateRegion.DestX, UpdateRegion.Width, Texture->GetSizeX());
		checkf(UpdateRegion.DestY + UpdateRegion.Height <= Texture->GetSizeY(), TEXT("UpdateTexture3D out of bounds on Y. Texture: %s, %i, %i, %i"), *Texture->GetName().ToString(), UpdateRegion.DestY, UpdateRegion.Height, Texture->GetSizeY());
		checkf(UpdateRegion.DestZ + UpdateRegion.Depth <= Texture->GetSizeZ(), TEXT("UpdateTexture3D out of bounds on Z. Texture: %s, %i, %i, %i"), *Texture->GetName().ToString(), UpdateRegion.DestZ, UpdateRegion.Depth, Texture->GetSizeZ());
		LLM_SCOPE(ELLMTag::Textures);
		return GDynamicRHI->BeginUpdateTexture3D_RenderThread(*this, Texture, MipIndex, UpdateRegion);
	}

	inline void EndUpdateTexture3D(FUpdateTexture3DData& UpdateData)
	{		
		LLM_SCOPE(ELLMTag::Textures);
		GDynamicRHI->EndUpdateTexture3D_RenderThread(*this, UpdateData);
	}
	
	inline void UpdateTexture3D(FTexture3DRHIParamRef Texture, uint32_t MipIndex, const struct FUpdateTextureRegion3D& UpdateRegion, uint32_t SourceRowPitch, uint32_t SourceDepthPitch, const uint8_t* SourceData)
	{
		checkf(UpdateRegion.DestX + UpdateRegion.Width <= Texture->GetSizeX(), TEXT("UpdateTexture3D out of bounds on X. Texture: %s, %i, %i, %i"), *Texture->GetName().ToString(), UpdateRegion.DestX, UpdateRegion.Width, Texture->GetSizeX());
		checkf(UpdateRegion.DestY + UpdateRegion.Height <= Texture->GetSizeY(), TEXT("UpdateTexture3D out of bounds on Y. Texture: %s, %i, %i, %i"), *Texture->GetName().ToString(), UpdateRegion.DestY, UpdateRegion.Height, Texture->GetSizeY());
		checkf(UpdateRegion.DestZ + UpdateRegion.Depth <= Texture->GetSizeZ(), TEXT("UpdateTexture3D out of bounds on Z. Texture: %s, %i, %i, %i"), *Texture->GetName().ToString(), UpdateRegion.DestZ, UpdateRegion.Depth, Texture->GetSizeZ());
		LLM_SCOPE(ELLMTag::Textures);
		GDynamicRHI->UpdateTexture3D_RenderThread(*this, Texture, MipIndex, UpdateRegion, SourceRowPitch, SourceDepthPitch, SourceData);
	}
	
	inline FTextureCubeRHIRef CreateTextureCube(uint32_t Size, uint8_t Format, uint32_t NumMips, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
	{
		LLM_SCOPE((Flags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable)) != 0 ? ELLMTag::RenderTargets : ELLMTag::Textures);
		return GDynamicRHI->RHICreateTextureCube_RenderThread(*this, Size, Format, NumMips, Flags, CreateInfo);
	}
	
	inline FTextureCubeRHIRef CreateTextureCubeArray(uint32_t Size, uint32_t ArraySize, uint8_t Format, uint32_t NumMips, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
	{
		LLM_SCOPE((Flags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable)) != 0 ? ELLMTag::RenderTargets : ELLMTag::Textures);
		return GDynamicRHI->RHICreateTextureCubeArray_RenderThread(*this, Size, ArraySize, Format, NumMips, Flags, CreateInfo);
	}
	
	inline void* LockTextureCubeFace(FTextureCubeRHIParamRef Texture, uint32_t FaceIndex, uint32_t ArrayIndex, uint32_t MipIndex, EResourceLockMode LockMode, uint32_t& DestStride, bool bLockWithinMiptail)
	{
		LLM_SCOPE(ELLMTag::Textures);
		return GDynamicRHI->RHILockTextureCubeFace_RenderThread(*this, Texture, FaceIndex, ArrayIndex, MipIndex, LockMode, DestStride, bLockWithinMiptail);
	}
	
	inline void UnlockTextureCubeFace(FTextureCubeRHIParamRef Texture, uint32_t FaceIndex, uint32_t ArrayIndex, uint32_t MipIndex, bool bLockWithinMiptail)
	{
		LLM_SCOPE(ELLMTag::Textures);
		GDynamicRHI->RHIUnlockTextureCubeFace_RenderThread(*this, Texture, FaceIndex, ArrayIndex, MipIndex, bLockWithinMiptail);
	}
	
	inline void BindDebugLabelName(FTextureRHIParamRef Texture, const char* Name)
	{
		RHIBindDebugLabelName(Texture, Name);
	}

	inline void BindDebugLabelName(FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI, const char* Name)
	{
		RHIBindDebugLabelName(UnorderedAccessViewRHI, Name);
	}

	inline void ReadSurfaceData(FTextureRHIParamRef Texture,FIntRect Rect,std::vector<FColor>& OutData,FReadSurfaceDataFlags InFlags)
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_ReadSurfaceData_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);  
		GDynamicRHI->RHIReadSurfaceData(Texture,Rect,OutData,InFlags);
	}

	inline void ReadSurfaceData(FTextureRHIParamRef Texture, FIntRect Rect, std::vector<FLinearColor>& OutData, FReadSurfaceDataFlags InFlags)
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_ReadSurfaceData_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);
		GDynamicRHI->RHIReadSurfaceData(Texture, Rect, OutData, InFlags);
	}
	
	inline void MapStagingSurface(FTextureRHIParamRef Texture,void*& OutData,int32_t& OutWidth,int32_t& OutHeight)
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_MapStagingSurface_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);  
		GDynamicRHI->RHIMapStagingSurface(Texture,OutData,OutWidth,OutHeight);
	}
	
	inline void UnmapStagingSurface(FTextureRHIParamRef Texture)
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_UnmapStagingSurface_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);  
		GDynamicRHI->RHIUnmapStagingSurface(Texture);
	}
	
	inline void ReadSurfaceFloatData(FTextureRHIParamRef Texture,FIntRect Rect,std::vector<FFloat16Color>& OutData,ECubeFace CubeFace,int32_t ArrayIndex,int32_t MipIndex)
	{
		LLM_SCOPE(ELLMTag::Textures);
		GDynamicRHI->RHIReadSurfaceFloatData_RenderThread(*this, Texture,Rect,OutData,CubeFace,ArrayIndex,MipIndex);
	}

	inline void Read3DSurfaceFloatData(FTextureRHIParamRef Texture,FIntRect Rect,FIntPoint ZMinMax,std::vector<FFloat16Color>& OutData)
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_Read3DSurfaceFloatData_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);  
		GDynamicRHI->RHIRead3DSurfaceFloatData(Texture,Rect,ZMinMax,OutData);
	}
	
	inline FRenderQueryRHIRef CreateRenderQuery(ERenderQueryType QueryType)
	{
		FScopedRHIThreadStaller StallRHIThread(*this);
		return GDynamicRHI->RHICreateRenderQuery(QueryType);
	}

	inline FRenderQueryRHIRef CreateRenderQuery_RenderThread(ERenderQueryType QueryType)
	{
		return GDynamicRHI->RHICreateRenderQuery_RenderThread(*this, QueryType);
	}


	inline void AcquireTransientResource_RenderThread(FTextureRHIParamRef Texture)
	{
		if (!Texture->IsCommitted() )
		{
			if (GSupportsTransientResourceAliasing)
			{
				GDynamicRHI->RHIAcquireTransientResource_RenderThread(Texture);
			}
			Texture->SetCommitted(true);
		}
	}

	inline void DiscardTransientResource_RenderThread(FTextureRHIParamRef Texture)
	{
		if (Texture->IsCommitted())
		{
			if (GSupportsTransientResourceAliasing)
			{
				GDynamicRHI->RHIDiscardTransientResource_RenderThread(Texture);
			}
			Texture->SetCommitted(false);
		}
	}

	inline void AcquireTransientResource_RenderThread(FVertexBufferRHIParamRef Buffer)
	{
		if (!Buffer->IsCommitted())
		{
			if (GSupportsTransientResourceAliasing)
			{
				GDynamicRHI->RHIAcquireTransientResource_RenderThread(Buffer);
			}
			Buffer->SetCommitted(true);
		}
	}

	inline void DiscardTransientResource_RenderThread(FVertexBufferRHIParamRef Buffer)
	{
		if (Buffer->IsCommitted())
		{
			if (GSupportsTransientResourceAliasing)
			{
				GDynamicRHI->RHIDiscardTransientResource_RenderThread(Buffer);
			}
			Buffer->SetCommitted(false);
		}
	}

	inline void AcquireTransientResource_RenderThread(FStructuredBufferRHIParamRef Buffer)
	{
		if (!Buffer->IsCommitted())
		{
			if (GSupportsTransientResourceAliasing)
			{
				GDynamicRHI->RHIAcquireTransientResource_RenderThread(Buffer);
			}
			Buffer->SetCommitted(true);
		}
	}

	inline void DiscardTransientResource_RenderThread(FStructuredBufferRHIParamRef Buffer)
	{
		if (Buffer->IsCommitted())
		{
			if (GSupportsTransientResourceAliasing)
			{
				GDynamicRHI->RHIDiscardTransientResource_RenderThread(Buffer);
			}
			Buffer->SetCommitted(false);
		}
	}

	inline bool GetRenderQueryResult(FRenderQueryRHIParamRef RenderQuery, uint64_t& OutResult, bool bWait)
	{
		return RHIGetRenderQueryResult(RenderQuery, OutResult, bWait);
	}
	
	inline FTexture2DRHIRef GetViewportBackBuffer(FViewportRHIParamRef Viewport)
	{
		return RHIGetViewportBackBuffer(Viewport);
	}
	
	inline void AdvanceFrameForGetViewportBackBuffer(FViewportRHIParamRef Viewport)
	{
		return RHIAdvanceFrameForGetViewportBackBuffer(Viewport);
	}
	
	inline void AcquireThreadOwnership()
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_AcquireThreadOwnership_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		 
		return GDynamicRHI->RHIAcquireThreadOwnership();
	}
	
	inline void ReleaseThreadOwnership()
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_ReleaseThreadOwnership_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		 
		return GDynamicRHI->RHIReleaseThreadOwnership();
	}
	
	inline void FlushResources()
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_FlushResources_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		 
		return GDynamicRHI->RHIFlushResources();
	}
	
	inline uint32_t GetGPUFrameCycles()
	{
		return RHIGetGPUFrameCycles();
	}
	
	inline FViewportRHIRef CreateViewport(void* WindowHandle, uint32_t SizeX, uint32_t SizeY, bool bIsFullscreen, EPixelFormat PreferredPixelFormat)
	{
		LLM_SCOPE(ELLMTag::RenderTargets);
		return RHICreateViewport(WindowHandle, SizeX, SizeY, bIsFullscreen, PreferredPixelFormat);
	}
	
	inline void ResizeViewport(FViewportRHIParamRef Viewport, uint32_t SizeX, uint32_t SizeY, bool bIsFullscreen, EPixelFormat PreferredPixelFormat)
	{
		LLM_SCOPE(ELLMTag::RenderTargets);
		RHIResizeViewport(Viewport, SizeX, SizeY, bIsFullscreen, PreferredPixelFormat);
	}
	
	inline void Tick(float DeltaTime)
	{
		LLM_SCOPE(ELLMTag::RHIMisc);
		RHITick(DeltaTime);
	}
	
	inline void SetStreamOutTargets(uint32_t NumTargets,const FVertexBufferRHIParamRef* VertexBuffers,const uint32_t* Offsets)
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_SetStreamOutTargets_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);  
		GDynamicRHI->RHISetStreamOutTargets(NumTargets,VertexBuffers,Offsets);
	}
	
	inline void BlockUntilGPUIdle()
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_BlockUntilGPUIdle_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);  
		GDynamicRHI->RHIBlockUntilGPUIdle();
	}

	inline void SubmitCommandsAndFlushGPU()
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_SubmitCommandsAndFlushGPU_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread);
		GDynamicRHI->RHISubmitCommandsAndFlushGPU();
	}
	
	inline void SuspendRendering()
	{
		RHISuspendRendering();
	}
	
	inline void ResumeRendering()
	{
		RHIResumeRendering();
	}
	
	inline bool IsRenderingSuspended()
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_IsRenderingSuspended_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		return GDynamicRHI->RHIIsRenderingSuspended();
	}

	inline bool EnqueueDecompress(uint8_t* SrcBuffer, uint8_t* DestBuffer, int CompressedSize, void* ErrorCodeBuffer)
	{
		return GDynamicRHI->RHIEnqueueDecompress(SrcBuffer, DestBuffer, CompressedSize, ErrorCodeBuffer);
	}

	inline bool EnqueueCompress(uint8_t* SrcBuffer, uint8_t* DestBuffer, int UnCompressedSize, void* ErrorCodeBuffer)
	{
		return GDynamicRHI->RHIEnqueueCompress(SrcBuffer, DestBuffer, UnCompressedSize, ErrorCodeBuffer);
	}
	
	inline bool GetAvailableResolutions(FScreenResolutionArray& Resolutions, bool bIgnoreRefreshRate)
	{
		return RHIGetAvailableResolutions(Resolutions, bIgnoreRefreshRate);
	}
	
	inline void GetSupportedResolution(uint32_t& Width, uint32_t& Height)
	{
		RHIGetSupportedResolution(Width, Height);
	}
	
	inline void VirtualTextureSetFirstMipInMemory(FTexture2DRHIParamRef Texture, uint32_t FirstMip)
	{
		GDynamicRHI->VirtualTextureSetFirstMipInMemory_RenderThread(*this, Texture, FirstMip);
	}
	
	inline void VirtualTextureSetFirstMipVisible(FTexture2DRHIParamRef Texture, uint32_t FirstMip)
	{
		GDynamicRHI->VirtualTextureSetFirstMipVisible_RenderThread(*this, Texture, FirstMip);
	}

	inline void CopySubTextureRegion(FTexture2DRHIParamRef SourceTexture, FTexture2DRHIParamRef DestinationTexture, FBox2D SourceBox, FBox2D DestinationBox)
	{
		GDynamicRHI->RHICopySubTextureRegion_RenderThread(*this, SourceTexture, DestinationTexture, SourceBox, DestinationBox);
	}
	
	inline void ExecuteCommandList(FRHICommandList* CmdList)
	{
		FScopedRHIThreadStaller StallRHIThread(*this);
		GDynamicRHI->RHIExecuteCommandList(CmdList);
	}
	
	inline void SetResourceAliasability(EResourceAliasability AliasMode, FTextureRHIParamRef* InTextures, int32_t NumTextures)
	{
		GDynamicRHI->RHISetResourceAliasability_RenderThread(*this, AliasMode, InTextures, NumTextures);
	}
	
	inline void* GetNativeDevice()
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_RHIMETHOD_GetNativeDevice_Flush);
		ImmediateFlush(EImmediateFlushType::FlushRHIThread); 
		 
		return GDynamicRHI->RHIGetNativeDevice();
	}
	
	inline class IRHICommandContext* GetDefaultContext()
	{
		return RHIGetDefaultContext();
	}
	
	inline class IRHICommandContextContainer* GetCommandContextContainer(int32_t Index, int32_t Num)
	{
		return RHIGetCommandContextContainer(Index, Num, GetGPUMask());
	}
	void UpdateTextureReference(FTextureReferenceRHIParamRef TextureRef, FTextureRHIParamRef NewTexture);

};

 struct FScopedGPUMask
{
	FRHICommandListImmediate& RHICmdList;
	FRHIGPUMask PrevGPUMask;
	inline FScopedGPUMask(FRHICommandListImmediate& InRHICmdList, FRHIGPUMask InGPUMask)
		: RHICmdList(InRHICmdList)
		, PrevGPUMask(InRHICmdList.GetGPUMask())
	{
		InRHICmdList.SetGPUMask(InGPUMask);
	}
	inline ~FScopedGPUMask() { RHICmdList.SetGPUMask(PrevGPUMask); }
};

#if WITH_MGPU
	#define SCOPED_GPU_MASK(RHICmdList, GPUMask) FScopedGPUMask ScopedGPUMask(RHICmdList, GPUMask);
#else
	#define SCOPED_GPU_MASK(RHICmdList, GPUMask)
#endif // WITH_MGPU

// Single commandlist for async compute generation.  In the future we may expand this to allow async compute command generation
// on multiple threads at once.
class  FRHIAsyncComputeCommandListImmediate : public FRHIAsyncComputeCommandList
{
public:

	//If RHIThread is enabled this will dispatch all current commands to the RHI Thread.  If RHI thread is disabled
	//this will immediately execute the current commands.
	//This also queues a GPU Submission command as the final command in the dispatch.
	static void ImmediateDispatch(FRHIAsyncComputeCommandListImmediate& RHIComputeCmdList);

	/** Dispatch current work and change the GPUMask. */
	void SetGPUMask(FRHIGPUMask InGPUMask);
private:
};

// typedef to mark the recursive use of commandlists in the RHI implementations

class  FRHICommandList_RecursiveHazardous : public FRHICommandList
{
	FRHICommandList_RecursiveHazardous()
		: FRHICommandList(FRHIGPUMask::All())
	{

	}
public:
	FRHICommandList_RecursiveHazardous(IRHICommandContext *Context)
		: FRHICommandList(FRHIGPUMask::All())
	{
		SetContext(Context);
	}
};


// This controls if the cmd list bypass can be toggled at runtime. It is quite expensive to have these branches in there.
#define CAN_TOGGLE_COMMAND_LIST_BYPASS (!UE_BUILD_SHIPPING && !UE_BUILD_TEST)

class  FRHICommandListExecutor
{
public:
	enum
	{
		//DefaultBypass = PLATFORM_RHITHREAD_DEFAULT_BYPASS
		DefaultBypass = 0
	};
	FRHICommandListExecutor()
		: bLatchedBypass(!!DefaultBypass)
		, bLatchedUseParallelAlgorithms(false)
	{
	}
	static inline FRHICommandListImmediate& GetImmediateCommandList();
	static inline FRHIAsyncComputeCommandListImmediate& GetImmediateAsyncComputeCommandList();

	void ExecuteList(FRHICommandListBase& CmdList);
	void ExecuteList(FRHICommandListImmediate& CmdList);
	void LatchBypass();

	static void WaitOnRHIThreadFence(FGraphEventRef& Fence);

	inline bool Bypass()
	{
#if CAN_TOGGLE_COMMAND_LIST_BYPASS
		return bLatchedBypass;
#else
		return !!DefaultBypass;
#endif
	}
	inline bool UseParallelAlgorithms()
	{
#if CAN_TOGGLE_COMMAND_LIST_BYPASS
		return bLatchedUseParallelAlgorithms;
#else
		return  FApp::ShouldUseThreadingForPerformance() && !Bypass() && (GSupportsParallelRenderingTasksWithSeparateRHIThread || !IsRunningRHIInSeparateThread());
#endif
	}
	static void CheckNoOutstandingCmdLists();
	static bool IsRHIThreadActive();
	static bool IsRHIThreadCompletelyFlushed();

private:

	void ExecuteInner(FRHICommandListBase& CmdList);
	friend class FExecuteRHIThreadTask;
	static void ExecuteInner_DoExecute(FRHICommandListBase& CmdList);

	bool bLatchedBypass;
	bool bLatchedUseParallelAlgorithms;
	friend class FRHICommandListBase;
	FThreadSafeCounter UIDCounter;
	FThreadSafeCounter OutstandingCmdListCount;
	FRHICommandListImmediate CommandListImmediate;
	FRHIAsyncComputeCommandListImmediate AsyncComputeCmdListImmediate;
};

extern  FRHICommandListExecutor GRHICommandList;

//extern  FAutoConsoleTaskPriority CPrio_SceneRenderingTask;

class FRenderTask
{
public:
	inline static ENamedThreads::Type GetDesiredThread()
	{
		//return CPrio_SceneRenderingTask.Get();
		assert(0);
		return ENamedThreads::NormalTaskPriority;
	}
};


inline FRHICommandListImmediate& FRHICommandListExecutor::GetImmediateCommandList()
{
	return GRHICommandList.CommandListImmediate;
}

inline FRHIAsyncComputeCommandListImmediate& FRHICommandListExecutor::GetImmediateAsyncComputeCommandList()
{
	return GRHICommandList.AsyncComputeCmdListImmediate;
}

struct FScopedCommandListWaitForTasks
{
	FRHICommandListImmediate& RHICmdList;
	bool bWaitForTasks;

	FScopedCommandListWaitForTasks(bool InbWaitForTasks, FRHICommandListImmediate& InRHICmdList = FRHICommandListExecutor::GetImmediateCommandList())
		: RHICmdList(InRHICmdList)
		, bWaitForTasks(InbWaitForTasks)
	{
	}
	 ~FScopedCommandListWaitForTasks();
};


inline FVertexDeclarationRHIRef RHICreateVertexDeclaration(const FVertexDeclarationElementList& Elements)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateVertexDeclaration(Elements);
}

inline FPixelShaderRHIRef RHICreatePixelShader(const std::vector<uint8_t>& Code)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreatePixelShader(Code);
}

inline FPixelShaderRHIRef RHICreatePixelShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreatePixelShader(Library, Hash);
}

inline FVertexShaderRHIRef RHICreateVertexShader(const std::vector<uint8_t>& Code)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateVertexShader(Code);
}

inline FVertexShaderRHIRef RHICreateVertexShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateVertexShader(Library, Hash);
}

inline FHullShaderRHIRef RHICreateHullShader(const std::vector<uint8_t>& Code)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateHullShader(Code);
}

inline FHullShaderRHIRef RHICreateHullShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateHullShader(Library, Hash);
}

inline FDomainShaderRHIRef RHICreateDomainShader(const std::vector<uint8_t>& Code)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateDomainShader(Code);
}

inline FDomainShaderRHIRef RHICreateDomainShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateDomainShader(Library, Hash);
}

inline FGeometryShaderRHIRef RHICreateGeometryShader(const std::vector<uint8_t>& Code)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateGeometryShader(Code);
}

inline FGeometryShaderRHIRef RHICreateGeometryShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateGeometryShader(Library, Hash);
}

inline FGeometryShaderRHIRef RHICreateGeometryShaderWithStreamOutput(const std::vector<uint8_t>& Code, const FStreamOutElementList& ElementList, uint32_t NumStrides, const uint32_t* Strides, int32_t RasterizedStream)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateGeometryShaderWithStreamOutput(Code, ElementList, NumStrides, Strides, RasterizedStream);
}

inline FGeometryShaderRHIRef RHICreateGeometryShaderWithStreamOutput(const FStreamOutElementList& ElementList, uint32_t NumStrides, const uint32_t* Strides, int32_t RasterizedStream, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateGeometryShaderWithStreamOutput(ElementList, NumStrides, Strides, RasterizedStream, Library, Hash);
}

inline FComputeShaderRHIRef RHICreateComputeShader(const std::vector<uint8_t>& Code)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateComputeShader(Code);
}

inline FComputeShaderRHIRef RHICreateComputeShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateComputeShader(Library, Hash);
}

inline FComputeFenceRHIRef RHICreateComputeFence(const std::string& Name)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateComputeFence(Name);
}

inline FGPUFenceRHIRef RHICreateGPUFence(const std::string& Name)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateGPUFence(Name);
}


inline FStagingBufferRHIRef RHICreateStagingBuffer()
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateStagingBuffer();
}



inline FIndexBufferRHIRef RHICreateAndLockIndexBuffer(uint32_t Stride, uint32_t Size, uint32_t InUsage, FRHIResourceCreateInfo& CreateInfo, void*& OutDataBuffer)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateAndLockIndexBuffer(Stride, Size, InUsage, CreateInfo, OutDataBuffer);
}

inline FIndexBufferRHIRef RHICreateIndexBuffer(uint32_t Stride, uint32_t Size, uint32_t InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateIndexBuffer(Stride, Size, InUsage, CreateInfo);
}

inline void* RHILockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer, uint32_t Offset, uint32_t Size, EResourceLockMode LockMode)
{
	return FRHICommandListExecutor::GetImmediateCommandList().LockIndexBuffer(IndexBuffer, Offset, Size, LockMode);
}

inline void RHIUnlockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer)
{
	 FRHICommandListExecutor::GetImmediateCommandList().UnlockIndexBuffer(IndexBuffer);
}

inline FVertexBufferRHIRef RHICreateAndLockVertexBuffer(uint32_t Size, uint32_t InUsage, FRHIResourceCreateInfo& CreateInfo, void*& OutDataBuffer)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateAndLockVertexBuffer(Size, InUsage, CreateInfo, OutDataBuffer);
}

inline FVertexBufferRHIRef RHICreateVertexBuffer(uint32_t Size, uint32_t InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateVertexBuffer(Size, InUsage, CreateInfo);
}

inline void* RHILockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer, uint32_t Offset, uint32_t SizeRHI, EResourceLockMode LockMode)
{
	return FRHICommandListExecutor::GetImmediateCommandList().LockVertexBuffer(VertexBuffer, Offset, SizeRHI, LockMode);
}

inline void RHIUnlockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer)
{
	 FRHICommandListExecutor::GetImmediateCommandList().UnlockVertexBuffer(VertexBuffer);
}

inline FStructuredBufferRHIRef RHICreateStructuredBuffer(uint32_t Stride, uint32_t Size, uint32_t InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateStructuredBuffer(Stride, Size, InUsage, CreateInfo);
}

inline void* RHILockStructuredBuffer(FStructuredBufferRHIParamRef StructuredBuffer, uint32_t Offset, uint32_t SizeRHI, EResourceLockMode LockMode)
{
	return FRHICommandListExecutor::GetImmediateCommandList().LockStructuredBuffer(StructuredBuffer, Offset, SizeRHI, LockMode);
}

inline void RHIUnlockStructuredBuffer(FStructuredBufferRHIParamRef StructuredBuffer)
{
	 FRHICommandListExecutor::GetImmediateCommandList().UnlockStructuredBuffer(StructuredBuffer);
}

inline FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView(FStructuredBufferRHIParamRef StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(StructuredBuffer, bUseUAVCounter, bAppendBuffer);
}

inline FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView(FTextureRHIParamRef Texture, uint32_t MipLevel = 0)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(Texture, MipLevel);
}

inline FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView(FVertexBufferRHIParamRef VertexBuffer, uint8_t Format)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(VertexBuffer, Format);
}

inline FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView(FIndexBufferRHIParamRef IndexBuffer, uint8_t Format)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(IndexBuffer, Format);
}

inline FShaderResourceViewRHIRef RHICreateShaderResourceView(FStructuredBufferRHIParamRef StructuredBuffer)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(StructuredBuffer);
}

inline FShaderResourceViewRHIRef RHICreateShaderResourceView(FVertexBufferRHIParamRef VertexBuffer, uint32_t Stride, uint8_t Format)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(VertexBuffer, Stride, Format);
}

inline FShaderResourceViewRHIRef RHICreateShaderResourceView(FIndexBufferRHIParamRef Buffer)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(Buffer);
}

inline FTextureReferenceRHIRef RHICreateTextureReference(FLastRenderTimeContainer* LastRenderTime)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateTextureReference(LastRenderTime);
}

inline void RHIUpdateTextureReference(FTextureReferenceRHIParamRef TextureRef, FTextureRHIParamRef NewTexture)
{
	 FRHICommandListExecutor::GetImmediateCommandList().UpdateTextureReference(TextureRef, NewTexture);
}

inline FTexture2DRHIRef RHICreateTexture2D(uint32_t SizeX, uint32_t SizeY, uint8_t Format, uint32_t NumMips, uint32_t NumSamples, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateTexture2D(SizeX, SizeY, Format, NumMips, NumSamples, Flags, CreateInfo);
}

inline FStructuredBufferRHIRef RHICreateRTWriteMaskBuffer(FTexture2DRHIRef RenderTarget)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateRTWriteMaskBuffer(RenderTarget);
}

inline FTexture2DRHIRef RHIAsyncCreateTexture2D(uint32_t SizeX, uint32_t SizeY, uint8_t Format, uint32_t NumMips, uint32_t Flags, void** InitialMipData, uint32_t NumInitialMips)
{
	return FRHICommandListExecutor::GetImmediateCommandList().AsyncCreateTexture2D(SizeX, SizeY, Format, NumMips, Flags, InitialMipData, NumInitialMips);
}

inline void RHICopySharedMips(FTexture2DRHIParamRef DestTexture2D, FTexture2DRHIParamRef SrcTexture2D)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CopySharedMips(DestTexture2D, SrcTexture2D);
}

inline FTexture2DArrayRHIRef RHICreateTexture2DArray(uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ, uint8_t Format, uint32_t NumMips, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateTexture2DArray(SizeX, SizeY, SizeZ, Format, NumMips, Flags, CreateInfo);
}

inline FTexture3DRHIRef RHICreateTexture3D(uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ, uint8_t Format, uint32_t NumMips, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateTexture3D(SizeX, SizeY, SizeZ, Format, NumMips, Flags, CreateInfo);
}

inline FShaderResourceViewRHIRef RHICreateShaderResourceView(FTexture2DRHIParamRef Texture2DRHI, uint8_t MipLevel)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(Texture2DRHI, MipLevel);
}

inline FShaderResourceViewRHIRef RHICreateShaderResourceView(FTexture2DRHIParamRef Texture2DRHI, uint8_t MipLevel, uint8_t NumMipLevels, uint8_t Format)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(Texture2DRHI, MipLevel, NumMipLevels, Format);
}

inline FShaderResourceViewRHIRef RHICreateShaderResourceView(FTexture3DRHIParamRef Texture3DRHI, uint8_t MipLevel)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(Texture3DRHI, MipLevel);
}

inline FShaderResourceViewRHIRef RHICreateShaderResourceView(FTexture2DArrayRHIParamRef Texture2DArrayRHI, uint8_t MipLevel)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(Texture2DArrayRHI, MipLevel);
}

inline FShaderResourceViewRHIRef RHICreateShaderResourceView(FTextureCubeRHIParamRef TextureCubeRHI, uint8_t MipLevel)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(TextureCubeRHI, MipLevel);
}

inline FTexture2DRHIRef RHIAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, int32_t NewMipCount, int32_t NewSizeX, int32_t NewSizeY, FThreadSafeCounter* RequestStatus)
{
	return FRHICommandListExecutor::GetImmediateCommandList().AsyncReallocateTexture2D(Texture2D, NewMipCount, NewSizeX, NewSizeY, RequestStatus);
}

inline ETextureReallocationStatus RHIFinalizeAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted)
{
	return FRHICommandListExecutor::GetImmediateCommandList().FinalizeAsyncReallocateTexture2D(Texture2D, bBlockUntilCompleted);
}

inline ETextureReallocationStatus RHICancelAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CancelAsyncReallocateTexture2D(Texture2D, bBlockUntilCompleted);
}

inline void* RHILockTexture2D(FTexture2DRHIParamRef Texture, uint32_t MipIndex, EResourceLockMode LockMode, uint32_t& DestStride, bool bLockWithinMiptail, bool bFlushRHIThread = true)
{
	return FRHICommandListExecutor::GetImmediateCommandList().LockTexture2D(Texture, MipIndex, LockMode, DestStride, bLockWithinMiptail, bFlushRHIThread);
}

inline void RHIUnlockTexture2D(FTexture2DRHIParamRef Texture, uint32_t MipIndex, bool bLockWithinMiptail, bool bFlushRHIThread = true)
{
	 FRHICommandListExecutor::GetImmediateCommandList().UnlockTexture2D(Texture, MipIndex, bLockWithinMiptail, bFlushRHIThread);
}

inline void* RHILockTexture2DArray(FTexture2DArrayRHIParamRef Texture, uint32_t TextureIndex, uint32_t MipIndex, EResourceLockMode LockMode, uint32_t& DestStride, bool bLockWithinMiptail)
{
	return FRHICommandListExecutor::GetImmediateCommandList().LockTexture2DArray(Texture, TextureIndex, MipIndex, LockMode, DestStride, bLockWithinMiptail);
}

inline void RHIUnlockTexture2DArray(FTexture2DArrayRHIParamRef Texture, uint32_t TextureIndex, uint32_t MipIndex, bool bLockWithinMiptail)
{
	 FRHICommandListExecutor::GetImmediateCommandList().UnlockTexture2DArray(Texture, TextureIndex, MipIndex, bLockWithinMiptail);
}

inline void RHIUpdateTexture2D(FTexture2DRHIParamRef Texture, uint32_t MipIndex, const struct FUpdateTextureRegion2D& UpdateRegion, uint32_t SourcePitch, const uint8_t* SourceData)
{
	 FRHICommandListExecutor::GetImmediateCommandList().UpdateTexture2D(Texture, MipIndex, UpdateRegion, SourcePitch, SourceData);
}

inline FUpdateTexture3DData RHIBeginUpdateTexture3D(FTexture3DRHIParamRef Texture, uint32_t MipIndex, const struct FUpdateTextureRegion3D& UpdateRegion)
{
	return FRHICommandListExecutor::GetImmediateCommandList().BeginUpdateTexture3D(Texture, MipIndex, UpdateRegion);
}

inline void RHIEndUpdateTexture3D(FUpdateTexture3DData& UpdateData)
{
	FRHICommandListExecutor::GetImmediateCommandList().EndUpdateTexture3D(UpdateData);
}

inline void RHIUpdateTexture3D(FTexture3DRHIParamRef Texture, uint32_t MipIndex, const struct FUpdateTextureRegion3D& UpdateRegion, uint32_t SourceRowPitch, uint32_t SourceDepthPitch, const uint8_t* SourceData)
{
	 FRHICommandListExecutor::GetImmediateCommandList().UpdateTexture3D(Texture, MipIndex, UpdateRegion, SourceRowPitch, SourceDepthPitch, SourceData);
}

inline FTextureCubeRHIRef RHICreateTextureCube(uint32_t Size, uint8_t Format, uint32_t NumMips, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateTextureCube(Size, Format, NumMips, Flags, CreateInfo);
}

inline FTextureCubeRHIRef RHICreateTextureCubeArray(uint32_t Size, uint32_t ArraySize, uint8_t Format, uint32_t NumMips, uint32_t Flags, FRHIResourceCreateInfo& CreateInfo)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateTextureCubeArray(Size, ArraySize, Format, NumMips, Flags, CreateInfo);
}

inline void* RHILockTextureCubeFace(FTextureCubeRHIParamRef Texture, uint32_t FaceIndex, uint32_t ArrayIndex, uint32_t MipIndex, EResourceLockMode LockMode, uint32_t& DestStride, bool bLockWithinMiptail)
{
	return FRHICommandListExecutor::GetImmediateCommandList().LockTextureCubeFace(Texture, FaceIndex, ArrayIndex, MipIndex, LockMode, DestStride, bLockWithinMiptail);
}

inline void RHIUnlockTextureCubeFace(FTextureCubeRHIParamRef Texture, uint32_t FaceIndex, uint32_t ArrayIndex, uint32_t MipIndex, bool bLockWithinMiptail)
{
	 FRHICommandListExecutor::GetImmediateCommandList().UnlockTextureCubeFace(Texture, FaceIndex, ArrayIndex, MipIndex, bLockWithinMiptail);
}

inline FRenderQueryRHIRef RHICreateRenderQuery(ERenderQueryType QueryType)
{
	return FRHICommandListExecutor::GetImmediateCommandList().CreateRenderQuery_RenderThread(QueryType);
}

inline void RHIAcquireTransientResource(FTextureRHIParamRef Resource)
{
	FRHICommandListExecutor::GetImmediateCommandList().AcquireTransientResource_RenderThread(Resource);
}

inline void RHIDiscardTransientResource(FTextureRHIParamRef Resource)
{
	FRHICommandListExecutor::GetImmediateCommandList().DiscardTransientResource_RenderThread(Resource);
}

inline void RHIAcquireTransientResource(FVertexBufferRHIParamRef Resource)
{
	FRHICommandListExecutor::GetImmediateCommandList().AcquireTransientResource_RenderThread(Resource);
}

inline void RHIDiscardTransientResource(FVertexBufferRHIParamRef Resource)
{
	FRHICommandListExecutor::GetImmediateCommandList().DiscardTransientResource_RenderThread(Resource);
}

inline void RHIAcquireTransientResource(FStructuredBufferRHIParamRef Resource)
{
	FRHICommandListExecutor::GetImmediateCommandList().AcquireTransientResource_RenderThread(Resource);
}

inline void RHIDiscardTransientResource(FStructuredBufferRHIParamRef Resource)
{
	FRHICommandListExecutor::GetImmediateCommandList().DiscardTransientResource_RenderThread(Resource);
}

inline void RHIAcquireThreadOwnership()
{
	return FRHICommandListExecutor::GetImmediateCommandList().AcquireThreadOwnership();
}

inline void RHIReleaseThreadOwnership()
{
	return FRHICommandListExecutor::GetImmediateCommandList().ReleaseThreadOwnership();
}

inline void RHIFlushResources()
{
	return FRHICommandListExecutor::GetImmediateCommandList().FlushResources();
}

inline void RHIVirtualTextureSetFirstMipInMemory(FTexture2DRHIParamRef Texture, uint32_t FirstMip)
{
	 FRHICommandListExecutor::GetImmediateCommandList().VirtualTextureSetFirstMipInMemory(Texture, FirstMip);
}

inline void RHIVirtualTextureSetFirstMipVisible(FTexture2DRHIParamRef Texture, uint32_t FirstMip)
{
	 FRHICommandListExecutor::GetImmediateCommandList().VirtualTextureSetFirstMipVisible(Texture, FirstMip);
}

inline void RHIExecuteCommandList(FRHICommandList* CmdList)
{
	 FRHICommandListExecutor::GetImmediateCommandList().ExecuteCommandList(CmdList);
}

inline void* RHIGetNativeDevice()
{
	return FRHICommandListExecutor::GetImmediateCommandList().GetNativeDevice();
}

inline void RHIRecreateRecursiveBoundShaderStates()
{
	FRHICommandListExecutor::GetImmediateCommandList(). ImmediateFlush(EImmediateFlushType::FlushRHIThread);
	GDynamicRHI->RHIRecreateRecursiveBoundShaderStates();
}

inline FRHIShaderLibraryRef RHICreateShaderLibrary(EShaderPlatform Platform, FString const& FilePath, FString const& Name)
{
    return GDynamicRHI->RHICreateShaderLibrary(Platform, FilePath, Name);
}


#include "RHICommandList.inl"
