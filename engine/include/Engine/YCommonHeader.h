#pragma once
#include <stdint.h>
#include <cassert>
#include <intsafe.h>
#include <cstring>
typedef int8_t int8;
typedef uint8_t uint8;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

#define check assert
#define ensure assert
#define checkf assert
#define checkSlow assert

#ifndef FORCEINLINE
#define  FORCEINLINE inline
#endif

#ifndef FORCEINLINE_DEBUGGABLE
#define  FORCEINLINE_DEBUGGABLE FORCEINLINE
#endif

#define STAT(a) 
#define STATS 0
#define UE_BUILD_SHIPPING 0
#define UE_BUILD_TEST 0
#define PLATFORM_MAC 0
#define WITH_EDITOR 1
static FORCEINLINE void* Memzero(void* Dest, SIZE_T Count)
{
	return memset(Dest,0, Count);
}

enum
{
	// Default allocator alignment. If the default is specified, the allocator applies to engine rules.
	// Blocks >= 16 bytes will be 16-byte-aligned, Blocks < 16 will be 8-byte aligned. If the allocator does
	// not support allocation alignment, the alignment will be ignored.
	DEFAULT_ALIGNMENT = 0,

	// Minimum allocator alignment
	MIN_ALIGNMENT = 8,
};

#define  LLM_SCOPE(x) 
#define  PLATFORM_RHITHREAD_DEFAULT_BYPASS 0
