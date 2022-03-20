// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <assert.h>
#include <functional>
#include <vector>
#include "Platform/Windows/WindowsPlatformProcess.h"
#include "Platform/ThreadSafeCounter.h"
#include "Engine/TaskGraphCommon.h"
#include "Platform/Windows/WindowsPlatformAtomics.h"
#include "Platform/YPlatform.h"



#if _WIN32
#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedAdd
#undef InterlockedExchange
#undef InterlockedExchangeAdd
#undef InterlockedCompareExchange
#undef InterlockedCompareExchangePointer
#undef InterlockedExchange64
#undef InterlockedExchangeAdd64
#undef InterlockedCompareExchange64
#undef InterlockedIncrement64
#undef InterlockedDecrement64
#endif

#ifdef _DEBUG 

 void DoTestCriticalStall();
extern  int GTestCriticalStalls;

inline void TestCriticalStall()
{
	if (GTestCriticalStalls)
	{
		DoTestCriticalStall();
	}
}
#else
inline void TestCriticalStall()
{
}
#endif

 void LockFreeTagCounterHasOverflowed();
 void LockFreeLinksExhausted(uint32_t TotalNum);
 void* LockFreeAllocLinks(size_t AllocSize);
 void LockFreeFreeLinks(size_t AllocSize, void* Ptr);

#define MAX_LOCK_FREE_LINKS_AS_BITS (26)
#define MAX_LOCK_FREE_LINKS (1 << 26)

template<int TPaddingForCacheContention>
struct FPaddingForCacheContention
{
	uint8_t PadToAvoidContention[TPaddingForCacheContention];
};

template<>
struct FPaddingForCacheContention<0>
{
};

template<class T, uint32_t MaxTotalItems, uint32_t ItemsPerPage>
class TLockFreeAllocOnceIndexedAllocator // MaxTotalItems = 1<< 26 , ItemPerPace = 1<<14
{
	enum
	{
		MaxBlocks = (MaxTotalItems + ItemsPerPage - 1) / ItemsPerPage // 1<< 12
	};
public:

	TLockFreeAllocOnceIndexedAllocator()
	{
		NextIndex.Increment(); // skip the null ptr
		for (uint32_t Index = 0; Index < MaxBlocks; Index++)
		{
			Pages[Index] = nullptr;
		}
	}

	inline uint32_t Alloc(uint32_t Count = 1)
	{
		uint32_t FirstItem = NextIndex.Add(Count);
		if (FirstItem + Count > MaxTotalItems)
		{
			LockFreeLinksExhausted(MaxTotalItems);
		}
		for (uint32_t CurrentItem = FirstItem; CurrentItem < FirstItem + Count; CurrentItem++)
		{
			new (GetRawItem(CurrentItem)) T();
		}
		return FirstItem;
	}
	inline T* GetItem(uint32_t Index)
	{
		if (!Index)
		{
			return nullptr;
		}
		uint32_t BlockIndex = Index / ItemsPerPage;
		uint32_t SubIndex = Index % ItemsPerPage;
		assert(Index < (uint32_t)NextIndex.GetValue() && Index < MaxTotalItems && BlockIndex < MaxBlocks && Pages[BlockIndex]);
		return Pages[BlockIndex] + SubIndex;
	}

private:
	void* GetRawItem(uint32_t Index)
	{
		uint32_t BlockIndex = Index / ItemsPerPage;
		uint32_t SubIndex = Index % ItemsPerPage;
		assert(Index && Index < (uint32_t)NextIndex.GetValue() && Index < MaxTotalItems && BlockIndex < MaxBlocks);
		if (!Pages[BlockIndex])
		{
			T* NewBlock = (T*)LockFreeAllocLinks(ItemsPerPage * sizeof(T));
			assert(IsAligned(NewBlock, __alignof(T)));
			if (FPlatformAtomics::InterlockedCompareExchangePointer((void**)&Pages[BlockIndex], NewBlock, nullptr) != nullptr)
			{
				// we lost discard block
				assert(Pages[BlockIndex] && Pages[BlockIndex] != NewBlock);
				LockFreeFreeLinks(ItemsPerPage * sizeof(T), NewBlock);
			}
			else
			{
				assert(Pages[BlockIndex]);
			}
		}
		return (void*)(Pages[BlockIndex] + SubIndex);
	}

