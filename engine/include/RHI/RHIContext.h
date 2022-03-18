// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHIContext.h: Interface for RHI Contexts
=============================================================================*/

#pragma once

//#include "Misc/AssertionMacros.h"
#include <cassert>
#include "Math/YColor.h"
#include "Math/IntPoint.h"
#include "Math/IntRect.h"
#include "Math/Float16Color.h"
#include "RHIResources.h"

class FRHIDepthRenderTargetView;
class FRHIRenderTargetView;
class FRHISetRenderTargetsInfo;
struct FResolveParams;
struct FViewportBounds;
enum class EAsyncComputeBudget;
enum class EResourceTransitionAccess;
enum class EResourceTransitionPipeline;


inline FBoundShaderStateRHIRef RHICreateBoundShaderState(
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FVertexShaderRHIParamRef VertexShader,
	FHullShaderRHIParamRef HullShader,
	FDomainShaderRHIParamRef DomainShader,
	FPixelShaderRHIParamRef PixelShader,
	FGeometryShaderRHIParamRef GeometryShader
);

/** Context that is capable of doing Compute work.  Can be async or compute on the gfx pipe. */
class IRHIComputeContext
{
public:
	/**
	* Compute queue will wait for the fence to be written before continuing.
	*/
	virtual void RHIWaitComputeFence(FComputeFenceRHIParamRef InFence) = 0;

	/**
	*Sets the current compute shader.
	*/
	virtual void RHISetComputeShader(FComputeShaderRHIParamRef ComputeShader) = 0;

	virtual void RHISetComputePipelineState(FRHIComputePipelineState* ComputePipelineState)
	{
		if (ComputePipelineState)
		{
			FRHIComputePipelineStateFallback* FallbackState = static_cast<FRHIComputePipelineStateFallback*>(ComputePipelineState);
			RHISetComputeShader(FallbackState->GetComputeShader());
		}
	}

	virtual void RHIDispatchComputeShader(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) = 0;

	virtual void RHIDispatchIndirectComputeShader(FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset) = 0;

	virtual void RHISetAsyncComputeBudget(EAsyncComputeBudget Budget) = 0;

	/**
	* Explicitly transition a UAV from readable -> writable by the GPU or vice versa.
	* Also explicitly states which pipeline the UAV can be used on next.  For example, if a Compute job just wrote this UAV for a Pixel shader to read
	* you would do EResourceTransitionAccess::Readable and EResourceTransitionPipeline::EComputeToGfx
	*
	* @param TransitionType - direction of the transition
	* @param EResourceTransitionPipeline - How this UAV is transitioning between Gfx and Compute, if at all.
	* @param InUAVs - array of UAV objects to transition
	* @param NumUAVs - number of UAVs to transition
	* @param WriteComputeFence - Optional ComputeFence to write as part of this transition
	*/
	virtual void RHITransitionResources(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32_t NumUAVs, FComputeFenceRHIParamRef WriteComputeFence) = 0;

	/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
	virtual void RHISetShaderTexture(FComputeShaderRHIParamRef PixelShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) = 0;

	/**
	* Sets sampler state.
	* @param GeometryShader	The geometry shader to set the sampler for.
	* @param SamplerIndex		The index of the sampler.
	* @param NewState			The new sampler state.
	*/
	virtual void RHISetShaderSampler(FComputeShaderRHIParamRef ComputeShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) = 0;

	/**
	* Sets a compute shader UAV parameter.
	* @param ComputeShader	The compute shader to set the UAV for.
	* @param UAVIndex		The index of the UAVIndex.
	* @param UAV			The new UAV.
	*/
	virtual void RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV) = 0;

	/**
	* Sets a compute shader counted UAV parameter and initial count
	* @param ComputeShader	The compute shader to set the UAV for.
	* @param UAVIndex		The index of the UAVIndex.
	* @param UAV			The new UAV.
	* @param InitialCount	The initial number of items in the UAV.
	*/
	virtual void RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32_t InitialCount) = 0;

	virtual void RHISetShaderResourceViewParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) = 0;

	virtual void RHISetShaderUniformBuffer(FComputeShaderRHIParamRef ComputeShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) = 0;

	virtual void RHISetShaderParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) = 0;

	virtual void RHIPushEvent(const TCHAR* Name, FColor Color) = 0;

	virtual void RHIPopEvent() = 0;

	/**
	* Submit the current command buffer to the GPU if possible.
	*/
	virtual void RHISubmitCommandsHint() = 0;

	/**
	 * Some RHI implementations (OpenGL) cache render state internally
	 * Signal to RHI that cached state is no longer valid
	 */
	virtual void RHIInvalidateCachedState() {};
};

