#pragma once
#include <stdint.h>
#include <cassert>
#include <intsafe.h>
#include <cstring>
#include "Engine/PlatformMacro.h"

typedef int8_t int8;
typedef uint8_t uint8;
typedef int32_t int32;
typedef int16_t int16;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;



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
#define DECLARE_CYCLE_STAT(x)
#define QUICK_SCOPE_CYCLE_COUNTER(x)
#define  SCOPE_TIME_GUARD(x)
#define  SCOPE_CYCLE_COUNTER(x)

typedef int TStatId;