	uint8_t PadToAvoidContention0[PLATFORM_CACHE_LINE_SIZE];
	FThreadSafeCounter NextIndex;
	uint8_t PadToAvoidContention1[PLATFORM_CACHE_LINE_SIZE];
	T* Pages[MaxBlocks];
	uint8_t PadToAvoidContention2[PLATFORM_CACHE_LINE_SIZE];
};


#define MAX_TagBitsValue (uint64_t(1) << (64 - MAX_LOCK_FREE_LINKS_AS_BITS))
struct FIndexedLockFreeLink;


MS_ALIGN(8)
struct FIndexedPointer
{
	// no constructor, intentionally. We need to keep the ABA double counter in tact

	// This should only be used for FIndexedPointer's with no outstanding concurrency.
	// Not recycled links, for example.
	void Init()
	{
		static_assert(((MAX_LOCK_FREE_LINKS - 1) & MAX_LOCK_FREE_LINKS) == 0, "MAX_LOCK_FREE_LINKS must be a power of two");
		Ptrs = 0;
	}
	inline void SetAll(uint32_t Ptr, uint64_t CounterAndState)
	{
		assert(Ptr < MAX_LOCK_FREE_LINKS && CounterAndState < (uint64_t(1) << (64 - MAX_LOCK_FREE_LINKS_AS_BITS)));
		Ptrs = (uint64_t(Ptr) | (CounterAndState << MAX_LOCK_FREE_LINKS_AS_BITS));
	}

	inline uint32_t GetPtr() const
	{
		return uint32_t(Ptrs & (MAX_LOCK_FREE_LINKS - 1));
	}

	inline void SetPtr(uint32_t To)
	{
		SetAll(To, GetCounterAndState());
	}

	inline uint64_t GetCounterAndState() const
	{
		return (Ptrs >> MAX_LOCK_FREE_LINKS_AS_BITS);
	}

	inline void SetCounterAndState(uint64_t To)
	{
		SetAll(GetPtr(), To);
	}

	inline void AdvanceCounterAndState(const FIndexedPointer &From, uint64_t TABAInc)
	{
		SetCounterAndState(From.GetCounterAndState() + TABAInc);
		if (GetCounterAndState() < From.GetCounterAndState())
		{
			// this is not expected to be a problem and it is not expected to happen very often. When it does happen, we will sleep as an extra precaution.
			LockFreeTagCounterHasOverflowed();
		}
	}

	template<uint64_t TABAInc>
	inline uint64_t GetState() const
	{
		return GetCounterAndState() & (TABAInc - 1);
	}

	template<uint64_t TABAInc>
	inline void SetState(uint64_t Value)
	{
		assert(Value < TABAInc);
		SetCounterAndState((GetCounterAndState() & ~(TABAInc - 1)) | Value);
	}

	inline void AtomicRead(const FIndexedPointer& Other)
	{
		assert(IsAligned(&Ptrs, 8) && IsAligned(&Other.Ptrs, 8));
		Ptrs = uint64_t(FPlatformAtomics::AtomicRead64((volatile const long long*)&Other.Ptrs));
		TestCriticalStall();
	}

	inline bool InterlockedCompareExchange(const FIndexedPointer& Exchange, const FIndexedPointer& Comparand)
	{
		TestCriticalStall();
		return uint64_t(FPlatformAtomics::InterlockedCompareExchange((volatile long long *)&Ptrs, Exchange.Ptrs, Comparand.Ptrs)) == Comparand.Ptrs;
	}

	inline bool operator==(const FIndexedPointer& Other) const
	{
		return Ptrs == Other.Ptrs;
	}
	inline bool operator!=(const FIndexedPointer& Other) const
	{
		return Ptrs != Other.Ptrs;
	}

private:
	uint64_t Ptrs;

}GCC_ALIGN(8);;

