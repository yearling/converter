#pragma once
#include "RHI/DynamicRHI.h"
#include "RHI/RHIContext.h"
#include "RHI/MultiGPU.h"
#include "RHI/RHICommandList.h"
#include "RHI/DirectX11/ComPtr.h"
#include "RHI/DirectX11/D3D11StateCache.h"
#include <d3d11.h>

// DX11 doesn't support higher MSAA count
#define DX_MAX_MSAA_COUNT 8

struct FD3D11GlobalStats
{
	// in bytes, never change after RHI, needed to scale game features
	static int64 GDedicatedVideoMemory;

	// in bytes, never change after RHI, needed to scale game features
	static int64 GDedicatedSystemMemory;

	// in bytes, never change after RHI, needed to scale game features
	static int64 GSharedSystemMemory;

	// In bytes. Never changed after RHI init. Our estimate of the amount of memory that we can use for graphics resources in total.
	static int64 GTotalGraphicsMemory;
};


struct FD3D11Adapter
{
	/** -1 if not supported or FindAdpater() wasn't called. Ideally we would store a pointer to IDXGIAdapter but it's unlikely the adpaters change during engine init. */
	int32 AdapterIndex;
	/** The maximum D3D11 feature level supported. 0 if not supported or FindAdpater() wasn't called */
	D3D_FEATURE_LEVEL MaxSupportedFeatureLevel;

	// constructor
	FD3D11Adapter(int32 InAdapterIndex = -1, D3D_FEATURE_LEVEL InMaxSupportedFeatureLevel = (D3D_FEATURE_LEVEL)0)
		: AdapterIndex(InAdapterIndex)
		, MaxSupportedFeatureLevel(InMaxSupportedFeatureLevel)
	{
	}

	bool IsValid() const
	{
		return MaxSupportedFeatureLevel != (D3D_FEATURE_LEVEL)0 && AdapterIndex >= 0;
	}
};

class YD3D11DynamicRHIModule : public IDynamicRHIModule
{
	public:


		virtual bool IsSupported() override;


		virtual FDynamicRHI* CreateRHI(ERHIFeatureLevel::Type RequestedFeatureLevel = ERHIFeatureLevel::Num) override;
protected:
	void FindAdapter();
	FD3D11Adapter adapter_;
	DXGI_ADAPTER_DESC ChosenDescription;
};
class YD3D11SamplerState :public FRHISamplerState
{
public:
	TRefCountPtr<ID3D11SamplerState> resource_inner;
};


class YD3D11DynamicRHI :public FDynamicRHI, public IRHICommandContext
{

public:
	YD3D11DynamicRHI(IDXGIFactory1* InDXGIFactory1, D3D_FEATURE_LEVEL InFeatureLevel, int32 InChosenAdapter, const DXGI_ADAPTER_DESC& ChosenDescription);
	~YD3D11DynamicRHI() override;

	virtual void Init() override;

	virtual	void InitD3DDevice();
	virtual void PostInit() override;


	virtual void Shutdown() override;


	virtual const TCHAR* GetName() override;


	virtual FSamplerStateRHIRef RHICreateSamplerState(const FSamplerStateInitializerRHI& Initializer) override;


	virtual FRasterizerStateRHIRef RHICreateRasterizerState(const FRasterizerStateInitializerRHI& Initializer) override;


	virtual FDepthStencilStateRHIRef RHICreateDepthStencilState(const FDepthStencilStateInitializerRHI& Initializer) override;


	virtual FBlendStateRHIRef RHICreateBlendState(const FBlendStateInitializerRHI& Initializer) override;


	virtual FVertexDeclarationRHIRef RHICreateVertexDeclaration(const FVertexDeclarationElementList& Elements) override;


	virtual FPixelShaderRHIRef RHICreatePixelShader(const std::vector<uint8>& Code) override;


	virtual FPixelShaderRHIRef RHICreatePixelShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FVertexShaderRHIRef RHICreateVertexShader(const std::vector<uint8>& Code) override;


	virtual FVertexShaderRHIRef RHICreateVertexShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FHullShaderRHIRef RHICreateHullShader(const std::vector<uint8>& Code) override;


	virtual FHullShaderRHIRef RHICreateHullShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FDomainShaderRHIRef RHICreateDomainShader(const std::vector<uint8>& Code) override;


	virtual FDomainShaderRHIRef RHICreateDomainShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FGeometryShaderRHIRef RHICreateGeometryShader(const std::vector<uint8>& Code) override;


	virtual FGeometryShaderRHIRef RHICreateGeometryShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FGeometryShaderRHIRef RHICreateGeometryShaderWithStreamOutput(const std::vector<uint8>& Code, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream) override;