// These states are now set by the Pipeline State Object and are now deprecated
class IRHIDeprecatedContext
{
public:
	/**
	* Set bound shader state. This will set the vertex decl/shader, and pixel shader
	* @param BoundShaderState - state resource
	*/
	virtual void RHISetBoundShaderState(FBoundShaderStateRHIParamRef BoundShaderState) = 0;

	virtual void RHISetDepthStencilState(FDepthStencilStateRHIParamRef NewState, uint32_t StencilRef) = 0;

	virtual void RHISetRasterizerState(FRasterizerStateRHIParamRef NewState) = 0;

	// Allows to set the blend state, parameter can be created with RHICreateBlendState()
	virtual void RHISetBlendState(FBlendStateRHIParamRef NewState, const FLinearColor& BlendFactor) = 0;

	virtual void RHIEnableDepthBoundsTest(bool bEnable) = 0;
};

/** The interface RHI command context. Sometimes the RHI handles these. On platforms that can processes command lists in parallel, it is a separate object. */
class IRHICommandContext : public IRHIComputeContext, public IRHIDeprecatedContext
{
public:
	virtual ~IRHICommandContext()
	{
	}

	/**
	* Compute queue will wait for the fence to be written before continuing.
	*/
	virtual void RHIWaitComputeFence(FComputeFenceRHIParamRef InFence) override
	{
		if (InFence)
		{
			//checkf(InFence->GetWriteEnqueued(), TEXT("ComputeFence: %s waited on before being written. This will hang the GPU."), *InFence->GetName().ToString());
			assert(InFence->GetWriteEnqueued());
		}
	}

	/**
	*Sets the current compute shader.  Mostly for compliance with platforms
	*that require shader setting before resource binding.
	*/
	virtual void RHISetComputeShader(FComputeShaderRHIParamRef ComputeShader) = 0;

	virtual void RHIDispatchComputeShader(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) = 0;

	virtual void RHIDispatchIndirectComputeShader(FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset) = 0;

	virtual void RHISetAsyncComputeBudget(EAsyncComputeBudget Budget) override
	{
	}

	virtual void RHIAutomaticCacheFlushAfterComputeShader(bool bEnable) = 0;

	virtual void RHIFlushComputeShaderCache() = 0;

	// Useful when used with geometry shader (emit polygons to different viewports), otherwise SetViewPort() is simpler
	// @param Count >0
	// @param Data must not be 0
	virtual void RHISetMultipleViewports(uint32_t Count, const FViewportBounds* Data) = 0;

	/** Clears a UAV to the multi-component value provided. */
	virtual void RHIClearTinyUAV(FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI, const uint32_t* Values) = 0;

	/**
	* Resolves from one texture to another.
	* @param SourceTexture - texture to resolve from, 0 is silenty ignored
	* @param DestTexture - texture to resolve to, 0 is silenty ignored
	* @param ResolveParams - optional resolve params
	*/
	virtual void RHICopyToResolveTarget(FTextureRHIParamRef SourceTexture, FTextureRHIParamRef DestTexture, const FResolveParams& ResolveParams) = 0;

