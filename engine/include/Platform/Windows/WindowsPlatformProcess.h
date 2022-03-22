// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Platform/YGenericPlatformProcess.h"
#include <windows.h>
#pragma warning(disable : 4251) 
#define PLATFORM_CACHE_LINE_SIZE	128
class FEvent;
class FRunnableThread;


/**
* Windows implementation of the Process OS functions.
**/
struct  FWindowsPlatformProcess:public YGenericPlatformProcess
{
	
	static void Sleep(float Seconds);
	static void SleepNoStats(float Seconds);
	static void SleepInfinite();
	static class FEvent* CreateSynchEvent(bool bIsManualReset = false);
	/**
	* Gets an event from the pool or creates a new one if necessary.
	*
	* @param bIsManualReset Whether the event requires manual reseting or not.
	* @return An event, or nullptr none could be created.
	* @see CreateSynchEvent, ReturnSynchEventToPool
	*/
	static class FEvent* GetSynchEventFromPool(bool bIsManualReset = false);

	/**
	* Returns an event to the pool.
	*
	* @param Event The event to return.
	* @see CreateSynchEvent, GetSynchEventFromPool
	*/
	static void ReturnSynchEventToPool(FEvent* Event);

	/**
	* Creates the platform-specific runnable thread. This should only be called from FRunnableThread::Create.
	*
	* @return The newly created thread
	*/
	static class FRunnableThread* CreateRunnableThread();
	static bool SupportsMultithreading();
	static int NumberOfWorkerThreadsToSpawn();
	static void SetThreadAffinityMask(unsigned long long AffinityMask);
	static void SetRealTimeMode() { }
};

typedef FWindowsPlatformProcess FPlatformProcess;