struct FIndexedLockFreeLink
{
	FIndexedPointer DoubleNext;
	void *Payload;
	uint32_t SingleNext;
};

// there is a version of this code that uses 128 bit atomics to avoid the indirection, that is why we have this policy class at all.
struct FLockFreeLinkPolicy
{
	enum
	{
		MAX_BITS_IN_TLinkPtr = MAX_LOCK_FREE_LINKS_AS_BITS
	};
	typedef FIndexedPointer TDoublePtr;
	typedef FIndexedLockFreeLink TLink;
	typedef uint32_t TLinkPtr;
	typedef TLockFreeAllocOnceIndexedAllocator<FIndexedLockFreeLink, MAX_LOCK_FREE_LINKS, 16384> TAllocator;

	static inline FIndexedLockFreeLink* DerefLink(uint32_t Ptr)
	{
		return LinkAllocator.GetItem(Ptr);
	}
	static inline FIndexedLockFreeLink* IndexToLink(uint32_t Index)
	{
		return LinkAllocator.GetItem(Index);
	}
	static inline uint32_t IndexToPtr(uint32_t Index)
	{
		return Index;
	}

	static uint32_t AllocLockFreeLink();
	static void FreeLockFreeLink(uint32_t Item);
	static TAllocator LinkAllocator;
};

template<int TPaddingForCacheContention, uint64_t TABAInc = 1>
class FLockFreePointerListLIFORoot : public FNoncopyable
{
	typedef FLockFreeLinkPolicy::TDoublePtr TDoublePtr;
	typedef FLockFreeLinkPolicy::TLink TLink;
	typedef FLockFreeLinkPolicy::TLinkPtr TLinkPtr;

public:
	inline FLockFreePointerListLIFORoot()
	{
		// We want to make sure we have quite a lot of extra counter values to avoid the ABA problem. This could probably be relaxed, but eventually it will be dangerous. 
		// The question is "how many queue operations can a thread starve for".
		static_assert(MAX_TagBitsValue / TABAInc >= (1 << 23), "risk of ABA problem");
		static_assert((TABAInc & (TABAInc - 1)) == 0, "must be power of two");
		Reset();
	}

	void Reset()
	{
		Head.Init();
	}

	void Push(TLinkPtr Item)
	{
		while (true)
		{
			TDoublePtr LocalHead;
			LocalHead.AtomicRead(Head);
			TDoublePtr NewHead;
			NewHead.AdvanceCounterAndState(LocalHead, TABAInc);
			NewHead.SetPtr(Item);
			FLockFreeLinkPolicy::DerefLink(Item)->SingleNext = LocalHead.GetPtr();
			if (Head.InterlockedCompareExchange(NewHead, LocalHead))
			{
				break;
			}
		}
	}

	bool PushIf(std::function<TLinkPtr(uint64_t)> AllocateIfOkToPush)
	{
		static_assert(TABAInc > 1, "method should not be used for lists without state");
		while (true)
		{
			TDoublePtr LocalHead;
			LocalHead.AtomicRead(Head);
			uint64_t LocalState = LocalHead.GetState<TABAInc>();
			TLinkPtr Item = AllocateIfOkToPush(LocalState);
			if (!Item)
			{
				return false;
			}

			TDoublePtr NewHead;
			NewHead.AdvanceCounterAndState(LocalHead, TABAInc);
			FLockFreeLinkPolicy::DerefLink(Item)->SingleNext = LocalHead.GetPtr();
			NewHead.SetPtr(Item);
			if (Head.InterlockedCompareExchange(NewHead, LocalHead))
			{
				break;
			}
		}
		return true;
	}


	TLinkPtr Pop()
	{
		TLinkPtr Item = 0;
		while (true)
		{
			TDoublePtr LocalHead;
			LocalHead.AtomicRead(Head);
			Item = LocalHead.GetPtr();
			if (!Item)
			{
				break;
			}
			TDoublePtr NewHead;
			NewHead.AdvanceCounterAndState(LocalHead, TABAInc);
			TLink* ItemP = FLockFreeLinkPolicy::DerefLink(Item);
			NewHead.SetPtr(ItemP->SingleNext);
			if (Head.InterlockedCompareExchange(NewHead, LocalHead))
			{
				ItemP->SingleNext = 0;
				break;
			}
		}
		return Item;
	}