	/**
	* Explicitly transition a texture resource from readable -> writable by the GPU or vice versa.
	* We know rendertargets are only used as rendered targets on the Gfx pipeline, so these transitions are assumed to be implemented such
	* Gfx->Gfx and Gfx->Compute pipeline transitions are both handled by this call by the RHI implementation.  Hence, no pipeline parameter on this call.
	*
	* @param TransitionType - direction of the transition
	* @param InTextures - array of texture objects to transition
	* @param NumTextures - number of textures to transition
	*/
	virtual void RHITransitionResources(EResourceTransitionAccess TransitionType, FTextureRHIParamRef* InTextures, int32_t NumTextures)
	{
		if (TransitionType == EResourceTransitionAccess::EReadable)
		{
			const FResolveParams ResolveParams;
			for (int32_t i = 0; i < NumTextures; ++i)
			{
				RHICopyToResolveTarget(InTextures[i], InTextures[i], ResolveParams);
			}
		}
	}

	/**
	* Explicitly transition a UAV from readable -> writable by the GPU or vice versa.
	* Also explicitly states which pipeline the UAV can be used on next.  For example, if a Compute job just wrote this UAV for a Pixel shader to read
	* you would do EResourceTransitionAccess::Readable and EResourceTransitionPipeline::EComputeToGfx
	*
	* @param TransitionType - direction of the transition
	* @param EResourceTransitionPipeline - How this UAV is transitioning between Gfx and Compute, if at all.
	* @param InUAVs - array of UAV objects to transition
	* @param NumUAVs - number of UAVs to transition
	* @param WriteComputeFence - Optional ComputeFence to write as part of this transition
	*/
	virtual void RHITransitionResources(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32_t NumUAVs, FComputeFenceRHIParamRef WriteComputeFence)
	{
		if (WriteComputeFence)
		{
			WriteComputeFence->WriteFence();
		}
	}

	void RHITransitionResources(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32_t NumUAVs)
	{
		RHITransitionResources(TransitionType, TransitionPipeline, InUAVs, NumUAVs, nullptr);
	}


	virtual void RHIBeginRenderQuery(FRenderQueryRHIParamRef RenderQuery) = 0;

	virtual void RHIEndRenderQuery(FRenderQueryRHIParamRef RenderQuery) = 0;

	virtual void RHISubmitCommandsHint() = 0;

	// Not all RHIs need this (Mobile specific)
	virtual void RHIDiscardRenderTargets(bool Depth, bool Stencil, uint32_t ColorBitMask) {};

	// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
	virtual void RHIBeginDrawingViewport(FViewportRHIParamRef Viewport, FTextureRHIParamRef RenderTargetRHI) = 0;

	// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
	virtual void RHIEndDrawingViewport(FViewportRHIParamRef Viewport, bool bPresent, bool bLockToVsync) = 0;

	// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
	virtual void RHIBeginFrame() = 0;

	// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
	virtual void RHIEndFrame() = 0;

	/**
	* Signals the beginning of scene rendering. The RHI makes certain caching assumptions between
	* calls to BeginScene/EndScene. Currently the only restriction is that you can't update texture
	* references.
	*/
	// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
	virtual void RHIBeginScene() = 0;

	/**
	* Signals the end of scene rendering. See RHIBeginScene.
	*/
	// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
	virtual void RHIEndScene() = 0;

	/**
	* Signals the beginning and ending of rendering to a resource to be used in the next frame on a multiGPU system
	*/
	virtual void RHIBeginUpdateMultiFrameResource(FTextureRHIParamRef Texture)
	{
		/* empty default implementation */
	}

	virtual void RHIEndUpdateMultiFrameResource(FTextureRHIParamRef Texture)
	{
		/* empty default implementation */
	}

	virtual void RHIBeginUpdateMultiFrameResource(FUnorderedAccessViewRHIParamRef UAV)
	{
		/* empty default implementation */
	}

	virtual void RHIEndUpdateMultiFrameResource(FUnorderedAccessViewRHIParamRef UAV)
	{
		/* empty default implementation */
	}

	virtual void RHISetStreamSource(uint32_t StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32_t Offset) = 0;

	// @param MinX including like Win32 RECT
	// @param MinY including like Win32 RECT
	// @param MaxX excluding like Win32 RECT
	// @param MaxY excluding like Win32 RECT
	virtual void RHISetViewport(uint32_t MinX, uint32_t MinY, float MinZ, uint32_t MaxX, uint32_t MaxY, float MaxZ) = 0;

