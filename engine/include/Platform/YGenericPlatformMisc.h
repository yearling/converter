#pragma once
struct FGenericPlatformMisc
{
	inline  static void MemoryBarrier() {};
	static bool UseRenderThread();
	static int NumberOfCoresIncludingHyperthreads();
	static int NumberOfCores();

	static void SetThreadAffinityMask(unsigned long long AffinityMask);
	static void RequestExit(bool Force);
};