	TLinkPtr PopAll()
	{
		TLinkPtr Item = 0;
		while (true)
		{
			TDoublePtr LocalHead;
			LocalHead.AtomicRead(Head);
			Item = LocalHead.GetPtr();
			if (!Item)
			{
				break;
			}
			TDoublePtr NewHead;
			NewHead.AdvanceCounterAndState(LocalHead, TABAInc);
			NewHead.SetPtr(0);
			if (Head.InterlockedCompareExchange(NewHead, LocalHead))
			{
				break;
			}
		}
		return Item;
	}

	TLinkPtr PopAllAndChangeState(std::function<uint64_t(uint64_t)> StateChange)
	{
		static_assert(TABAInc > 1, "method should not be used for lists without state");
		TLinkPtr Item = 0;
		while (true)
		{
			TDoublePtr LocalHead;
			LocalHead.AtomicRead(Head);
			Item = LocalHead.GetPtr();
			TDoublePtr NewHead;
			NewHead.AdvanceCounterAndState(LocalHead, TABAInc);
			NewHead.SetState<TABAInc>(StateChange(LocalHead.GetState<TABAInc>()));
			NewHead.SetPtr(0);
			if (Head.InterlockedCompareExchange(NewHead, LocalHead))
			{
				break;
			}
		}
		return Item;
	}

	inline bool IsEmpty() const
	{
		return !Head.GetPtr();
	}

	inline uint64_t GetState() const
	{
		TDoublePtr LocalHead;
		LocalHead.AtomicRead(Head);
		return LocalHead.GetState<TABAInc>();
	}

private:

	FPaddingForCacheContention<TPaddingForCacheContention> PadToAvoidContention1;
	TDoublePtr Head;
	FPaddingForCacheContention<TPaddingForCacheContention> PadToAvoidContention2;
};

template<class T, int TPaddingForCacheContention, uint64_t TABAInc = 1>
class FLockFreePointerListLIFOBase : public FNoncopyable
{
	typedef FLockFreeLinkPolicy::TDoublePtr TDoublePtr;
	typedef FLockFreeLinkPolicy::TLink TLink;
	typedef FLockFreeLinkPolicy::TLinkPtr TLinkPtr;
public:
	void Reset()
	{
		RootList.Reset();
	}

	void Push(T* InPayload)
	{
		TLinkPtr Item = FLockFreeLinkPolicy::AllocLockFreeLink();
		FLockFreeLinkPolicy::DerefLink(Item)->Payload = InPayload;
		RootList.Push(Item);
	}

	bool PushIf(T* InPayload, std::function<bool(uint64_t)> OkToPush)
	{
		TLinkPtr Item = 0;

		auto AllocateIfOkToPush = [&OkToPush, InPayload, &Item](uint64_t State)->TLinkPtr
		{
			if (OkToPush(State))
			{
				if (!Item)
				{
					Item = FLockFreeLinkPolicy::AllocLockFreeLink();
					FLockFreeLinkPolicy::DerefLink(Item)->Payload = InPayload;
				}
				return Item;
			}
			return 0;
		};
		if (!RootList.PushIf(AllocateIfOkToPush))
		{
			if (Item)
			{
				// we allocated the link, but it turned out that the list was closed
				FLockFreeLinkPolicy::FreeLockFreeLink(Item);
			}
			return false;
		}
		return true;
	}


	T* Pop()
	{
		TLinkPtr Item = RootList.Pop();
		T* Result = nullptr;
		if (Item)
		{
			Result = (T*)FLockFreeLinkPolicy::DerefLink(Item)->Payload;
			FLockFreeLinkPolicy::FreeLockFreeLink(Item);
		}
		return Result;
	}