	virtual void RHISetStereoViewport(uint32_t LeftMinX, uint32_t RightMinX, uint32_t LeftMinY, uint32_t RightMinY, float MinZ, uint32_t LeftMaxX, uint32_t RightMaxX, uint32_t LeftMaxY, uint32_t RightMaxY, float MaxZ)
	{
		/* empty default implementation */
	}

	// @param MinX including like Win32 RECT
	// @param MinY including like Win32 RECT
	// @param MaxX excluding like Win32 RECT
	// @param MaxY excluding like Win32 RECT
	virtual void RHISetScissorRect(bool bEnable, uint32_t MinX, uint32_t MinY, uint32_t MaxX, uint32_t MaxY) = 0;

	/**
	* This will set most relevant pipeline state. Legacy APIs are expected to set corresponding disjoint state as well.
	* @param GraphicsShaderState - the graphics pipeline state
	* This implementation is only in place while we transition/refactor.
	*/
	virtual void RHISetGraphicsPipelineState(FGraphicsPipelineStateRHIParamRef GraphicsState)
	{
		FRHIGraphicsPipelineStateFallBack* FallbackGraphicsState = static_cast<FRHIGraphicsPipelineStateFallBack*>(GraphicsState);

		auto& PsoInit = FallbackGraphicsState->Initializer;

		RHISetBoundShaderState(
			RHICreateBoundShaderState(
				PsoInit.BoundShaderState.VertexDeclarationRHI,
				PsoInit.BoundShaderState.VertexShaderRHI,
				PsoInit.BoundShaderState.HullShaderRHI,
				PsoInit.BoundShaderState.DomainShaderRHI,
				PsoInit.BoundShaderState.PixelShaderRHI,
				PsoInit.BoundShaderState.GeometryShaderRHI
			).GetReference()
		);

		RHISetDepthStencilState(FallbackGraphicsState->Initializer.DepthStencilState, 0);
		RHISetRasterizerState(FallbackGraphicsState->Initializer.RasterizerState);
		RHISetBlendState(FallbackGraphicsState->Initializer.BlendState, FLinearColor(1.0f, 1.0f, 1.0f));
		if (GSupportsDepthBoundsTest)
		{
			RHIEnableDepthBoundsTest(FallbackGraphicsState->Initializer.bDepthBounds);
		}
	}

	/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
	virtual void RHISetShaderTexture(FVertexShaderRHIParamRef VertexShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) = 0;

	/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
	virtual void RHISetShaderTexture(FHullShaderRHIParamRef HullShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) = 0;

	/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
	virtual void RHISetShaderTexture(FDomainShaderRHIParamRef DomainShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) = 0;

	/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
	virtual void RHISetShaderTexture(FGeometryShaderRHIParamRef GeometryShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) = 0;

	/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
	virtual void RHISetShaderTexture(FPixelShaderRHIParamRef PixelShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) = 0;

	/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
	virtual void RHISetShaderTexture(FComputeShaderRHIParamRef PixelShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) = 0;

	/**
	* Sets sampler state.
	* @param GeometryShader	The geometry shader to set the sampler for.
	* @param SamplerIndex		The index of the sampler.
	* @param NewState			The new sampler state.
	*/
	virtual void RHISetShaderSampler(FComputeShaderRHIParamRef ComputeShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) = 0;

	/**
	* Sets sampler state.
	* @param GeometryShader	The geometry shader to set the sampler for.
	* @param SamplerIndex		The index of the sampler.
	* @param NewState			The new sampler state.
	*/
	virtual void RHISetShaderSampler(FVertexShaderRHIParamRef VertexShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) = 0;

	/**
	* Sets sampler state.
	* @param GeometryShader	The geometry shader to set the sampler for.
	* @param SamplerIndex		The index of the sampler.
	* @param NewState			The new sampler state.
	*/
	virtual void RHISetShaderSampler(FGeometryShaderRHIParamRef GeometryShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) = 0;

