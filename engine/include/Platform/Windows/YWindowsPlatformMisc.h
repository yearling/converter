#pragma once
#include "Platform/YGenericPlatformMisc.h"
#include <xmmintrin.h>
struct  FWindowsPlatformMisc: public FGenericPlatformMisc
{
	inline static void MemoryBarrier() { _mm_sfence(); }
};
typedef FWindowsPlatformMisc FPlatformMisc;