	void PopAll(std::vector<T*>& OutArray)
	{
		TLinkPtr Links = RootList.PopAll();
		while (Links)
		{
			TLink* LinksP = FLockFreeLinkPolicy::DerefLink(Links);
			OutArray.push_back((T*)LinksP->Payload);
			TLinkPtr Del = Links;
			Links = LinksP->SingleNext;
			FLockFreeLinkPolicy::FreeLockFreeLink(Del);
		}
	}

	void PopAllAndChangeState(std::vector<T*>& OutArray, std::function<uint64_t(uint64_t)> StateChange)
	{
		TLinkPtr Links = RootList.PopAllAndChangeState(StateChange);
		while (Links)
		{
			TLink* LinksP = FLockFreeLinkPolicy::DerefLink(Links);
			OutArray.push_back((T*)LinksP->Payload);
			TLinkPtr Del = Links;
			Links = LinksP->SingleNext;
			FLockFreeLinkPolicy::FreeLockFreeLink(Del);
		}
	}

	inline bool IsEmpty() const
	{
		return RootList.IsEmpty();
	}

	inline uint64_t GetState() const
	{
		return RootList.GetState();
	}

private:

	FLockFreePointerListLIFORoot<TPaddingForCacheContention, TABAInc> RootList;
};

template<class T, int TPaddingForCacheContention, uint64_t TABAInc = 1>
class FLockFreePointerFIFOBase : public FNoncopyable
{
	typedef FLockFreeLinkPolicy::TDoublePtr TDoublePtr;
	typedef FLockFreeLinkPolicy::TLink TLink;
	typedef FLockFreeLinkPolicy::TLinkPtr TLinkPtr;
public:

	inline FLockFreePointerFIFOBase()
	{
		// We want to make sure we have quite a lot of extra counter values to avoid the ABA problem. This could probably be relaxed, but eventually it will be dangerous. 
		// The question is "how many queue operations can a thread starve for".
		static_assert(TABAInc <= 65536, "risk of ABA problem");
		static_assert((TABAInc & (TABAInc - 1)) == 0, "must be power of two");

		Head.Init();
		Tail.Init();
		TLinkPtr Stub = FLockFreeLinkPolicy::AllocLockFreeLink();
		Head.SetPtr(Stub);
		Tail.SetPtr(Stub);
	}

	void Push(T* InPayload)
	{
		TLinkPtr Item = FLockFreeLinkPolicy::AllocLockFreeLink();
		FLockFreeLinkPolicy::DerefLink(Item)->Payload = InPayload;
		TDoublePtr LocalTail;
		while (true)
		{
			LocalTail.AtomicRead(Tail);
			TLink* LoadTailP = FLockFreeLinkPolicy::DerefLink(LocalTail.GetPtr());
			TDoublePtr LocalNext;
			LocalNext.AtomicRead(LoadTailP->DoubleNext);
			TDoublePtr TestLocalTail;
			TestLocalTail.AtomicRead(Tail);
			if (TestLocalTail == LocalTail)
			{
				if (LocalNext.GetPtr())
				{
					TDoublePtr NewTail;
					NewTail.AdvanceCounterAndState(LocalTail, TABAInc);
					NewTail.SetPtr(LocalNext.GetPtr());
					Tail.InterlockedCompareExchange(NewTail, LocalTail);
				}
				else
				{
					TDoublePtr NewNext;
					NewNext.AdvanceCounterAndState(LocalNext, 1);
					NewNext.SetPtr(Item);
					if (LoadTailP->DoubleNext.InterlockedCompareExchange(NewNext, LocalNext))
					{
						break;
					}
				}
			}
		}
		{
			TestCriticalStall();
			TDoublePtr NewTail;
			NewTail.AdvanceCounterAndState(LocalTail, TABAInc);
			NewTail.SetPtr(Item);
			Tail.InterlockedCompareExchange(NewTail, LocalTail);
		}
	}