	virtual FGeometryShaderRHIRef RHICreateGeometryShaderWithStreamOutput(const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream, FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual void FlushPendingLogs() override;


	virtual FComputeShaderRHIRef RHICreateComputeShader(const std::vector<uint8>& Code) override;


	virtual FComputeShaderRHIRef RHICreateComputeShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FRHIShaderLibraryRef RHICreateShaderLibrary(EShaderPlatform Platform, std::string const& FilePath, std::string const& Name) override;


	virtual FComputeFenceRHIRef RHICreateComputeFence(const std::string& Name) override;


	virtual FGPUFenceRHIRef RHICreateGPUFence(const std::string& Name) override;


	virtual FStagingBufferRHIRef RHICreateStagingBuffer() override;


	virtual FBoundShaderStateRHIRef RHICreateBoundShaderState(FVertexDeclarationRHIParamRef VertexDeclaration, FVertexShaderRHIParamRef VertexShader, FHullShaderRHIParamRef HullShader, FDomainShaderRHIParamRef DomainShader, FPixelShaderRHIParamRef PixelShader, FGeometryShaderRHIParamRef GeometryShader) override;


	virtual FGraphicsPipelineStateRHIRef RHICreateGraphicsPipelineState(const FGraphicsPipelineStateInitializer& Initializer) override;


	virtual TRefCountPtr<FRHIComputePipelineState> RHICreateComputePipelineState(FRHIComputeShader* ComputeShader) override;


	virtual FGraphicsPipelineStateRHIRef RHICreateGraphicsPipelineState(const FGraphicsPipelineStateInitializer& Initializer, FRHIPipelineBinaryLibraryParamRef PipelineBinary) override;


	virtual TRefCountPtr<FRHIComputePipelineState> RHICreateComputePipelineState(FRHIComputeShader* ComputeShader, FRHIPipelineBinaryLibraryParamRef PipelineBinary) override;


	virtual FUniformBufferRHIRef RHICreateUniformBuffer(const void* Contents, const FRHIUniformBufferLayout& Layout, EUniformBufferUsage Usage) override;


	virtual FIndexBufferRHIRef RHICreateIndexBuffer(uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo) override;


	virtual void* RHILockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer, uint32 Offset, uint32 Size, EResourceLockMode LockMode) override;