	/**
	* Sets sampler state.
	* @param GeometryShader	The geometry shader to set the sampler for.
	* @param SamplerIndex		The index of the sampler.
	* @param NewState			The new sampler state.
	*/
	virtual void RHISetShaderSampler(FDomainShaderRHIParamRef DomainShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) = 0;

	/**
	* Sets sampler state.
	* @param GeometryShader	The geometry shader to set the sampler for.
	* @param SamplerIndex		The index of the sampler.
	* @param NewState			The new sampler state.
	*/
	virtual void RHISetShaderSampler(FHullShaderRHIParamRef HullShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) = 0;

	/**
	* Sets sampler state.
	* @param GeometryShader	The geometry shader to set the sampler for.
	* @param SamplerIndex		The index of the sampler.
	* @param NewState			The new sampler state.
	*/
	virtual void RHISetShaderSampler(FPixelShaderRHIParamRef PixelShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) = 0;

	/**
	* Sets a compute shader UAV parameter.
	* @param ComputeShader	The compute shader to set the UAV for.
	* @param UAVIndex		The index of the UAVIndex.
	* @param UAV			The new UAV.
	*/
	virtual void RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV) = 0;

	/**
	* Sets a compute shader counted UAV parameter and initial count
	* @param ComputeShader	The compute shader to set the UAV for.
	* @param UAVIndex		The index of the UAVIndex.
	* @param UAV			The new UAV.
	* @param InitialCount	The initial number of items in the UAV.
	*/
	virtual void RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32_t InitialCount) = 0;

	virtual void RHISetShaderResourceViewParameter(FPixelShaderRHIParamRef PixelShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) = 0;

	virtual void RHISetShaderResourceViewParameter(FVertexShaderRHIParamRef VertexShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) = 0;

	virtual void RHISetShaderResourceViewParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) = 0;

	virtual void RHISetShaderResourceViewParameter(FHullShaderRHIParamRef HullShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) = 0;

	virtual void RHISetShaderResourceViewParameter(FDomainShaderRHIParamRef DomainShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) = 0;

	virtual void RHISetShaderResourceViewParameter(FGeometryShaderRHIParamRef GeometryShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) = 0;

	virtual void RHISetShaderUniformBuffer(FVertexShaderRHIParamRef VertexShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) = 0;

	virtual void RHISetShaderUniformBuffer(FHullShaderRHIParamRef HullShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) = 0;

	virtual void RHISetShaderUniformBuffer(FDomainShaderRHIParamRef DomainShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) = 0;

	virtual void RHISetShaderUniformBuffer(FGeometryShaderRHIParamRef GeometryShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) = 0;

	virtual void RHISetShaderUniformBuffer(FPixelShaderRHIParamRef PixelShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) = 0;

	virtual void RHISetShaderUniformBuffer(FComputeShaderRHIParamRef ComputeShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) = 0;

	virtual void RHISetShaderParameter(FVertexShaderRHIParamRef VertexShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) = 0;

	virtual void RHISetShaderParameter(FPixelShaderRHIParamRef PixelShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) = 0;

	virtual void RHISetShaderParameter(FHullShaderRHIParamRef HullShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) = 0;

	virtual void RHISetShaderParameter(FDomainShaderRHIParamRef DomainShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) = 0;

	virtual void RHISetShaderParameter(FGeometryShaderRHIParamRef GeometryShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) = 0;

	virtual void RHISetShaderParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) = 0;

	virtual void RHISetStencilRef(uint32_t StencilRef) {}

	virtual void RHISetBlendFactor(const FLinearColor& BlendFactor) {}

	virtual void RHISetRenderTargets(uint32_t NumSimultaneousRenderTargets, const FRHIRenderTargetView* NewRenderTargets, const FRHIDepthRenderTargetView* NewDepthStencilTarget, uint32_t NumUAVs, const FUnorderedAccessViewRHIParamRef* UAVs) = 0;

	virtual void RHISetRenderTargetsAndClear(const FRHISetRenderTargetsInfo& RenderTargetsInfo) = 0;

	// Bind the clear state of the currently set rendertargets.  This is used by platforms which
	// need the state of the target when finalizing a hardware clear or a resource transition to SRV
	// The explicit bind is needed to support parallel rendering (propagate state between contexts).
	virtual void RHIBindClearMRTValues(bool bClearColor, bool bClearDepth, bool bClearStencil) {}

	virtual void RHIDrawPrimitive(uint32_t PrimitiveType, uint32_t BaseVertexIndex, uint32_t NumPrimitives, uint32_t NumInstances) = 0;

	virtual void RHIDrawPrimitiveIndirect(uint32_t PrimitiveType, FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset) = 0;

	virtual void RHIDrawIndexedIndirect(FIndexBufferRHIParamRef IndexBufferRHI, uint32_t PrimitiveType, FStructuredBufferRHIParamRef ArgumentsBufferRHI, int32_t DrawArgumentsIndex, uint32_t NumInstances) = 0;

	// @param NumPrimitives need to be >0 
	virtual void RHIDrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32_t PrimitiveType, int32_t BaseVertexIndex, uint32_t FirstInstance, uint32_t NumVertices, uint32_t StartIndex, uint32_t NumPrimitives, uint32_t NumInstances) = 0;

	virtual void RHIDrawIndexedPrimitiveIndirect(uint32_t PrimitiveType, FIndexBufferRHIParamRef IndexBuffer, FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset) = 0;

	/**
	* Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawPrimitiveUP
	* @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
	* @param NumPrimitives The number of primitives in the VertexData buffer
	* @param NumVertices The number of vertices to be written
	* @param VertexDataStride Size of each vertex
	* @param OutVertexData Reference to the allocated vertex memory
	*/
	virtual void RHIBeginDrawPrimitiveUP(uint32_t PrimitiveType, uint32_t NumPrimitives, uint32_t NumVertices, uint32_t VertexDataStride, void*& OutVertexData) = 0;

	/**
	* Draw a primitive using the vertex data populated since RHIBeginDrawPrimitiveUP and clean up any memory as needed
	*/
	virtual void RHIEndDrawPrimitiveUP() = 0;

	/**
	* Preallocate memory or get a direct command stream pointer to fill up for immediate rendering . This avoids memcpys below in DrawIndexedPrimitiveUP
	* @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
	* @param NumPrimitives The number of primitives in the VertexData buffer
	* @param NumVertices The number of vertices to be written
	* @param VertexDataStride Size of each vertex
	* @param OutVertexData Reference to the allocated vertex memory
	* @param MinVertexIndex The lowest vertex index used by the index buffer
	* @param NumIndices Number of indices to be written
	* @param IndexDataStride Size of each index (either 2 or 4 bytes)
	* @param OutIndexData Reference to the allocated index memory
	*/
	virtual void RHIBeginDrawIndexedPrimitiveUP(uint32_t PrimitiveType, uint32_t NumPrimitives, uint32_t NumVertices, uint32_t VertexDataStride, void*& OutVertexData, uint32_t MinVertexIndex, uint32_t NumIndices, uint32_t IndexDataStride, void*& OutIndexData) = 0;

	/**
	* Draw a primitive using the vertex and index data populated since RHIBeginDrawIndexedPrimitiveUP and clean up any memory as needed
	*/
	virtual void RHIEndDrawIndexedPrimitiveUP() = 0;

	/**
	* Sets Depth Bounds range with the given min/max depth.
	* @param MinDepth	The minimum depth for depth bounds test
	* @param MaxDepth	The maximum depth for depth bounds test.
	*					The valid values for fMinDepth and fMaxDepth are such that 0 <= fMinDepth <= fMaxDepth <= 1
	*/
	virtual void RHISetDepthBounds(float MinDepth, float MaxDepth) = 0;

	virtual void RHIPushEvent(const TCHAR* Name, FColor Color) = 0;

	virtual void RHIPopEvent() = 0;

	virtual void RHIUpdateTextureReference(FTextureReferenceRHIParamRef TextureRef, FTextureRHIParamRef NewTexture) = 0;

	virtual void RHIBeginRenderPass(const FRHIRenderPassInfo& InInfo, const TCHAR* InName)
	{
		// Fallback...
		InInfo.Validate();

		if (InInfo.bGeneratingMips)
		{
			FRHITexture* Textures[MaxSimultaneousRenderTargets];
			FRHITexture** LastTexture = Textures;
			for (int32_t Index = 0; Index < MaxSimultaneousRenderTargets; ++Index)
			{
				if (!InInfo.ColorRenderTargets[Index].RenderTarget)
				{
					break;
				}

				*LastTexture = InInfo.ColorRenderTargets[Index].RenderTarget;
				++LastTexture;
			}

			//Use RWBarrier since we don't transition individual subresources.  Basically treat the whole texture as R/W as we walk down the mip chain.
			int32_t NumTextures = (int32_t)(LastTexture - Textures);
			if (NumTextures)
			{
				RHITransitionResources(EResourceTransitionAccess::ERWSubResBarrier, Textures, NumTextures);
			}
		}

		FRHISetRenderTargetsInfo RTInfo;
		InInfo.ConvertToRenderTargetsInfo(RTInfo);
		RHISetRenderTargetsAndClear(RTInfo);

		RenderPassInfo = InInfo;
	}

	virtual void RHIEndRenderPass()
	{
		for (int32_t Index = 0; Index < MaxSimultaneousRenderTargets; ++Index)
		{
			if (!RenderPassInfo.ColorRenderTargets[Index].RenderTarget)
			{
				break;
			}
			if (RenderPassInfo.ColorRenderTargets[Index].ResolveTarget)
			{
				RHICopyToResolveTarget(RenderPassInfo.ColorRenderTargets[Index].RenderTarget, RenderPassInfo.ColorRenderTargets[Index].ResolveTarget, RenderPassInfo.ResolveParameters);
			}
		}

		if (RenderPassInfo.DepthStencilRenderTarget.DepthStencilTarget && RenderPassInfo.DepthStencilRenderTarget.ResolveTarget)
		{
			RHICopyToResolveTarget(RenderPassInfo.DepthStencilRenderTarget.DepthStencilTarget, RenderPassInfo.DepthStencilRenderTarget.ResolveTarget, RenderPassInfo.ResolveParameters);
		}
	}

	virtual void RHIBeginComputePass(const TCHAR* InName)
	{
		RHISetRenderTargets(0, nullptr, nullptr, 0, nullptr);
	}

	virtual void RHIEndComputePass()
	{
	}

	virtual void RHICopyTexture(FTextureRHIParamRef SourceTexture, FTextureRHIParamRef DestTexture, const FRHICopyTextureInfo& CopyInfo)
	{
		const bool bIsCube = SourceTexture->GetTextureCube() != nullptr;
		const bool bAllCubeFaces = bIsCube && (CopyInfo.NumArraySlices % 6) == 0;
		const int32_t NumArraySlices = bAllCubeFaces ? CopyInfo.NumArraySlices / 6 : CopyInfo.NumArraySlices;
		const int32_t NumFaces = bAllCubeFaces ? 6 : 1;
		for (int32_t ArrayIndex = 0; ArrayIndex < NumArraySlices; ++ArrayIndex)
		{
			int32_t SourceArrayIndex = CopyInfo.SourceArraySlice + ArrayIndex;
			int32_t DestArrayIndex = CopyInfo.DestArraySlice + ArrayIndex;
			for (int32_t FaceIndex = 0; FaceIndex < NumFaces; ++FaceIndex)
			{
				FResolveParams ResolveParams(FResolveRect(),
					bIsCube ? (ECubeFace)FaceIndex : CubeFace_PosX,
					CopyInfo.MipIndex,
					SourceArrayIndex,
					DestArrayIndex
				);
				RHICopyToResolveTarget(SourceTexture, DestTexture, ResolveParams);
			}
		}
	}

	protected:
		FRHIRenderPassInfo RenderPassInfo;
};
