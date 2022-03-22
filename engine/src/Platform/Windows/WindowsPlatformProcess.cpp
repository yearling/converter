// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Platform/Event.h"
#include "Platform/Windows/WindowsEvent.h"
#include "Platform/windows/WindowsPlatformProcess.h"
#include <assert.h>
#include "Platform/EventPool.h"
#include <algorithm>
#include <sysinfoapi.h>
#include "Platform/Windows/WindowsRunnableThread.h"
#include "Math/YMath.h"
#include "Platform/YPlatfomMisc.h"

FEvent* FPlatformProcess::CreateSynchEvent(bool bIsManualReset)
{
	// Allocate the new object
	FEvent* Event = NULL;	
	//if (FPlatformProcess::SupportsMultithreading())
	if (1)
	{
		Event = new FEventWin();
	}
	else
	{
		// Fake event object.
		//Event = new FSingleThreadEvent();
	}
	// If the internal create fails, delete the instance and return NULL
	if (!Event->Create(bIsManualReset))
	{
		delete Event;
		Event = NULL;
	}
	return Event;
}


bool FEventWin::Wait(unsigned int WaitTime, const bool bIgnoreThreadIdleStats /*= false*/)
{
	WaitForStats();

	assert( Event );

	//FThreadIdleStats::FScopeIdle Scope( bIgnoreThreadIdleStats );
	return (WaitForSingleObject( Event, WaitTime ) == WAIT_OBJECT_0);
}

void FEventWin::Trigger()
{
	TriggerForStats();
	assert( Event );
	SetEvent( Event );
}

void FEventWin::Reset()
{
	ResetForStats();
	assert( Event );
	ResetEvent( Event );
}

void FWindowsPlatformProcess::Sleep(float Seconds)
{
	SleepNoStats(Seconds);
}

void FWindowsPlatformProcess::SleepNoStats(float Seconds)
{
	unsigned int Milliseconds = (unsigned int)(Seconds * 1000.0);
	if (Milliseconds == 0)
	{
		::SwitchToThread();
	}
	::Sleep(Milliseconds);
}

void FWindowsPlatformProcess::SleepInfinite()
{
	::Sleep(INFINITE);
}

FEvent* FWindowsPlatformProcess::GetSynchEventFromPool(bool bIsManualReset)
{
	return bIsManualReset
		? FEventPool<EEventPoolTypes::ManualReset>::Get().GetEventFromPool()
		: FEventPool<EEventPoolTypes::AutoReset>::Get().GetEventFromPool();
}


void FWindowsPlatformProcess::ReturnSynchEventToPool(FEvent* Event)
{
	if (!Event)
	{
		return;
	}

	if (Event->IsManualReset())
	{
		FEventPool<EEventPoolTypes::ManualReset>::Get().ReturnToPool(Event);
	}
	else
	{
		FEventPool<EEventPoolTypes::AutoReset>::Get().ReturnToPool(Event);
	}
}

bool FWindowsPlatformProcess::SupportsMultithreading()
{
	return true;
}

int FWindowsPlatformProcess::NumberOfWorkerThreadsToSpawn()
{
	static int32_t MaxServerWorkerThreads = 4;
	static int32_t MaxWorkerThreads = 26;

	int NumberOfCores = FPlatformMisc::NumberOfCores();
	int NumberofCoreIncludingHyperthreads = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
	int NumberOfThreads = 0;

	if (NumberofCoreIncludingHyperthreads > NumberOfCores)
	{
		NumberOfThreads = NumberofCoreIncludingHyperthreads - 2;
	}
	else
	{
		NumberOfThreads = NumberOfCores - 1;
	}
	//int32 MaxWorkerThreadsWanted = IsRunningDedicatedServer() ? MaxServerWorkerThreads : MaxWorkerThreads;
	int MaxWorkerThreadsWanted = MaxWorkerThreads;
	return YMath::Max(YMath::Min(NumberOfThreads, MaxWorkerThreadsWanted), 1);
}

void FWindowsPlatformProcess::SetThreadAffinityMask(unsigned long long AffinityMask)
{
	if (AffinityMask != FPlatformAffinity::GetNoAffinityMask())
	{
		::SetThreadAffinityMask(::GetCurrentThread(), (DWORD_PTR)AffinityMask);
	}
}

FRunnableThread* FWindowsPlatformProcess::CreateRunnableThread()
{
	return new FRunnableThreadWin();
}