	virtual void RHIUnlockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer) override;


	virtual FVertexBufferRHIRef RHICreateVertexBuffer(uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo) override;


	virtual void* RHILockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode) override;


	virtual void RHIUnlockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer) override;


	virtual void RHICopyVertexBuffer(FVertexBufferRHIParamRef SourceBuffer, FVertexBufferRHIParamRef DestBuffer) override;


	virtual FStructuredBufferRHIRef RHICreateStructuredBuffer(uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo) override;


	virtual void* RHILockStructuredBuffer(FStructuredBufferRHIParamRef StructuredBuffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode) override;


	virtual void RHIUnlockStructuredBuffer(FStructuredBufferRHIParamRef StructuredBuffer) override;


	virtual FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView(FStructuredBufferRHIParamRef StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer) override;


	virtual FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView(FTextureRHIParamRef Texture, uint32 MipLevel) override;


	virtual FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView(FVertexBufferRHIParamRef VertexBuffer, uint8 Format) override;


	virtual FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView(FIndexBufferRHIParamRef IndexBuffer, uint8 Format) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView(FStructuredBufferRHIParamRef StructuredBuffer) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView(FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint8 Format) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView(FIndexBufferRHIParamRef Buffer) override;


	virtual uint64 RHICalcTexture2DPlatformSize(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, uint32& OutAlign) override;


	virtual uint64 RHICalcTexture3DPlatformSize(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, uint32& OutAlign) override;


	virtual uint64 RHICalcTextureCubePlatformSize(uint32 Size, uint8 Format, uint32 NumMips, uint32 Flags, uint32& OutAlign) override;


	virtual void RHIGetTextureMemoryStats(FTextureMemoryStats& OutStats) override;


	virtual bool RHIGetTextureMemoryVisualizeData(FColor* TextureData, int32 SizeX, int32 SizeY, int32 Pitch, int32 PixelSize) override;


	virtual FTextureReferenceRHIRef RHICreateTextureReference(FLastRenderTimeContainer* LastRenderTime) override;


	virtual FTexture2DRHIRef RHICreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FTexture2DRHIRef RHICreateTextureExternal2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FStructuredBufferRHIRef RHICreateRTWriteMaskBuffer(FTexture2DRHIParamRef RenderTarget) override;


	virtual FTexture2DRHIRef RHIAsyncCreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 Flags, void** InitialMipData, uint32 NumInitialMips) override;


	virtual void RHICopySharedMips(FTexture2DRHIParamRef DestTexture2D, FTexture2DRHIParamRef SrcTexture2D) override;


	virtual FTexture2DArrayRHIRef RHICreateTexture2DArray(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FTexture3DRHIRef RHICreateTexture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual void RHIGetResourceInfo(FTextureRHIParamRef Ref, FRHIResourceInfo& OutInfo) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView(FTexture2DRHIParamRef Texture2DRHI, uint8 MipLevel) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView(FTexture2DRHIParamRef Texture2DRHI, uint8 MipLevel, uint8 NumMipLevels, uint8 Format) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView(FTexture3DRHIParamRef Texture3DRHI, uint8 MipLevel) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView(FTexture2DArrayRHIParamRef Texture2DArrayRHI, uint8 MipLevel) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView(FTextureCubeRHIParamRef TextureCubeRHI, uint8 MipLevel) override;


	virtual void RHIGenerateMips(FTextureRHIParamRef Texture) override;


	virtual uint32 RHIComputeMemorySize(FTextureRHIParamRef TextureRHI) override;


	virtual FTexture2DRHIRef RHIAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, int32 NewMipCount, int32 NewSizeX, int32 NewSizeY, FThreadSafeCounter* RequestStatus) override;


	virtual ETextureReallocationStatus RHIFinalizeAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted) override;


	virtual ETextureReallocationStatus RHICancelAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted) override;


	virtual void* RHILockTexture2D(FTexture2DRHIParamRef Texture, uint32 MipIndex, EResourceLockMode LockMode, uint32& DestStride, bool bLockWithinMiptail) override;


	virtual void RHIUnlockTexture2D(FTexture2DRHIParamRef Texture, uint32 MipIndex, bool bLockWithinMiptail) override;


	virtual void* RHILockTexture2DArray(FTexture2DArrayRHIParamRef Texture, uint32 TextureIndex, uint32 MipIndex, EResourceLockMode LockMode, uint32& DestStride, bool bLockWithinMiptail) override;


	virtual void RHIUnlockTexture2DArray(FTexture2DArrayRHIParamRef Texture, uint32 TextureIndex, uint32 MipIndex, bool bLockWithinMiptail) override;


	virtual void RHIUpdateTexture2D(FTexture2DRHIParamRef Texture, uint32 MipIndex, const struct FUpdateTextureRegion2D& UpdateRegion, uint32 SourcePitch, const uint8* SourceData) override;


	virtual void RHIUpdateTexture3D(FTexture3DRHIParamRef Texture, uint32 MipIndex, const struct FUpdateTextureRegion3D& UpdateRegion, uint32 SourceRowPitch, uint32 SourceDepthPitch, const uint8* SourceData) override;


	virtual FTextureCubeRHIRef RHICreateTextureCube(uint32 Size, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FTextureCubeRHIRef RHICreateTextureCubeArray(uint32 Size, uint32 ArraySize, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual void* RHILockTextureCubeFace(FTextureCubeRHIParamRef Texture, uint32 FaceIndex, uint32 ArrayIndex, uint32 MipIndex, EResourceLockMode LockMode, uint32& DestStride, bool bLockWithinMiptail) override;


	virtual void RHIUnlockTextureCubeFace(FTextureCubeRHIParamRef Texture, uint32 FaceIndex, uint32 ArrayIndex, uint32 MipIndex, bool bLockWithinMiptail) override;


	virtual void RHIBindDebugLabelName(FTextureRHIParamRef Texture, const TCHAR* Name) override;


	virtual void RHIBindDebugLabelName(FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI, const TCHAR* Name) override;


	virtual void RHIReadSurfaceData(FTextureRHIParamRef Texture, FIntRect Rect, std::vector<FColor>& OutData, FReadSurfaceDataFlags InFlags) override;


	virtual void RHIReadSurfaceData(FTextureRHIParamRef Texture, FIntRect Rect, std::vector<FLinearColor>& OutData, FReadSurfaceDataFlags InFlags) override;


	virtual void RHIMapStagingSurface(FTextureRHIParamRef Texture, void*& OutData, int32& OutWidth, int32& OutHeight) override;


	virtual void RHIUnmapStagingSurface(FTextureRHIParamRef Texture) override;


	virtual void RHIReadSurfaceFloatData(FTextureRHIParamRef Texture, FIntRect Rect, std::vector<FFloat16Color>& OutData, ECubeFace CubeFace, int32 ArrayIndex, int32 MipIndex) override;


	virtual void RHIRead3DSurfaceFloatData(FTextureRHIParamRef Texture, FIntRect Rect, FIntPoint ZMinMax, std::vector<FFloat16Color>& OutData) override;


	virtual FRenderQueryRHIRef RHICreateRenderQuery(ERenderQueryType QueryType) override;


	virtual bool RHIGetRenderQueryResult(FRenderQueryRHIParamRef RenderQuery, uint64& OutResult, bool bWait) override;


	virtual FTexture2DRHIRef RHIGetViewportBackBuffer(FViewportRHIParamRef Viewport) override;


	virtual FUnorderedAccessViewRHIRef RHIGetViewportBackBufferUAV(FViewportRHIParamRef ViewportRHI) override;


	virtual FTexture2DRHIRef RHIGetFMaskTexture(FTextureRHIParamRef SourceTextureRHI) override;


	virtual void RHIAdvanceFrameForGetViewportBackBuffer(FViewportRHIParamRef Viewport) override;


	virtual void RHIAcquireThreadOwnership() override;


	virtual void RHIReleaseThreadOwnership() override;


	virtual void RHIFlushResources() override;


	virtual uint32 RHIGetGPUFrameCycles() override;


	virtual FViewportRHIRef RHICreateViewport(void* WindowHandle, uint32 SizeX, uint32 SizeY, bool bIsFullscreen, EPixelFormat PreferredPixelFormat) override;


	virtual void RHIResizeViewport(FViewportRHIParamRef Viewport, uint32 SizeX, uint32 SizeY, bool bIsFullscreen) override;


	virtual void RHIResizeViewport(FViewportRHIParamRef Viewport, uint32 SizeX, uint32 SizeY, bool bIsFullscreen, EPixelFormat PreferredPixelFormat) override;


	virtual void RHITick(float DeltaTime) override;


	virtual void RHISetStreamOutTargets(uint32 NumTargets, const FVertexBufferRHIParamRef* VertexBuffers, const uint32* Offsets) override;


	virtual void RHIBlockUntilGPUIdle() override;


	virtual void RHISubmitCommandsAndFlushGPU() override;


	virtual void RHIBeginSuspendRendering() override;


	virtual void RHISuspendRendering() override;


	virtual void RHIResumeRendering() override;


	virtual bool RHIIsRenderingSuspended() override;


	virtual bool RHIEnqueueDecompress(uint8_t* SrcBuffer, uint8_t* DestBuffer, int CompressedSize, void* ErrorCodeBuffer) override;


	virtual bool RHIEnqueueCompress(uint8_t* SrcBuffer, uint8_t* DestBuffer, int UnCompressedSize, void* ErrorCodeBuffer) override;


	virtual void RHIRecreateRecursiveBoundShaderStates() override;


	virtual bool RHIGetAvailableResolutions(FScreenResolutionArray& Resolutions, bool bIgnoreRefreshRate) override;


	virtual void RHIGetSupportedResolution(uint32& Width, uint32& Height) override;


	virtual void RHIVirtualTextureSetFirstMipInMemory(FTexture2DRHIParamRef Texture, uint32 FirstMip) override;


	virtual void RHIVirtualTextureSetFirstMipVisible(FTexture2DRHIParamRef Texture, uint32 FirstMip) override;


	virtual void RHIPerFrameRHIFlushComplete() override;


	virtual void RHIExecuteCommandList(FRHICommandList* CmdList) override;


	virtual void* RHIGetNativeDevice() override;


	virtual IRHICommandContext* RHIGetDefaultContext() override;




	virtual class IRHICommandContextContainer* RHIGetCommandContextContainer(int32 Index, int32 Num) override;

	virtual FVertexBufferRHIRef CreateAndLockVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo, void*& OutDataBuffer) override;


	virtual FIndexBufferRHIRef CreateAndLockIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo, void*& OutDataBuffer) override;


	virtual FVertexBufferRHIRef CreateVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FStructuredBufferRHIRef CreateStructuredBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FShaderResourceViewRHIRef CreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint8 Format) override;


	virtual FShaderResourceViewRHIRef CreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef Buffer) override;


	virtual void* LockVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode) override;


	virtual void UnlockVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer) override;


	virtual FTexture2DRHIRef AsyncReallocateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2D, int32 NewMipCount, int32 NewSizeX, int32 NewSizeY, FThreadSafeCounter* RequestStatus) override;


	virtual ETextureReallocationStatus FinalizeAsyncReallocateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted) override;


	virtual ETextureReallocationStatus CancelAsyncReallocateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted) override;


	virtual FIndexBufferRHIRef CreateIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo) override;


	virtual void* LockIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef IndexBuffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode) override;


	virtual void UnlockIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef IndexBuffer) override;


	virtual FVertexDeclarationRHIRef CreateVertexDeclaration_RenderThread(class FRHICommandListImmediate& RHICmdList, const FVertexDeclarationElementList& Elements) override;


	virtual FVertexShaderRHIRef CreateVertexShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code) override;


	virtual FVertexShaderRHIRef CreateVertexShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FPixelShaderRHIRef CreatePixelShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code) override;


	virtual FPixelShaderRHIRef CreatePixelShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FGeometryShaderRHIRef CreateGeometryShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code) override;


	virtual FGeometryShaderRHIRef CreateGeometryShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FGeometryShaderRHIRef CreateGeometryShaderWithStreamOutput_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream) override;


	virtual FGeometryShaderRHIRef CreateGeometryShaderWithStreamOutput_RenderThread(class FRHICommandListImmediate& RHICmdList, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream, FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FComputeShaderRHIRef CreateComputeShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code) override;


	virtual FComputeShaderRHIRef CreateComputeShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FHullShaderRHIRef CreateHullShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code) override;


	virtual FHullShaderRHIRef CreateHullShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual FDomainShaderRHIRef CreateDomainShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code) override;


	virtual FDomainShaderRHIRef CreateDomainShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash) override;


	virtual void* LockTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture, uint32 MipIndex, EResourceLockMode LockMode, uint32& DestStride, bool bLockWithinMiptail, bool bNeedsDefaultRHIFlush = true) override;


	virtual void UnlockTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture, uint32 MipIndex, bool bLockWithinMiptail, bool bNeedsDefaultRHIFlush = true) override;


	virtual void UpdateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture, uint32 MipIndex, const struct FUpdateTextureRegion2D& UpdateRegion, uint32 SourcePitch, const uint8* SourceData) override;


	virtual FUpdateTexture3DData BeginUpdateTexture3D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture3DRHIParamRef Texture, uint32 MipIndex, const struct FUpdateTextureRegion3D& UpdateRegion) override;


	virtual void EndUpdateTexture3D_RenderThread(class FRHICommandListImmediate& RHICmdList, FUpdateTexture3DData& UpdateData) override;


	virtual void UpdateTexture3D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture3DRHIParamRef Texture, uint32 MipIndex, const struct FUpdateTextureRegion3D& UpdateRegion, uint32 SourceRowPitch, uint32 SourceDepthPitch, const uint8* SourceData) override;


	virtual FRHIShaderLibraryRef RHICreateShaderLibrary_RenderThread(class FRHICommandListImmediate& RHICmdList, EShaderPlatform Platform, std::string FilePath, std::string Name) override;


	virtual FTextureReferenceRHIRef RHICreateTextureReference_RenderThread(class FRHICommandListImmediate& RHICmdList, FLastRenderTimeContainer* LastRenderTime) override;


	virtual FTexture2DRHIRef RHICreateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FTexture2DRHIRef RHICreateTextureExternal2D_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FTexture2DArrayRHIRef RHICreateTexture2DArray_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FTexture3DRHIRef RHICreateTexture3D_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView_RenderThread(class FRHICommandListImmediate& RHICmdList, FStructuredBufferRHIParamRef StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer) override;


	virtual FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTextureRHIParamRef Texture, uint32 MipLevel) override;


	virtual FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer, uint8 Format) override;


	virtual FUnorderedAccessViewRHIRef RHICreateUnorderedAccessView_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef IndexBuffer, uint8 Format) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2DRHI, uint8 MipLevel) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2DRHI, uint8 MipLevel, uint8 NumMipLevels, uint8 Format) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture3DRHIParamRef Texture3DRHI, uint8 MipLevel) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DArrayRHIParamRef Texture2DArrayRHI, uint8 MipLevel) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTextureCubeRHIParamRef TextureCubeRHI, uint8 MipLevel) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint8 Format) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef Buffer) override;


	virtual FShaderResourceViewRHIRef RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FStructuredBufferRHIParamRef StructuredBuffer) override;


	virtual FTextureCubeRHIRef RHICreateTextureCube_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Size, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FTextureCubeRHIRef RHICreateTextureCubeArray_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Size, uint32 ArraySize, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo) override;


	virtual FRenderQueryRHIRef RHICreateRenderQuery_RenderThread(class FRHICommandListImmediate& RHICmdList, ERenderQueryType QueryType) override;


	virtual void* RHILockTextureCubeFace_RenderThread(class FRHICommandListImmediate& RHICmdList, FTextureCubeRHIParamRef Texture, uint32 FaceIndex, uint32 ArrayIndex, uint32 MipIndex, EResourceLockMode LockMode, uint32& DestStride, bool bLockWithinMiptail) override;


	virtual void RHIUnlockTextureCubeFace_RenderThread(class FRHICommandListImmediate& RHICmdList, FTextureCubeRHIParamRef Texture, uint32 FaceIndex, uint32 ArrayIndex, uint32 MipIndex, bool bLockWithinMiptail) override;


	virtual void RHIAcquireTransientResource_RenderThread(FTextureRHIParamRef Texture) override;


	virtual void RHIDiscardTransientResource_RenderThread(FTextureRHIParamRef Texture) override;


	virtual void RHIAcquireTransientResource_RenderThread(FVertexBufferRHIParamRef Buffer) override;


	virtual void RHIDiscardTransientResource_RenderThread(FVertexBufferRHIParamRef Buffer) override;


	virtual void RHIAcquireTransientResource_RenderThread(FStructuredBufferRHIParamRef Buffer) override;


	virtual void RHIDiscardTransientResource_RenderThread(FStructuredBufferRHIParamRef Buffer) override;


	virtual void RHIReadSurfaceFloatData_RenderThread(class FRHICommandListImmediate& RHICmdList, FTextureRHIParamRef Texture, FIntRect Rect, std::vector<FFloat16Color>& OutData, ECubeFace CubeFace, int32 ArrayIndex, int32 MipIndex) override;


	virtual void EnableIdealGPUCaptureOptions(bool bEnable) override;


	virtual void RHISetResourceAliasability_RenderThread(class FRHICommandListImmediate& RHICmdList, EResourceAliasability AliasMode, FTextureRHIParamRef* InTextures, int32 NumTextures) override;


	virtual bool CheckGpuHeartbeat() const override;


	virtual void VirtualTextureSetFirstMipInMemory_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture, uint32 FirstMip) override;


	virtual void VirtualTextureSetFirstMipVisible_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture, uint32 FirstMip) override;


	virtual void RHICopySubTextureRegion_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef SourceTexture, FTexture2DRHIParamRef DestinationTexture, FBox2D SourceBox, FBox2D DestinationBox) override;


	virtual void RHICopySubTextureRegion(FTexture2DRHIParamRef SourceTexture, FTexture2DRHIParamRef DestinationTexture, FBox2D SourceBox, FBox2D DestinationBox) override;


	virtual FRHIFlipDetails RHIWaitForFlip(double TimeoutInSeconds) override;


	virtual void RHISignalFlipEvent() override;


	virtual void RHICalibrateTimers() override;


	virtual void RHIWaitComputeFence(FComputeFenceRHIParamRef InFence) override;


	virtual void RHISetComputeShader(FComputeShaderRHIParamRef ComputeShader) override;


	virtual void RHIDispatchComputeShader(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ) override;


	virtual void RHIDispatchIndirectComputeShader(FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset) override;


	virtual void RHISetAsyncComputeBudget(EAsyncComputeBudget Budget) override;


	virtual void RHIAutomaticCacheFlushAfterComputeShader(bool bEnable) override;


	virtual void RHIFlushComputeShaderCache() override;


	virtual void RHISetMultipleViewports(uint32_t Count, const FViewportBounds* Data) override;


	virtual void RHIClearTinyUAV(FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI, const uint32_t* Values) override;


	virtual void RHICopyToResolveTarget(FTextureRHIParamRef SourceTexture, FTextureRHIParamRef DestTexture, const FResolveParams& ResolveParams) override;


	virtual void RHITransitionResources(EResourceTransitionAccess TransitionType, FTextureRHIParamRef* InTextures, int32_t NumTextures) override;


	virtual void RHITransitionResources(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32_t NumUAVs, FComputeFenceRHIParamRef WriteComputeFence) override;


	virtual void RHIBeginRenderQuery(FRenderQueryRHIParamRef RenderQuery) override;


	virtual void RHIEndRenderQuery(FRenderQueryRHIParamRef RenderQuery) override;


	virtual void RHISubmitCommandsHint() override;


	virtual void RHIDiscardRenderTargets(bool Depth, bool Stencil, uint32_t ColorBitMask) override;


	virtual void RHIBeginDrawingViewport(FViewportRHIParamRef Viewport, FTextureRHIParamRef RenderTargetRHI) override;


	virtual void RHIEndDrawingViewport(FViewportRHIParamRef Viewport, bool bPresent, bool bLockToVsync) override;


	virtual void RHIBeginFrame() override;


	virtual void RHIEndFrame() override;


	virtual void RHIBeginScene() override;


	virtual void RHIEndScene() override;


	virtual void RHIBeginUpdateMultiFrameResource(FTextureRHIParamRef Texture) override;


	virtual void RHIEndUpdateMultiFrameResource(FTextureRHIParamRef Texture) override;


	virtual void RHIBeginUpdateMultiFrameResource(FUnorderedAccessViewRHIParamRef UAV) override;


	virtual void RHIEndUpdateMultiFrameResource(FUnorderedAccessViewRHIParamRef UAV) override;


	virtual void RHISetStreamSource(uint32_t StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32_t Offset) override;


	virtual void RHISetViewport(uint32_t MinX, uint32_t MinY, float MinZ, uint32_t MaxX, uint32_t MaxY, float MaxZ) override;


	virtual void RHISetStereoViewport(uint32_t LeftMinX, uint32_t RightMinX, uint32_t LeftMinY, uint32_t RightMinY, float MinZ, uint32_t LeftMaxX, uint32_t RightMaxX, uint32_t LeftMaxY, uint32_t RightMaxY, float MaxZ) override;


	virtual void RHISetScissorRect(bool bEnable, uint32_t MinX, uint32_t MinY, uint32_t MaxX, uint32_t MaxY) override;


	virtual void RHISetGraphicsPipelineState(FGraphicsPipelineStateRHIParamRef GraphicsState) override;


	virtual void RHISetShaderTexture(FVertexShaderRHIParamRef VertexShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) override;


	virtual void RHISetShaderTexture(FHullShaderRHIParamRef HullShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) override;


	virtual void RHISetShaderTexture(FDomainShaderRHIParamRef DomainShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) override;


	virtual void RHISetShaderTexture(FGeometryShaderRHIParamRef GeometryShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) override;


	virtual void RHISetShaderTexture(FPixelShaderRHIParamRef PixelShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) override;


	virtual void RHISetShaderTexture(FComputeShaderRHIParamRef PixelShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture) override;


	virtual void RHISetShaderSampler(FComputeShaderRHIParamRef ComputeShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) override;


	virtual void RHISetShaderSampler(FVertexShaderRHIParamRef VertexShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) override;


	virtual void RHISetShaderSampler(FGeometryShaderRHIParamRef GeometryShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) override;


	virtual void RHISetShaderSampler(FDomainShaderRHIParamRef DomainShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) override;


	virtual void RHISetShaderSampler(FHullShaderRHIParamRef HullShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) override;


	virtual void RHISetShaderSampler(FPixelShaderRHIParamRef PixelShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState) override;


	virtual void RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV) override;


	virtual void RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32_t InitialCount) override;


	virtual void RHISetShaderResourceViewParameter(FPixelShaderRHIParamRef PixelShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) override;


	virtual void RHISetShaderResourceViewParameter(FVertexShaderRHIParamRef VertexShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) override;


	virtual void RHISetShaderResourceViewParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) override;


	virtual void RHISetShaderResourceViewParameter(FHullShaderRHIParamRef HullShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) override;


	virtual void RHISetShaderResourceViewParameter(FDomainShaderRHIParamRef DomainShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) override;


	virtual void RHISetShaderResourceViewParameter(FGeometryShaderRHIParamRef GeometryShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV) override;


	virtual void RHISetShaderUniformBuffer(FVertexShaderRHIParamRef VertexShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) override;


	virtual void RHISetShaderUniformBuffer(FHullShaderRHIParamRef HullShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) override;


	virtual void RHISetShaderUniformBuffer(FDomainShaderRHIParamRef DomainShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) override;


	virtual void RHISetShaderUniformBuffer(FGeometryShaderRHIParamRef GeometryShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) override;


	virtual void RHISetShaderUniformBuffer(FPixelShaderRHIParamRef PixelShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) override;


	virtual void RHISetShaderUniformBuffer(FComputeShaderRHIParamRef ComputeShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer) override;


	virtual void RHISetShaderParameter(FVertexShaderRHIParamRef VertexShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) override;


	virtual void RHISetShaderParameter(FPixelShaderRHIParamRef PixelShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) override;


	virtual void RHISetShaderParameter(FHullShaderRHIParamRef HullShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) override;


	virtual void RHISetShaderParameter(FDomainShaderRHIParamRef DomainShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) override;


	virtual void RHISetShaderParameter(FGeometryShaderRHIParamRef GeometryShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) override;


	virtual void RHISetShaderParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue) override;


	virtual void RHISetStencilRef(uint32_t StencilRef) override;


	virtual void RHISetBlendFactor(const FLinearColor& BlendFactor) override;


	virtual void RHISetRenderTargets(uint32_t NumSimultaneousRenderTargets, const FRHIRenderTargetView* NewRenderTargets, const FRHIDepthRenderTargetView* NewDepthStencilTarget, uint32_t NumUAVs, const FUnorderedAccessViewRHIParamRef* UAVs) override;


	virtual void RHISetRenderTargetsAndClear(const FRHISetRenderTargetsInfo& RenderTargetsInfo) override;


	virtual void RHIBindClearMRTValues(bool bClearColor, bool bClearDepth, bool bClearStencil) override;


	virtual void RHIDrawPrimitive(uint32_t PrimitiveType, uint32_t BaseVertexIndex, uint32_t NumPrimitives, uint32_t NumInstances) override;


	virtual void RHIDrawPrimitiveIndirect(uint32_t PrimitiveType, FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset) override;


	virtual void RHIDrawIndexedIndirect(FIndexBufferRHIParamRef IndexBufferRHI, uint32_t PrimitiveType, FStructuredBufferRHIParamRef ArgumentsBufferRHI, int32_t DrawArgumentsIndex, uint32_t NumInstances) override;


	virtual void RHIDrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32_t PrimitiveType, int32_t BaseVertexIndex, uint32_t FirstInstance, uint32_t NumVertices, uint32_t StartIndex, uint32_t NumPrimitives, uint32_t NumInstances) override;


	virtual void RHIDrawIndexedPrimitiveIndirect(uint32_t PrimitiveType, FIndexBufferRHIParamRef IndexBuffer, FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset) override;


	virtual void RHIBeginDrawPrimitiveUP(uint32_t PrimitiveType, uint32_t NumPrimitives, uint32_t NumVertices, uint32_t VertexDataStride, void*& OutVertexData) override;


	virtual void RHIEndDrawPrimitiveUP() override;


	virtual void RHIBeginDrawIndexedPrimitiveUP(uint32_t PrimitiveType, uint32_t NumPrimitives, uint32_t NumVertices, uint32_t VertexDataStride, void*& OutVertexData, uint32_t MinVertexIndex, uint32_t NumIndices, uint32_t IndexDataStride, void*& OutIndexData) override;


	virtual void RHIEndDrawIndexedPrimitiveUP() override;


	virtual void RHISetDepthBounds(float MinDepth, float MaxDepth) override;


	virtual void RHIPushEvent(const TCHAR* Name, FColor Color) override;


	virtual void RHIPopEvent() override;


	virtual void RHIUpdateTextureReference(FTextureReferenceRHIParamRef TextureRef, FTextureRHIParamRef NewTexture) override;


	virtual void RHIBeginRenderPass(const FRHIRenderPassInfo& InInfo, const TCHAR* InName) override;


	virtual void RHIEndRenderPass() override;


	virtual void RHIBeginComputePass(const TCHAR* InName) override;


	virtual void RHIEndComputePass() override;


	virtual void RHICopyTexture(FTextureRHIParamRef SourceTexture, FTextureRHIParamRef DestTexture, const FRHICopyTextureInfo& CopyInfo) override;


	virtual void RHISetComputePipelineState(FRHIComputePipelineState* ComputePipelineState) override;


	virtual void RHIInvalidateCachedState() override;


	virtual void RHISetBoundShaderState(FBoundShaderStateRHIParamRef BoundShaderState) override;


	virtual void RHISetDepthStencilState(FDepthStencilStateRHIParamRef NewState, uint32_t StencilRef) override;


	virtual void RHISetRasterizerState(FRasterizerStateRHIParamRef NewState) override;


	virtual void RHISetBlendState(FBlendStateRHIParamRef NewState, const FLinearColor& BlendFactor) override;


	virtual void RHIEnableDepthBoundsTest(bool bEnable) override;
protected:
	void CleanupD3DDevice();
	void ClearState();
	// shared code for different D3D11 devices (e.g. PC DirectX11 and XboxOne) called
// after device creation and GRHISupportsAsyncTextureCreation was set and before resource init
	void SetupAfterDeviceCreation();

	// called by SetupAfterDeviceCreation() when the device gets initialized
	void UpdateMSAASettings();

	// @return 0xffffffff if not not supported
	uint32 GetMaxMSAAQuality(uint32 SampleCount);
private:
	TRefCountPtr<ID3D11Device> d3d_device_;
	TRefCountPtr<ID3D11DeviceContext> d3d_dc_;
	/** The global D3D interface. */
	TRefCountPtr<IDXGIFactory1> DXGIFactory1;
	D3D_FEATURE_LEVEL FeatureLevel;
	int32 ChosenAdapter;
	DXGI_ADAPTER_DESC DXGIAdpaterDesc;
	FD3D11StateCache StateCache;

	// set by UpdateMSAASettings(), get by GetMSAAQuality()
// [SampleCount] = Quality, 0xffffffff if not supported
	uint32 AvailableMSAAQualities[DX_MAX_MSAA_COUNT + 1];

	/** The global D3D device's immediate context */
};