	T* Pop()
	{
		T* Result = nullptr;
		TDoublePtr LocalHead;
		while (true)
		{
			LocalHead.AtomicRead(Head);
			TDoublePtr LocalTail;
			LocalTail.AtomicRead(Tail);
			TDoublePtr LocalNext;
			LocalNext.AtomicRead(FLockFreeLinkPolicy::DerefLink(LocalHead.GetPtr())->DoubleNext);
			TDoublePtr LocalHeadTest;
			LocalHeadTest.AtomicRead(Head);
			if (LocalHead == LocalHeadTest)
			{
				if (LocalHead.GetPtr() == LocalTail.GetPtr())
				{
					if (!LocalNext.GetPtr())
					{
						return nullptr;
					}
					TestCriticalStall();
					TDoublePtr NewTail;
					NewTail.AdvanceCounterAndState(LocalTail, TABAInc);
					NewTail.SetPtr(LocalNext.GetPtr()); // 特殊情况，tail->next已经更新，但tail还没有更新
					Tail.InterlockedCompareExchange(NewTail, LocalTail);
				}
				else
				{
					Result = (T*)FLockFreeLinkPolicy::DerefLink(LocalNext.GetPtr())->Payload;
					TDoublePtr NewHead;
					NewHead.AdvanceCounterAndState(LocalHead, TABAInc);
					NewHead.SetPtr(LocalNext.GetPtr());
					if (Head.InterlockedCompareExchange(NewHead, LocalHead))
					{
						break;
					}
				}
			}
		}
		FLockFreeLinkPolicy::FreeLockFreeLink(LocalHead.GetPtr());
		return Result;
	}

	void PopAll(std::vector<T*>& OutArray)
	{
		while (T* Item = Pop())
		{
			OutArray.push_back(Item);
		}
	}


	inline bool IsEmpty() const
	{
		TDoublePtr LocalHead;
		LocalHead.AtomicRead(Head);
		TDoublePtr LocalNext;
		LocalNext.AtomicRead(FLockFreeLinkPolicy::DerefLink(LocalHead.GetPtr())->DoubleNext);
		return !LocalNext.GetPtr();
	}

private:

	FPaddingForCacheContention<TPaddingForCacheContention> PadToAvoidContention1;
	TDoublePtr Head;
	FPaddingForCacheContention<TPaddingForCacheContention> PadToAvoidContention2;
	TDoublePtr Tail;
	FPaddingForCacheContention<TPaddingForCacheContention> PadToAvoidContention3;
};


template<class T, int TPaddingForCacheContention, int NumPriorities>
class FStallingTaskQueue : public FNoncopyable
{
	typedef FLockFreeLinkPolicy::TDoublePtr TDoublePtr;
	typedef FLockFreeLinkPolicy::TLink TLink;
	typedef FLockFreeLinkPolicy::TLinkPtr TLinkPtr;
public:
	FStallingTaskQueue()
	{
		MasterState.Init();
	}
	int Push(T* InPayload, uint32_t Priority)
	{
		assert(Priority < NumPriorities);
		TDoublePtr LocalMasterState;
		LocalMasterState.AtomicRead(MasterState);
		PriorityQueues[Priority].Push(InPayload);
		TDoublePtr NewMasterState;
		NewMasterState.AdvanceCounterAndState(LocalMasterState, 1);
		int ThreadToWake = FindThreadToWake(LocalMasterState.GetPtr());
		if (ThreadToWake >= 0)
		{
			NewMasterState.SetPtr(TurnOffBit(LocalMasterState.GetPtr(), ThreadToWake));
		}
		else
		{
			NewMasterState.SetPtr(LocalMasterState.GetPtr());
		}
		while (!MasterState.InterlockedCompareExchange(NewMasterState, LocalMasterState))
		{
			LocalMasterState.AtomicRead(MasterState);
			NewMasterState.AdvanceCounterAndState(LocalMasterState, 1);
			ThreadToWake = FindThreadToWake(LocalMasterState.GetPtr());
#if 0
			// This block of code is supposed to avoid starting the task thread if the queues are empty.
			// There WAS a silly bug here. In rare cases no task thread is woken up.
			// That bug has been fixed, but I don't think we really need this code anyway.
			// Without this block, it is possible that we do a redundant wake-up, but for task threads, that can happen anyway. 
			// For named threads, the rare redundant wakeup seems acceptable.
			if (ThreadToWake >= 0)
			{
				bool bAny = false;
				for (int32 Index = 0; !bAny && Index < NumPriorities; Index++)
				{
					bAny = !PriorityQueues[Index].IsEmpty();
				}
				if (!bAny) // if there is nothing in the queues, then don't wake anyone
				{
					ThreadToWake = -1;
				}
			}
#endif
			if (ThreadToWake >= 0)
			{
				NewMasterState.SetPtr(TurnOffBit(LocalMasterState.GetPtr(), ThreadToWake));
			}
			else
			{
				NewMasterState.SetPtr(LocalMasterState.GetPtr());
			}
		}
		return ThreadToWake;
	}

