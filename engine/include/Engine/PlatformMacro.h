#pragma once
#include <cassert>

#ifndef WITH_EDITOR
#define WITH_EDITOR 0
#endif

#define  PLATFORM_RHITHREAD_DEFAULT_BYPASS WITH_EDITOR

#define  PLATFORM_64BITS 1
#define  PLATFORM_HAS_64BIT_ATOMICS 1
#define  PLATFORM_HAS_128BIT_ATOMICS 0
#define  PLATFORM_MAC 0
#define PLATFORM_SUPPORTS_VIRTUAL_TEXTURES 0
#if defined DEBUG || defined _DEBUG
#define  UE_BUILD_DEBUG 1
#define UE_BUILD_SHIPPING 0
#define  UE_BUILD_DEVELOPMENT 1
#define  UE_BUILD_TEST 0
#else
#define  UE_BUILD_DEBUG 0
#define UE_BUILD_SHIPPING 1
#define  UE_BUILD_DEVELOPMENT 0
#define  UE_BUILD_TEST 0
#endif
#ifdef _WIN32
#define  PLATFORM_WINDOWS 1
#elif
#define  PLATFORM_WINDOWS 0
#endif

#define check assert
#define ensure assert
#define checkf assert
#define checkSlow assert
#define DO_CHECK 1
#ifndef FORCEINLINE
#define  FORCEINLINE inline
#endif

#ifndef FORCEINLINE_DEBUGGABLE
#define  FORCEINLINE_DEBUGGABLE FORCEINLINE
#endif

#define STAT(a) 
#define STATS 0

#define UE_BUILD_TEST 0
#define PLATFORM_MAC 0
