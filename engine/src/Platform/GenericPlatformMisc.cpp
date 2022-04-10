#include "Platform/YGenericPlatformMisc.h"
#include "Platform/YPlatformProcess.h"
#include "Platform/YPlatfomMisc.h"
#include "Engine/CoreGlobals.h"


int FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads()
{
	return -1;
}

int FGenericPlatformMisc::NumberOfCores()
{
	return -1;
}

void FGenericPlatformMisc::SetThreadAffinityMask(unsigned long long AffinityMask)
{

}

bool FGenericPlatformMisc::UseRenderThread()
{
	// single core devices shouldn't use it (unless that platform overrides this function - maybe RT could be required?)
	if (FPlatformMisc::NumberOfCoresIncludingHyperthreads() < 2)
	{
		return false;
	}

	// if the platform doesn't allow threading at all, we really can't use it
	if (FPlatformProcess::SupportsMultithreading() == false)
	{
		return false;
	}

	// dedicated servers should not use a rendering thread
	//if (IsRunningDedicatedServer())
	//{
	//	return false;
	//}

	// allow if not overridden
	return true;
}

void FGenericPlatformMisc::RequestExit(bool Force)
{
	if (Force)
	{
		abort();
	}
	else
	{
		GIsRequestingExit = true;
	}
}
