// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MultiGPU.h: Multi-GPU support
=============================================================================*/

#pragma once
#include <cstdint>
#include <cassert>

/** When greater than one, indicates that SLI rendering is enabled */
#if PLATFORM_DESKTOP
#define WITH_SLI 1	// Implicit SLI
#define WITH_MGPU 1	// Explicit MGPU
#define MAX_NUM_GPUS 4
extern  uint32_t GNumExplicitGPUsForRendering;
extern  uint32_t GNumAlternateFrameRenderingGroups;
#else
#define WITH_SLI 0
#define WITH_MGPU 0
#define MAX_NUM_GPUS 1
#define GNumExplicitGPUsForRendering 1
#define GNumAlternateFrameRenderingGroups 1
#endif

/** A mask where each bit is a GPU index. Can not be empty so that non SLI platforms can optimize it to be always 1.  */
struct FRHIGPUMask
{
private:
	uint32_t GPUMask;

public:
	inline explicit FRHIGPUMask(uint32_t InGPUMask) : GPUMask(InGPUMask)
	{
#if WITH_MGPU
		assert(InGPUMask != 0);
#else
		assert(InGPUMask == 1);
#endif
	}

	inline FRHIGPUMask() : GPUMask(FRHIGPUMask::GPU0())
	{
	}

	inline static FRHIGPUMask FromIndex(uint32_t GPUIndex) { return FRHIGPUMask(1 << GPUIndex); }

	inline uint32_t ToIndex() const
	{
#if WITH_MGPU
		assert(HasSingleIndex());
		return FMath::CountTrailingZeros(GPUMask);
#else
		return 0;
#endif
	}

	inline bool HasSingleIndex() const
	{
#if WITH_MGPU
		return FMath::IsPowerOfTwo(GPUMask);
#else
		return true;
#endif
	}

	inline uint32_t GetLastIndex()
	{
#if WITH_MGPU
		return FPlatformMath::FloorLog2(GPUMask);
#else
		return 0;
#endif
	}

	inline uint32_t GetFirstIndex()
	{
#if WITH_MGPU
		return FPlatformMath::CountTrailingZeros(GPUMask);
#else
		return 0;
#endif
	}

	inline bool Contains(uint32_t GPUIndex) const { return (GPUMask & (1 << GPUIndex)) != 0; }
	inline bool Intersects(const FRHIGPUMask& Rhs) const { return (GPUMask & Rhs.GPUMask) != 0; }

	inline bool operator ==(const FRHIGPUMask& Rhs) const { return GPUMask == Rhs.GPUMask; }

	void operator |=(const FRHIGPUMask& Rhs) { GPUMask |= Rhs.GPUMask; }
	void operator &=(const FRHIGPUMask& Rhs) { GPUMask &= Rhs.GPUMask; }
	inline operator uint32_t() const { return GPUMask; }

	inline FRHIGPUMask operator &(const FRHIGPUMask& Rhs) const
	{
		return FRHIGPUMask(GPUMask & Rhs.GPUMask);
	}

	inline FRHIGPUMask operator |(const FRHIGPUMask& Rhs) const
	{
		return FRHIGPUMask(GPUMask | Rhs.GPUMask);
	}

	inline static const FRHIGPUMask GPU0() { return FRHIGPUMask(1); }
	inline static const FRHIGPUMask All() { return FRHIGPUMask((1 << GNumExplicitGPUsForRendering) - 1); }

	struct FIterator
	{
		inline FIterator(const uint32_t InGPUMask) : GPUMask(InGPUMask), FirstGPUIndexInMask(0)
		{
#if WITH_MGPU
			FirstGPUIndexInMask = FPlatformMath::CountTrailingZeros(InGPUMask);
#endif
		}

		inline void operator++()
		{
#if WITH_MGPU
			GPUMask &= ~(1 << FirstGPUIndexInMask);
			FirstGPUIndexInMask = FPlatformMath::CountTrailingZeros(GPUMask);
#else
			GPUMask = 0;
#endif
		}

		inline uint32_t operator*() const { return FirstGPUIndexInMask; }

		inline bool operator !=(const FIterator& Rhs) const { return GPUMask != Rhs.GPUMask; }

	private:
		uint32_t GPUMask;
		unsigned long FirstGPUIndexInMask;
	};

	inline friend FRHIGPUMask::FIterator begin(const FRHIGPUMask& NodeMask) { return FRHIGPUMask::FIterator(NodeMask.GPUMask); }
	inline friend FRHIGPUMask::FIterator end(const FRHIGPUMask& NodeMask) { return FRHIGPUMask::FIterator(0); }
};
