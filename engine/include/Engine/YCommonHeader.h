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
#define  FORCEINLINE inline
#define  FORCEINLINE_DEBUGGABLE inline
#define STAT(a) 

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