	T* Pop(int MyThread, bool bAllowStall)
	{
		assert(MyThread >= 0 && MyThread < FLockFreeLinkPolicy::MAX_BITS_IN_TLinkPtr);

		while (true)
		{
			TDoublePtr LocalMasterState;
			LocalMasterState.AtomicRead(MasterState);
			assert(!TestBit(LocalMasterState.GetPtr(), MyThread) || !FPlatformProcess::SupportsMultithreading()); // you should not be stalled if you are asking for a task
			for (int Index = 0; Index < NumPriorities; Index++)
			{
				T *Result = PriorityQueues[Index].Pop();
				if (Result)
				{
					return Result;
				}
			}
			if (!bAllowStall)
			{
				break; // if we aren't stalling, we are done, the queues are empty
			}
			TDoublePtr NewMasterState;
			NewMasterState.AdvanceCounterAndState(LocalMasterState, 1);
			NewMasterState.SetPtr(TurnOnBit(LocalMasterState.GetPtr(), MyThread));
			if (MasterState.InterlockedCompareExchange(NewMasterState, LocalMasterState))
			{
				break;
			}
		}
		return nullptr;
	}

private:

	static int FindThreadToWake(TLinkPtr Ptr)
	{
		int Result = -1;
		uint64_t  Test = uint64_t(Ptr);
		if (Test)
		{
			Result = 0;
			while (!(Test & 1))
			{
				Test >>= 1;
				Result++;
			}
		}
		return Result;
	}

	static TLinkPtr TurnOffBit(TLinkPtr Ptr, int BitToTurnOff)
	{
		return (TLinkPtr)(uint64_t(Ptr) & ~(uint64_t(1) << BitToTurnOff));
	}

	static TLinkPtr TurnOnBit(TLinkPtr Ptr, int BitToTurnOn)
	{
		return (TLinkPtr)(uint64_t(Ptr) | (uint64_t(1) << BitToTurnOn));
	}

	static bool TestBit(TLinkPtr Ptr, int BitToTest)
	{
		return !!(uint64_t(Ptr) & (uint64_t(1) << BitToTest));
	}

	FLockFreePointerFIFOBase<T, TPaddingForCacheContention> PriorityQueues[NumPriorities];
	// not a pointer to anything, rather tracks the stall state of all threads servicing this queue.
	TDoublePtr MasterState;
	FPaddingForCacheContention<TPaddingForCacheContention> PadToAvoidContention1;
};




template<class T, int TPaddingForCacheContention>
class TLockFreePointerListLIFOPad : private FLockFreePointerListLIFOBase<T, TPaddingForCacheContention>
{
public:

	/**
	*	Push an item onto the head of the list.
	*
	*	@param NewItem, the new item to push on the list, cannot be NULL.
	*/
	void Push(T *NewItem)
	{
		FLockFreePointerListLIFOBase<T, TPaddingForCacheContention>::Push(NewItem);
	}

	/**
	*	Pop an item from the list or return NULL if the list is empty.
	*	@return The popped item, if any.
	*/
	T* Pop()
	{
		return FLockFreePointerListLIFOBase<T, TPaddingForCacheContention>::Pop();
	}

