#pragma once
// Alignment.
#if defined(__clang__)
#define GCC_PACK(n) __attribute__((packed,aligned(n)))
#define GCC_ALIGN(n) __attribute__((aligned(n)))
#if defined(_MSC_VER)
#define MS_ALIGN(n) __declspec(align(n)) // With -fms-extensions, Clang will accept either alignment attribute
#endif
#else
#define MS_ALIGN(n) __declspec(align(n))
#endif

#define GCC_ALIGN(n) 

// DLL export and import definitions
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)

#ifndef USE_SECURE_CRT
#define USE_SECURE_CRT 0
#endif

// Prefetch
#define PLATFORM_CACHE_LINE_SIZE	128
