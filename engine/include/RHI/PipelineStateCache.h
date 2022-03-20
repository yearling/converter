// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PipelineStateCache.h: Pipeline state cache definition.
=============================================================================*/

#pragma once

#include "Engine/YCommonHeader.h"
#include "RHI/RHI.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"

// Utility flags for modifying render target behavior on a PSO
enum class EApplyRendertargetOption : int
{
	DoNothing = 0,			// Just use the PSO from initializer's values, no checking and no modifying (faster)
	ForceApply = 1 << 0,	// Always apply the Cmd List's Render Target formats into the PSO initializer
	CheckApply = 1 << 1,	// Verify that the PSO's RT formats match the last Render Target formats set into the CmdList
};

//ENUM_CLASS_FLAGS(EApplyRendertargetOption);

extern  void SetComputePipelineState(FRHICommandList& RHICmdList, FRHIComputeShader* ComputeShader);
extern  void SetGraphicsPipelineState(FRHICommandList& RHICmdList, const FGraphicsPipelineStateInitializer& Initializer, EApplyRendertargetOption ApplyFlags = EApplyRendertargetOption::CheckApply);

namespace PipelineStateCache
{
	extern  FComputePipelineState*	GetAndOrCreateComputePipelineState(FRHICommandList& RHICmdList, FRHIComputeShader* ComputeShader);

	extern  FGraphicsPipelineState*	GetAndOrCreateGraphicsPipelineState(FRHICommandList& RHICmdList, const FGraphicsPipelineStateInitializer& OriginalInitializer, EApplyRendertargetOption ApplyFlags);

	/* Evicts unused state entries based on r.pso.evictiontime time. Called in RHICommandList::BeginFrame */
	extern  void FlushResources();

	/* Clears all pipeline cached state. Called on shutdown, calling GetAndOrCreate after this will recreate state */
	extern  void Shutdown();
}