	/**
	*	Pop all items from the list.
	*
	*	@param Output The array to hold the returned items. Must be empty.
	*/
	void PopAll(std::vector<T *>& Output)
	{
		FLockFreePointerListLIFOBase<T, TPaddingForCacheContention>::PopAll(Output);
	}

	/**
	*	Check if the list is empty.
	*
	*	@return true if the list is empty.
	*	CAUTION: This methods safety depends on external assumptions. For example, if another thread could add to the list at any time, the return value is no better than a best guess.
	*	As typically used, the list is not being access concurrently when this is called.
	*/
	inline bool IsEmpty() const
	{
		return FLockFreePointerListLIFOBase<T, TPaddingForCacheContention>::IsEmpty();
	}
};

template<class T>
class TLockFreePointerListLIFO : public TLockFreePointerListLIFOPad<T, 0>
{

};

template<class T, int TPaddingForCacheContention>
class TLockFreePointerListUnordered : public TLockFreePointerListLIFOPad<T, TPaddingForCacheContention>
{

};

template<class T, int TPaddingForCacheContention>
class TLockFreePointerListFIFO : private FLockFreePointerFIFOBase<T, TPaddingForCacheContention>
{
public:

	/**
	*	Push an item onto the head of the list.
	*
	*	@param NewItem, the new item to push on the list, cannot be NULL.
	*/
	void Push(T *NewItem)
	{
		FLockFreePointerFIFOBase<T, TPaddingForCacheContention>::Push(NewItem);
	}

	/**
	*	Pop an item from the list or return NULL if the list is empty.
	*	@return The popped item, if any.
	*/
	T* Pop()
	{
		return FLockFreePointerFIFOBase<T, TPaddingForCacheContention>::Pop();
	}

	/**
	*	Pop all items from the list.
	*
	*	@param Output The array to hold the returned items. Must be empty.
	*/
	void PopAll(std::vector<T *>& Output)
	{
		FLockFreePointerFIFOBase<T, TPaddingForCacheContention>::PopAll(Output);
	}

	/**
	*	Check if the list is empty.
	*
	*	@return true if the list is empty.
	*	CAUTION: This methods safety depends on external assumptions. For example, if another thread could add to the list at any time, the return value is no better than a best guess.
	*	As typically used, the list is not being access concurrently when this is called.
	*/
	inline bool IsEmpty() const
	{
		return FLockFreePointerFIFOBase<T, TPaddingForCacheContention>::IsEmpty();
	}
};


template<class T, int TPaddingForCacheContention>
class TClosableLockFreePointerListUnorderedSingleConsumer : private FLockFreePointerListLIFOBase<T, TPaddingForCacheContention, 2>
{
public:

	/**
	*	Reset the list to the initial state. Not thread safe, but used for recycling when we know all users are gone.
	*/
	void Reset()
	{
		FLockFreePointerListLIFOBase<T, TPaddingForCacheContention, 2>::Reset();
	}

	/**
	*	Push an item onto the head of the list, unless the list is closed
	*
	*	@param NewItem, the new item to push on the list, cannot be NULL
	*	@return true if the item was pushed on the list, false if the list was closed.
	*/
	bool PushIfNotClosed(T *NewItem)
	{
		return FLockFreePointerListLIFOBase<T, TPaddingForCacheContention, 2>::PushIf(NewItem, [](uint64_t State)->bool {return !(State & 1); });
	}

	/**
	*	Pop all items from the list and atomically close it.
	*
	*	@param Output The array to hold the returned items. Must be empty.
	*/
	void PopAllAndClose(std::vector<T *>& Output)
	{
		auto CheckOpenAndClose = [](uint64_t State) -> uint64_t
		{
			assert(!(State & 1));
			return State | 1;
		};
		FLockFreePointerListLIFOBase<T, TPaddingForCacheContention, 2>::PopAllAndChangeState(Output, CheckOpenAndClose);
	}

	/**
	*	Check if the list is closed
	*
	*	@return true if the list is closed.
	*/
	bool IsClosed() const
	{
		return !!(FLockFreePointerListLIFOBase<T, TPaddingForCacheContention, 2>::GetState() & 1);
	}

};


