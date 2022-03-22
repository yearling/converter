// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHI.h: Render Hardware Interface definitions.
=============================================================================*/

#pragma once

#include "RHI/RHIDefinitions.h"
#include <string>
#include "Math/YMatrix.h"
#include "Engine/EnumAsByte.h"
#include <vector>
#include "Math/YColor.h"
#include "Math/IntVector.h"
#include <stdint.h>

#ifndef RHI_COMMAND_LIST_DEBUG_TRACES
#define RHI_COMMAND_LIST_DEBUG_TRACES 0
#endif

class FResourceArrayInterface;
class FResourceBulkDataInterface;

/** Uniform buffer structs must be aligned to 16-byte boundaries. */
#define UNIFORM_BUFFER_STRUCT_ALIGNMENT 16

/** RHI Logging. */

/**
 * RHI configuration settings.
 */

namespace RHIConfig
{
	 bool ShouldSaveScreenshotAfterProfilingGPU();
	 bool ShouldShowProfilerAfterProfilingGPU();
	 float GetGPUHitchThreshold();
}

/**
 * RHI globals.
 */

/** True if the render hardware has been initialized. */
extern  bool GIsRHIInitialized;

class  FRHICommandList;

/**
 * RHI capabilities.
 */


/** The maximum number of mip-maps that a texture can contain. 	*/
extern	 int32_t		GMaxTextureMipCount;

/** true if this platform has quad buffer stereo support. */
extern  bool GSupportsQuadBufferStereo;

/** true if the RHI supports textures that may be bound as both a render target and a shader resource. */
extern  bool GSupportsRenderDepthTargetableShaderResources;

/** true if the RHI supports binding depth as a texture when testing against depth */
extern  bool GSupportsDepthFetchDuringDepthTest;

// The maximum feature level and shader platform available on this system
// GRHIFeatureLevel and GRHIShaderPlatform have been deprecated. There is no longer a current featurelevel/shaderplatform that
// should be used for all rendering, rather a specific set for each view.
extern  ERHIFeatureLevel::Type GMaxRHIFeatureLevel;
extern  EShaderPlatform GMaxRHIShaderPlatform;

/** true if the RHI supports SRVs */
extern  bool GSupportsResourceView;

/** 
 * only set if RHI has the information (after init of the RHI and only if RHI has that information, never changes after that)
 * e.g. "NVIDIA GeForce GTX 670"
 */
extern  std::string GRHIAdapterName;
extern  std::string GRHIAdapterInternalDriverVersion;
extern  std::string GRHIAdapterUserDriverVersion;
extern  std::string GRHIAdapterDriverDate;
extern  uint32_t GRHIDeviceId;
extern  uint32_t GRHIDeviceRevision;

// 0 means not defined yet, use functions like IsRHIDeviceAMD() to access
extern  uint32_t GRHIVendorId;

// to trigger GPU specific optimizations and fallbacks
 bool IsRHIDeviceAMD();

// to trigger GPU specific optimizations and fallbacks
 bool IsRHIDeviceIntel();

// to trigger GPU specific optimizations and fallbacks
 bool IsRHIDeviceNVIDIA();

// helper to convert GRHIVendorId into a printable string, or "Unknown" if unknown.
 const char* RHIVendorIdToString();

// helper to return the shader language version for the given shader platform.
 uint32_t RHIGetShaderLanguageVersion(const EShaderPlatform Platform);

// helper to check that the shader platform supports tessellation.
 bool RHISupportsTessellation(const EShaderPlatform Platform);

// helper to check that the shader platform supports writing to UAVs from pixel shaders.
 bool RHISupportsPixelShaderUAVs(const EShaderPlatform Platform);

// helper to check that the shader platform supports creating a UAV off an index buffer.
 bool RHISupportsIndexBufferUAVs(const EShaderPlatform Platform);

// helper to check if a preview feature level has been requested.
 bool RHIGetPreviewFeatureLevel(ERHIFeatureLevel::Type& PreviewFeatureLevelOUT);

inline bool RHISupportsInstancedStereo(const EShaderPlatform Platform)
{
	// Only D3D SM5, PS4 and Metal SM5 supports Instanced Stereo
	return (Platform == EShaderPlatform::SP_PCD3D_SM5 || Platform == EShaderPlatform::SP_PS4 || Platform == EShaderPlatform::SP_METAL_SM5 || Platform == EShaderPlatform::SP_METAL_SM5_NOTESS);
}

inline bool RHISupportsMultiView(const EShaderPlatform Platform)
{
	// Only PS4 and Metal SM5 from 10.13 onward supports Multi-View
	return (Platform == EShaderPlatform::SP_PS4) || ((Platform == EShaderPlatform::SP_METAL_SM5 || Platform == SP_METAL_SM5_NOTESS) && RHIGetShaderLanguageVersion(Platform) >= 3);
}

inline bool RHISupportsMSAA(EShaderPlatform Platform)
{
	return 
		//@todo-rco: Fix when iOS OpenGL supports MSAA
		Platform != SP_OPENGL_ES2_IOS
		// @todo marksatt Metal on macOS 10.12 and earlier (or Intel on any macOS < 10.13.2) don't reliably support our MSAA usage & custom resolve.
#if PLATFORM_MAC
		&& IsMetalPlatform(Platform) && (FPlatformMisc::MacOSXVersionCompare(10, 13, 0) >= 0) && (!IsRHIDeviceIntel() || FPlatformMisc::MacOSXVersionCompare(10, 13, 2) >= 0)
#endif
		// @todo marksatt iOS Desktop Forward needs more work internally
		&& Platform != SP_METAL_MRT;
}

inline bool RHISupportsBufferLoadTypeConversion(EShaderPlatform Platform)
{
#if PLATFORM_MAC || PLATFORM_IOS
	return !IsMetalPlatform(Platform);
#else
	return true;
#endif
}

/** Whether the platform supports reading from volume textures (does not cover rendering to volume textures). */
inline bool RHISupportsVolumeTextures(ERHIFeatureLevel::Type FeatureLevel)
{
	return FeatureLevel >= ERHIFeatureLevel::SM4;
}

inline bool RHISupports4ComponentUAVReadWrite(EShaderPlatform Platform)
{
	// Must match usf PLATFORM_SUPPORTS_4COMPONENT_UAV_READ_WRITE
	// D3D11 does not support multi-component loads from a UAV: "error X3676: typed UAV loads are only allowed for single-component 32-bit element types"
	return Platform == SP_XBOXONE_D3D12 || Platform == SP_PS4 || IsMetalPlatform(Platform);
}

/** Whether Manual Vertex Fetch is supported for the specified shader platform.
	Shader Platform must not use the mobile renderer, and for Metal, the shader language must be at least 2. */
inline bool RHISupportsManualVertexFetch(EShaderPlatform InShaderPlatform)
{
	return (!IsOpenGLPlatform(InShaderPlatform) || IsSwitchPlatform(InShaderPlatform)) && !IsMobilePlatform(InShaderPlatform) && (!IsMetalPlatform(InShaderPlatform) || RHIGetShaderLanguageVersion(InShaderPlatform) >= 2);
}

// Wrapper for GRHI## global variables, allows values to be overridden for mobile preview modes.
template <typename TValueType>
class TRHIGlobal
{
public:
	explicit TRHIGlobal(const TValueType& InValue) : Value(InValue) {}

	TRHIGlobal& operator=(const TValueType& InValue) 
	{
		Value = InValue; 
		return *this;
	}

//#if WITH_EDITOR
//	inline void SetPreviewOverride(const TValueType& InValue)
//	{
//		PreviewValue = InValue;
//	}
//
//	inline operator TValueType() const
//	{ 
//		return PreviewValue.IsSet() ? GetPreviewValue() : Value;
//	}
//#else
//	inline operator TValueType() const { return Value; }
//#endif

	inline operator TValueType() const { return Value; }
private:
	TValueType Value;
#if WITH_EDITOR
	//TOptional<TValueType> PreviewValue;
	//TValueType GetPreviewValue() const { return PreviewValue.GetValue(); }
#endif
};

#if WITH_EDITOR
//template<>
//inline int32_t TRHIGlobal<int32_t>::GetPreviewValue() const 
//{
//	// ensure the preview values are subsets of RHI functionality.
//	return YMath::Min(PreviewValue.GetValue(), Value);
//}
//template<>
//inline bool TRHIGlobal<bool>::GetPreviewValue() const
//{
//	// ensure the preview values are subsets of RHI functionality.
//	return PreviewValue.GetValue() && Value;
//}
#endif

/** true if the GPU is AMD's Pre-GCN architecture */
extern  bool GRHIDeviceIsAMDPreGCNArchitecture;

/** true if PF_G8 render targets are supported */
extern  TRHIGlobal<bool> GSupportsRenderTargetFormat_PF_G8;

/** true if PF_FloatRGBA render targets are supported */
extern  TRHIGlobal<bool> GSupportsRenderTargetFormat_PF_FloatRGBA;

/** true if mobile framebuffer fetch is supported */
extern  bool GSupportsShaderFramebufferFetch;

/** true if mobile depth & stencil fetch is supported */
extern  bool GSupportsShaderDepthStencilFetch;

/** true if RQT_AbsoluteTime is supported by RHICreateRenderQuery */
extern  bool GSupportsTimestampRenderQueries;

/** true if RQT_AbsoluteTime is supported by RHICreateRenderQuery */
extern  bool GRHISupportsGPUTimestampBubblesRemoval;

/** true if RHIGetGPUFrameCycles removes CPu generated bubbles. */
extern  bool GRHISupportsFrameCyclesBubblesRemoval;

/** true if the GPU supports hidden surface removal in hardware. */
extern  bool GHardwareHiddenSurfaceRemoval;

/** true if the RHI supports asynchronous creation of texture resources */
extern  bool GRHISupportsAsyncTextureCreation;

/** true if the RHI supports quad topology (PT_QuadList). */
extern  bool GRHISupportsQuadTopology;

/** true if the RHI supports rectangular topology (PT_RectList). */
extern  bool GRHISupportsRectTopology;

/** Temporary. When OpenGL is running in a separate thread, it cannot yet do things like initialize shaders that are first discovered in a rendering task. It is doable, it just isn't done. */
extern  bool GSupportsParallelRenderingTasksWithSeparateRHIThread;

/** If an RHI is so slow, that it is the limiting factor for the entire frame, we can kick early to try to give it as much as possible. */
extern  bool GRHIThreadNeedsKicking;

/** If an RHI cannot do an unlimited number of occlusion queries without stalling and waiting for the GPU, this can be used to tune hte occlusion culler to try not to do that. */
extern  int32_t GRHIMaximumReccommendedOustandingOcclusionQueries;

/** Some RHIs can only do visible or not occlusion queries. */
extern  bool GRHISupportsExactOcclusionQueries;

/** True if and only if the GPU support rendering to volume textures (2D Array, 3D). Some OpenGL 3.3 cards support SM4, but can't render to volume textures. */
extern  bool GSupportsVolumeTextureRendering;

/** True if the RHI supports separate blend states per render target. */
extern  bool GSupportsSeparateRenderTargetBlendState;

/** True if the RHI can render to a depth-only render target with no additional color render target. */
extern  bool GSupportsDepthRenderTargetWithoutColorRenderTarget;

/** True if the RHI has artifacts with atlased CSM depths. */
extern  bool GRHINeedsUnatlasedCSMDepthsWorkaround;

/** true if the RHI supports 3D textures */
extern  bool GSupportsTexture3D;

/** true if the RHI supports mobile multi-view */
extern  bool GSupportsMobileMultiView;

/** true if the RHI supports image external */
extern  bool GSupportsImageExternal;

/** true if the RHI supports MRT */
extern  TRHIGlobal<bool> GSupportsMultipleRenderTargets;

/** true if the RHI supports 256bit MRT */
extern  bool GSupportsWideMRT;

/** True if the RHI and current hardware supports supports depth bounds testing */
extern  bool GSupportsDepthBoundsTest;

/** True if the RHI and current hardware support a render target write mask */
extern  bool GSupportsRenderTargetWriteMask;

/** True if the RHI and current hardware supports efficient AsyncCompute (by default we assume false and later we can enable this for more hardware) */
extern  bool GSupportsEfficientAsyncCompute;

/** True if the RHI supports 'GetHDR32bppEncodeModeES2' shader intrinsic. */
extern  bool GSupportsHDR32bppEncodeModeIntrinsic;

/** True if the RHI supports getting the result of occlusion queries when on a thread other than the renderthread */
extern  bool GSupportsParallelOcclusionQueries;

/** true if the RHI supports aliasing of transient resources */
extern  bool GSupportsTransientResourceAliasing;

/** true if the RHI requires a valid RT bound during UAV scatter operation inside the pixel shader */
extern  bool GRHIRequiresRenderTargetForPixelShaderUAVs;

/** The minimum Z value in clip space for the RHI. */
extern  float GMinClipZ;

/** The sign to apply to the Y axis of projection matrices. */
extern  float GProjectionSignY;

/** Does this RHI need to wait for deletion of resources due to ref counting. */
extern  bool GRHINeedsExtraDeletionLatency;

/** The maximum size to allow for the shadow depth buffer in the X dimension.  This must be larger or equal to GMaxShadowDepthBufferSizeY. */
extern  TRHIGlobal<int32_t> GMaxShadowDepthBufferSizeX;
/** The maximum size to allow for the shadow depth buffer in the Y dimension. */
extern  TRHIGlobal<int32_t> GMaxShadowDepthBufferSizeY;

/** The maximum size allowed for 2D textures in both dimensions. */
extern  TRHIGlobal<int32_t> GMaxTextureDimensions;

inline uint32_t GetMax2DTextureDimension()
{
	return GMaxTextureDimensions;
}

/** The maximum size allowed for cube textures. */
extern  TRHIGlobal<int32_t> GMaxCubeTextureDimensions;
inline uint32_t GetMaxCubeTextureDimension()
{
	return GMaxCubeTextureDimensions;
}

/** The Maximum number of layers in a 1D or 2D texture array. */
extern  int32_t GMaxTextureArrayLayers;
inline uint32_t GetMaxTextureArrayLayers()
{
	return GMaxTextureArrayLayers;
}

extern  int32_t GMaxTextureSamplers;
inline uint32_t GetMaxTextureSamplers()
{
	return GMaxTextureSamplers;
}

/** true if we are running with the NULL RHI */
extern  bool GUsingNullRHI;

/**
 *	The size to check against for Draw*UP call vertex counts.
 *	If greater than this value, the draw call will not occur.
 */
extern  int32_t GDrawUPVertexCheckCount;
/**
 *	The size to check against for Draw*UP call index counts.
 *	If greater than this value, the draw call will not occur.
 */
extern  int32_t GDrawUPIndexCheckCount;

/** true for each VET that is supported. One-to-one mapping with EVertexElementType */
extern  class FVertexElementTypeSupportInfo GVertexElementTypeSupport;

#include "RHI/MultiGPU.h"

 EMultiGPUMode GetMultiGPUMode();
 FRHIGPUMask GetNodeMaskFromMultiGPUMode(EMultiGPUMode Strategy, uint32_t ViewIndex, uint32_t FrameIndex);

/** Whether the next frame should profile the GPU. */
extern  bool GTriggerGPUProfile;

/** Whether we are profiling GPU hitches. */
extern  bool GTriggerGPUHitchProfile;

/** Non-empty if we are performing a gpu trace. Also says where to place trace file. */
extern  std::string GGPUTraceFileName;

/** True if the RHI supports texture streaming */
extern  bool GRHISupportsTextureStreaming;
/** Amount of memory allocated by textures. In kilobytes. */
extern  volatile int32_t GCurrentTextureMemorySize;
/** Amount of memory allocated by rendertargets. In kilobytes. */
extern  volatile int32_t GCurrentRendertargetMemorySize;
/** Current texture streaming pool size, in bytes. 0 means unlimited. */
extern  int64_t GTexturePoolSize;
/** In percent. If non-zero, the texture pool size is a percentage of GTotalGraphicsMemory. */
extern  int32_t GPoolSizeVRAMPercentage;

/** Some simple runtime stats, reset on every call to RHIBeginFrame */
/** Num draw calls & primitives on previous frame (accurate on any thread)*/
extern  int32_t GNumDrawCallsRHI;
extern  int32_t GNumPrimitivesDrawnRHI;

/** Num draw calls and primitives this frame (only accurate on RenderThread) */
extern  int32_t GCurrentNumDrawCallsRHI;
extern  int32_t GCurrentNumPrimitivesDrawnRHI;


/** Whether or not the RHI can handle a non-zero BaseVertexIndex - extra SetStreamSource calls will be needed if this is false */
extern  bool GRHISupportsBaseVertexIndex;

/** True if the RHI supports hardware instancing */
extern  TRHIGlobal<bool> GRHISupportsInstancing;

/** True if the RHI supports copying cubemap faces using CopyToResolveTarget */
extern  bool GRHISupportsResolveCubemapFaces;

/** Whether or not the RHI can handle a non-zero FirstInstance - extra SetStreamSource calls will be needed if this is false */
extern  bool GRHISupportsFirstInstance;

/** Whether or not the RHI can handle dynamic resolution or not. */
extern  bool GRHISupportsDynamicResolution;

/** Whether or not the RHI supports an RHI thread.
Requirements for RHI thread
* Microresources (those in RHIStaticStates.h) need to be able to be created by any thread at any time and be able to work with a radically simplified rhi resource lifecycle. CreateSamplerState, CreateRasterizerState, CreateDepthStencilState, CreateBlendState
* CreateUniformBuffer needs to be threadsafe
* GetRenderQueryResult should be threadsafe, but this isn't required. If it isn't threadsafe, then you need to flush yourself in the RHI
* GetViewportBackBuffer and AdvanceFrameForGetViewportBackBuffer need to be threadsafe and need to support the fact that the render thread has a different concept of "current backbuffer" than the RHI thread. Without an RHIThread this is moot due to the next two items.
* AdvanceFrameForGetViewportBackBuffer needs be added as an RHI method and this needs to work with GetViewportBackBuffer to give the render thread the right back buffer even though many commands relating to the beginning and end of the frame are queued.
* BeginDrawingViewport, and 5 or so other frame advance methods are queued with an RHIThread. Without an RHIThread, these just flush internally.
***/
extern  bool GRHISupportsRHIThread;
/* as above, but we run the commands on arbitrary task threads */
extern  bool GRHISupportsRHIOnTaskThread;

/** Whether or not the RHI supports parallel RHIThread executes / translates
Requirements:
* RHICreateBoundShaderState & RHICreateGraphicsPipelineState is threadsafe and GetCachedBoundShaderState must not be used. GetCachedBoundShaderState_Threadsafe has a slightly different protocol.
***/
extern  bool GRHISupportsParallelRHIExecute;

/** Whether or not the RHI can perform MSAA sample load. */
extern  bool GRHISupportsMSAADepthSampleAccess;

/** Whether the present adapter/display offers HDR output capabilities. */
extern  bool GRHISupportsHDROutput;

/** Format used for the backbuffer when outputting to a HDR display. */
extern  EPixelFormat GRHIHDRDisplayOutputFormat;

/** Counter incremented once on each frame present. Used to support game thread synchronization with swap chain frame flips. */
extern  uint64_t GRHIPresentCounter;

/** Called once per frame only from within an RHI. */
extern  void RHIPrivateBeginFrame();

 std::string LegacyShaderPlatformToShaderFormat(EShaderPlatform Platform);
 EShaderPlatform ShaderFormatToLegacyShaderPlatform(std::string ShaderFormat);

/**
 * Adjusts a projection matrix to output in the correct clip space for the
 * current RHI. Unreal projection matrices follow certain conventions and
 * need to be patched for some RHIs. All projection matrices should be adjusted
 * before being used for rendering!
 */
inline YMatrix AdjustProjectionMatrixForRHI(const YMatrix& InProjectionMatrix)
{
	//FScaleMatrix ClipSpaceFixScale(FVector(1.0f, GProjectionSignY, 1.0f - GMinClipZ));
	//FTranslationMatrix ClipSpaceFixTranslate(FVector(0.0f, 0.0f, GMinClipZ));	
	//return InProjectionMatrix * ClipSpaceFixScale * ClipSpaceFixTranslate;
	assert(0);
	return YMatrix();
}

/** Set runtime selection of mobile feature level preview. */
 void RHISetMobilePreviewFeatureLevel(ERHIFeatureLevel::Type MobilePreviewFeatureLevel);

/** Current shader platform. */


/** Finds a corresponding ERHIFeatureLevel::Type given an std::string, or returns false if one could not be found. */
extern  bool GetFeatureLevelFromName(std::string Name, ERHIFeatureLevel::Type& OutFeatureLevel);

/** Creates a string for the given feature level. */
extern  void GetFeatureLevelName(ERHIFeatureLevel::Type InFeatureLevel, std::string& OutName);

/** Creates an std::string for the given feature level. */
//extern  void GetFeatureLevelName(ERHIFeatureLevel::Type InFeatureLevel, FName& OutName);


/** Table for finding out which shader platform corresponds to a given feature level for this RHI. */
extern  EShaderPlatform GShaderPlatformForFeatureLevel[ERHIFeatureLevel::Num];

/** Get the shader platform associated with the supplied feature level on this machine */
inline EShaderPlatform GetFeatureLevelShaderPlatform(ERHIFeatureLevel::Type InFeatureLevel)
{
	return GShaderPlatformForFeatureLevel[InFeatureLevel];
}

//inline FArchive& operator <<(FArchive& Ar, EResourceLockMode& LockMode)
//{
//	uint32_t Temp = LockMode;
//	Ar << Temp;
//	LockMode = (EResourceLockMode)Temp;
//	return Ar;
//}

/** to customize the RHIReadSurfaceData() output */
class FReadSurfaceDataFlags
{
public:
	// @param InCompressionMode defines the value input range that is mapped to output range
	// @param InCubeFace defined which cubemap side is used, only required for cubemap content, then it needs to be a valid side
	FReadSurfaceDataFlags(ERangeCompressionMode InCompressionMode = RCM_UNorm, ECubeFace InCubeFace = CubeFace_MAX) 
		:CubeFace(InCubeFace), CompressionMode(InCompressionMode), bLinearToGamma(true), MaxDepthRange(16000.0f), bOutputStencil(false), MipLevel(0)
	{
	}

	ECubeFace GetCubeFace() const
	{
		assert(CubeFace <= CubeFace_NegZ);
		return CubeFace;
	}

	ERangeCompressionMode GetCompressionMode() const
	{
		return CompressionMode;
	}

	void SetLinearToGamma(bool Value)
	{
		bLinearToGamma = Value;
	}

	bool GetLinearToGamma() const
	{
		return bLinearToGamma;
	}

	void SetOutputStencil(bool Value)
	{
		bOutputStencil = Value;
	}

	bool GetOutputStencil() const
	{
		return bOutputStencil;
	}

	void SetMip(uint8_t InMipLevel)
	{
		MipLevel = InMipLevel;
	}

	uint8_t GetMip() const
	{
		return MipLevel;
	}	

	void SetMaxDepthRange(float Value)
	{
		MaxDepthRange = Value;
	}

	float ComputeNormalizedDepth(float DeviceZ) const
	{
		return YMath::Abs(ConvertFromDeviceZ(DeviceZ) / MaxDepthRange);
	}

private:

	// @return SceneDepth
	float ConvertFromDeviceZ(float DeviceZ) const
	{
		DeviceZ = YMath::Min(DeviceZ, 1 - Z_PRECISION);

		// for depth to linear conversion
		const YVector2 InvDeviceZToWorldZ(0.1f, 0.1f);

		return 1.0f / (DeviceZ * InvDeviceZToWorldZ.x - InvDeviceZToWorldZ.y);
	}

	ECubeFace CubeFace;
	ERangeCompressionMode CompressionMode;
	bool bLinearToGamma;	
	float MaxDepthRange;
	bool bOutputStencil;
	uint8_t MipLevel;
};

/** Info for supporting the vertex element types */
class FVertexElementTypeSupportInfo
{
public:
	FVertexElementTypeSupportInfo() { for(int32_t i=0; i<VET_MAX; i++) ElementCaps[i]=true; }
	inline bool IsSupported(EVertexElementType ElementType) { return ElementCaps[ElementType]; }
	inline void SetSupported(EVertexElementType ElementType,bool bIsSupported) { ElementCaps[ElementType]=bIsSupported; }
private:
	/** cap bit set for each VET. One-to-one mapping based on EVertexElementType */
	bool ElementCaps[VET_MAX];
};

struct FVertexElement
{
	uint8_t StreamIndex;
	uint8_t Offset;
	TEnumAsByte<EVertexElementType> Type;
	uint8_t AttributeIndex;
	uint16_t Stride;
	/**
	 * Whether to use instance index or vertex index to consume the element.  
	 * eg if bUseInstanceIndex is 0, the element will be repeated for every instance.
	 */
	uint16_t bUseInstanceIndex;

	FVertexElement() {}
	FVertexElement(uint8_t InStreamIndex,uint8_t InOffset,EVertexElementType InType,uint8_t InAttributeIndex,uint16_t InStride,bool bInUseInstanceIndex = false):
		StreamIndex(InStreamIndex),
		Offset(InOffset),
		Type(InType),
		AttributeIndex(InAttributeIndex),
		Stride(InStride),
		bUseInstanceIndex(bInUseInstanceIndex)
	{}
	/**
	* Suppress the compiler generated assignment operator so that padding won't be copied.
	* This is necessary to get expected results for code that zeros, assigns and then CRC's the whole struct.
	*/
	void operator=(const FVertexElement& Other)
	{
		StreamIndex = Other.StreamIndex;
		Offset = Other.Offset;
		Type = Other.Type;
		AttributeIndex = Other.AttributeIndex;
		bUseInstanceIndex = Other.bUseInstanceIndex;
	}

	//friend FArchive& operator<<(FArchive& Ar,FVertexElement& Element)
	//{
	//	Ar << Element.StreamIndex;
	//	Ar << Element.Offset;
	//	Ar << Element.Type;
	//	Ar << Element.AttributeIndex;
	//	Ar << Element.Stride;
	//	Ar << Element.bUseInstanceIndex;
	//	return Ar;
	//}
	 std::string ToString() const;
	 void FromString(const std::string& Src);
};

typedef std::vector<FVertexElement > FVertexDeclarationElementList;

/** RHI representation of a single stream out element. */
struct FStreamOutElement
{
	/** Index of the output stream from the geometry shader. */
	uint32_t Stream;

	/** Semantic name of the output element as defined in the geometry shader.  This should not contain the semantic number. */
	const char* SemanticName;

	/** Semantic index of the output element as defined in the geometry shader.  For example "TEXCOORD5" in the shader would give a SemanticIndex of 5. */
	uint32_t SemanticIndex;

	/** Start component index of the shader output element to stream out. */
	uint8_t StartComponent;

	/** Number of components of the shader output element to stream out. */
	uint8_t ComponentCount;

	/** Stream output target slot, corresponding to the streams set by RHISetStreamOutTargets. */
	uint8_t OutputSlot;

	FStreamOutElement() {}
	FStreamOutElement(uint32_t InStream, const char* InSemanticName, uint32_t InSemanticIndex, uint8_t InComponentCount, uint8_t InOutputSlot) :
		Stream(InStream),
		SemanticName(InSemanticName),
		SemanticIndex(InSemanticIndex),
		StartComponent(0),
		ComponentCount(InComponentCount),
		OutputSlot(InOutputSlot)
	{}
};

typedef std::vector<FStreamOutElement > FStreamOutElementList;

struct FSamplerStateInitializerRHI
{
	FSamplerStateInitializerRHI() {}
	FSamplerStateInitializerRHI(
		ESamplerFilter InFilter,
		ESamplerAddressMode InAddressU = AM_Wrap,
		ESamplerAddressMode InAddressV = AM_Wrap,
		ESamplerAddressMode InAddressW = AM_Wrap,
		float InMipBias = 0,
		int32_t InMaxAnisotropy = 0,
		float InMinMipLevel = 0,
		float InMaxMipLevel = FLT_MAX,
		uint32_t InBorderColor = 0,
		/** Only supported in D3D11 */
		ESamplerCompareFunction InSamplerComparisonFunction = SCF_Never
		)
	:	Filter(InFilter)
	,	AddressU(InAddressU)
	,	AddressV(InAddressV)
	,	AddressW(InAddressW)
	,	MipBias(InMipBias)
	,	MinMipLevel(InMinMipLevel)
	,	MaxMipLevel(InMaxMipLevel)
	,	MaxAnisotropy(InMaxAnisotropy)
	,	BorderColor(InBorderColor)
	,	SamplerComparisonFunction(InSamplerComparisonFunction)
	{
	}
	TEnumAsByte<ESamplerFilter> Filter;
	TEnumAsByte<ESamplerAddressMode> AddressU;
	TEnumAsByte<ESamplerAddressMode> AddressV;
	TEnumAsByte<ESamplerAddressMode> AddressW;
	float MipBias;
	/** Smallest mip map level that will be used, where 0 is the highest resolution mip level. */
	float MinMipLevel;
	/** Largest mip map level that will be used, where 0 is the highest resolution mip level. */
	float MaxMipLevel;
	int32_t MaxAnisotropy;
	uint32_t BorderColor;
	TEnumAsByte<ESamplerCompareFunction> SamplerComparisonFunction;

	//friend FArchive& operator<<(FArchive& Ar,FSamplerStateInitializerRHI& SamplerStateInitializer)
	//{
	//	Ar << SamplerStateInitializer.Filter;
	//	Ar << SamplerStateInitializer.AddressU;
	//	Ar << SamplerStateInitializer.AddressV;
	//	Ar << SamplerStateInitializer.AddressW;
	//	Ar << SamplerStateInitializer.MipBias;
	//	Ar << SamplerStateInitializer.MinMipLevel;
	//	Ar << SamplerStateInitializer.MaxMipLevel;
	//	Ar << SamplerStateInitializer.MaxAnisotropy;
	//	Ar << SamplerStateInitializer.BorderColor;
	//	Ar << SamplerStateInitializer.SamplerComparisonFunction;
	//	return Ar;
	//}
};

struct FRasterizerStateInitializerRHI
{
	TEnumAsByte<ERasterizerFillMode> FillMode;
	TEnumAsByte<ERasterizerCullMode> CullMode;
	float DepthBias;
	float SlopeScaleDepthBias;
	bool bAllowMSAA;
	bool bEnableLineAA;
	
	//friend FArchive& operator<<(FArchive& Ar,FRasterizerStateInitializerRHI& RasterizerStateInitializer)
	//{
	//	Ar << RasterizerStateInitializer.FillMode;
	//	Ar << RasterizerStateInitializer.CullMode;
	//	Ar << RasterizerStateInitializer.DepthBias;
	//	Ar << RasterizerStateInitializer.SlopeScaleDepthBias;
	//	Ar << RasterizerStateInitializer.bAllowMSAA;
	//	Ar << RasterizerStateInitializer.bEnableLineAA;
	//	return Ar;
	//}
};

struct FDepthStencilStateInitializerRHI
{
	bool bEnableDepthWrite;
	TEnumAsByte<ECompareFunction> DepthTest;

	bool bEnableFrontFaceStencil;
	TEnumAsByte<ECompareFunction> FrontFaceStencilTest;
	TEnumAsByte<EStencilOp> FrontFaceStencilFailStencilOp;
	TEnumAsByte<EStencilOp> FrontFaceDepthFailStencilOp;
	TEnumAsByte<EStencilOp> FrontFacePassStencilOp;
	bool bEnableBackFaceStencil;
	TEnumAsByte<ECompareFunction> BackFaceStencilTest;
	TEnumAsByte<EStencilOp> BackFaceStencilFailStencilOp;
	TEnumAsByte<EStencilOp> BackFaceDepthFailStencilOp;
	TEnumAsByte<EStencilOp> BackFacePassStencilOp;
	uint8_t StencilReadMask;
	uint8_t StencilWriteMask;

	FDepthStencilStateInitializerRHI(
		bool bInEnableDepthWrite = true,
		ECompareFunction InDepthTest = CF_LessEqual,
		bool bInEnableFrontFaceStencil = false,
		ECompareFunction InFrontFaceStencilTest = CF_Always,
		EStencilOp InFrontFaceStencilFailStencilOp = SO_Keep,
		EStencilOp InFrontFaceDepthFailStencilOp = SO_Keep,
		EStencilOp InFrontFacePassStencilOp = SO_Keep,
		bool bInEnableBackFaceStencil = false,
		ECompareFunction InBackFaceStencilTest = CF_Always,
		EStencilOp InBackFaceStencilFailStencilOp = SO_Keep,
		EStencilOp InBackFaceDepthFailStencilOp = SO_Keep,
		EStencilOp InBackFacePassStencilOp = SO_Keep,
		uint8_t InStencilReadMask = 0xFF,
		uint8_t InStencilWriteMask = 0xFF
		)
	: bEnableDepthWrite(bInEnableDepthWrite)
	, DepthTest(InDepthTest)
	, bEnableFrontFaceStencil(bInEnableFrontFaceStencil)
	, FrontFaceStencilTest(InFrontFaceStencilTest)
	, FrontFaceStencilFailStencilOp(InFrontFaceStencilFailStencilOp)
	, FrontFaceDepthFailStencilOp(InFrontFaceDepthFailStencilOp)
	, FrontFacePassStencilOp(InFrontFacePassStencilOp)
	, bEnableBackFaceStencil(bInEnableBackFaceStencil)
	, BackFaceStencilTest(InBackFaceStencilTest)
	, BackFaceStencilFailStencilOp(InBackFaceStencilFailStencilOp)
	, BackFaceDepthFailStencilOp(InBackFaceDepthFailStencilOp)
	, BackFacePassStencilOp(InBackFacePassStencilOp)
	, StencilReadMask(InStencilReadMask)
	, StencilWriteMask(InStencilWriteMask)
	{}
	
	//friend FArchive& operator<<(FArchive& Ar,FDepthStencilStateInitializerRHI& DepthStencilStateInitializer)
	//{
	//	Ar << DepthStencilStateInitializer.bEnableDepthWrite;
	//	Ar << DepthStencilStateInitializer.DepthTest;
	//	Ar << DepthStencilStateInitializer.bEnableFrontFaceStencil;
	//	Ar << DepthStencilStateInitializer.FrontFaceStencilTest;
	//	Ar << DepthStencilStateInitializer.FrontFaceStencilFailStencilOp;
	//	Ar << DepthStencilStateInitializer.FrontFaceDepthFailStencilOp;
	//	Ar << DepthStencilStateInitializer.FrontFacePassStencilOp;
	//	Ar << DepthStencilStateInitializer.bEnableBackFaceStencil;
	//	Ar << DepthStencilStateInitializer.BackFaceStencilTest;
	//	Ar << DepthStencilStateInitializer.BackFaceStencilFailStencilOp;
	//	Ar << DepthStencilStateInitializer.BackFaceDepthFailStencilOp;
	//	Ar << DepthStencilStateInitializer.BackFacePassStencilOp;
	//	Ar << DepthStencilStateInitializer.StencilReadMask;
	//	Ar << DepthStencilStateInitializer.StencilWriteMask;
	//	return Ar;
	//}
	 std::string ToString() const;
	 void FromString(const std::string& Src);


};

class FBlendStateInitializerRHI
{
public:

	struct FRenderTarget
	{
		enum
		{
			NUM_STRING_FIELDS = 7
		};
		TEnumAsByte<EBlendOperation> ColorBlendOp;
		TEnumAsByte<EBlendFactor> ColorSrcBlend;
		TEnumAsByte<EBlendFactor> ColorDestBlend;
		TEnumAsByte<EBlendOperation> AlphaBlendOp;
		TEnumAsByte<EBlendFactor> AlphaSrcBlend;
		TEnumAsByte<EBlendFactor> AlphaDestBlend;
		TEnumAsByte<EColorWriteMask> ColorWriteMask;
		
		FRenderTarget(
			EBlendOperation InColorBlendOp = BO_Add,
			EBlendFactor InColorSrcBlend = BF_One,
			EBlendFactor InColorDestBlend = BF_Zero,
			EBlendOperation InAlphaBlendOp = BO_Add,
			EBlendFactor InAlphaSrcBlend = BF_One,
			EBlendFactor InAlphaDestBlend = BF_Zero,
			EColorWriteMask InColorWriteMask = CW_RGBA
			)
		: ColorBlendOp(InColorBlendOp)
		, ColorSrcBlend(InColorSrcBlend)
		, ColorDestBlend(InColorDestBlend)
		, AlphaBlendOp(InAlphaBlendOp)
		, AlphaSrcBlend(InAlphaSrcBlend)
		, AlphaDestBlend(InAlphaDestBlend)
		, ColorWriteMask(InColorWriteMask)
		{}
		
		/*friend FArchive& operator<<(FArchive& Ar,FRenderTarget& RenderTarget)
		{
			Ar << RenderTarget.ColorBlendOp;
			Ar << RenderTarget.ColorSrcBlend;
			Ar << RenderTarget.ColorDestBlend;
			Ar << RenderTarget.AlphaBlendOp;
			Ar << RenderTarget.AlphaSrcBlend;
			Ar << RenderTarget.AlphaDestBlend;
			Ar << RenderTarget.ColorWriteMask;
			return Ar;
		}*/
		 std::string ToString() const;
		 void FromString(const std::vector<std::string>& Parts, int32_t Index);


	};

	FBlendStateInitializerRHI() {}

	FBlendStateInitializerRHI(const FRenderTarget& InRenderTargetBlendState)
	:	bUseIndependentRenderTargetBlendStates(false)
	{
		RenderTargets[0] = InRenderTargetBlendState;
	}

	template<uint32_t NumRenderTargets>
	FBlendStateInitializerRHI(const std::vector<FRenderTarget>& InRenderTargetBlendStates)
	:	bUseIndependentRenderTargetBlendStates(NumRenderTargets > 1)
	{
		static_assert(NumRenderTargets <= MaxSimultaneousRenderTargets, "Too many render target blend states.");

		for(uint32_t RenderTargetIndex = 0;RenderTargetIndex < NumRenderTargets;++RenderTargetIndex)
		{
			RenderTargets[RenderTargetIndex] = InRenderTargetBlendStates[RenderTargetIndex];
		}
	}

	std::vector<FRenderTarget> RenderTargets;
	bool bUseIndependentRenderTargetBlendStates;
	
	//friend FArchive& operator<<(FArchive& Ar,FBlendStateInitializerRHI& BlendStateInitializer)
	//{
	//	Ar << BlendStateInitializer.RenderTargets;
	//	Ar << BlendStateInitializer.bUseIndependentRenderTargetBlendStates;
	//	return Ar;
	//}
	 std::string ToString() const;
	 void FromString(const std::string& Src);


};

/**
 *	Screen Resolution
 */
struct FScreenResolutionRHI
{
	uint32_t	Width;
	uint32_t	Height;
	uint32_t	RefreshRate;
};

/**
 *	Viewport bounds structure to set multiple view ports for the geometry shader
 *  (needs to be 1:1 to the D3D11 structure)
 */
struct FViewportBounds
{
	float	TopLeftX;
	float	TopLeftY;
	float	Width;
	float	Height;
	float	MinDepth;
	float	MaxDepth;

	FViewportBounds() {}

	FViewportBounds(float InTopLeftX, float InTopLeftY, float InWidth, float InHeight, float InMinDepth = 0.0f, float InMaxDepth = 1.0f)
		:TopLeftX(InTopLeftX), TopLeftY(InTopLeftY), Width(InWidth), Height(InHeight), MinDepth(InMinDepth), MaxDepth(InMaxDepth)
	{
	}

	//friend FArchive& operator<<(FArchive& Ar,FViewportBounds& ViewportBounds)
	//{
	//	Ar << ViewportBounds.TopLeftX;
	//	Ar << ViewportBounds.TopLeftY;
	//	Ar << ViewportBounds.Width;
	//	Ar << ViewportBounds.Height;
	//	Ar << ViewportBounds.MinDepth;
	//	Ar << ViewportBounds.MaxDepth;
	//	return Ar;
	//}
};


typedef std::vector<FScreenResolutionRHI>	FScreenResolutionArray;

struct FVRamAllocation
{
	FVRamAllocation(uint32_t InAllocationStart = 0, uint32_t InAllocationSize = 0)
		: AllocationStart(InAllocationStart)
		, AllocationSize(InAllocationSize)
	{
	}

	bool IsValid() const { return AllocationSize > 0; }

	// in bytes
	uint32_t AllocationStart;
	// in bytes
	uint32_t AllocationSize;
};

struct FRHIResourceInfo
{
	FVRamAllocation VRamAllocation;
};

enum class EClearBinding
{
	ENoneBound, //no clear color associated with this target.  Target will not do hardware clears on most platforms
	EColorBound, //target has a clear color bound.  Clears will use the bound color, and do hardware clears.
	EDepthStencilBound, //target has a depthstencil value bound.  Clears will use the bound values and do hardware clears.
};

struct FClearValueBinding
{
	struct DSVAlue
	{
		float Depth;
		uint32_t Stencil;
	};

	FClearValueBinding()
		: ColorBinding(EClearBinding::EColorBound)
	{
		Value.Color[0] = 0.0f;
		Value.Color[1] = 0.0f;
		Value.Color[2] = 0.0f;
		Value.Color[3] = 0.0f;
	}

	FClearValueBinding(EClearBinding NoBinding)
		: ColorBinding(NoBinding)
	{
		assert(ColorBinding == EClearBinding::ENoneBound);
	}

	explicit FClearValueBinding(const FLinearColor& InClearColor)
		: ColorBinding(EClearBinding::EColorBound)
	{
		Value.Color[0] = InClearColor.R;
		Value.Color[1] = InClearColor.G;
		Value.Color[2] = InClearColor.B;
		Value.Color[3] = InClearColor.A;
	}

	explicit FClearValueBinding(float DepthClearValue, uint32_t StencilClearValue = 0)
		: ColorBinding(EClearBinding::EDepthStencilBound)
	{
		Value.DSValue.Depth = DepthClearValue;
		Value.DSValue.Stencil = StencilClearValue;
	}

	FLinearColor GetClearColor() const
	{
		assert(ColorBinding == EClearBinding::EColorBound);
		return FLinearColor(Value.Color[0], Value.Color[1], Value.Color[2], Value.Color[3]);
	}

	void GetDepthStencil(float& OutDepth, uint32_t& OutStencil) const
	{
		assert(ColorBinding == EClearBinding::EDepthStencilBound);
		OutDepth = Value.DSValue.Depth;
		OutStencil = Value.DSValue.Stencil;
	}

	bool operator==(const FClearValueBinding& Other) const
	{
		if (ColorBinding == Other.ColorBinding)
		{
			if (ColorBinding == EClearBinding::EColorBound)
			{
				return
					Value.Color[0] == Other.Value.Color[0] &&
					Value.Color[1] == Other.Value.Color[1] &&
					Value.Color[2] == Other.Value.Color[2] &&
					Value.Color[3] == Other.Value.Color[3];

			}
			if (ColorBinding == EClearBinding::EDepthStencilBound)
			{
				return
					Value.DSValue.Depth == Other.Value.DSValue.Depth &&
					Value.DSValue.Stencil == Other.Value.DSValue.Stencil;
			}
			return true;
		}
		return false;
	}

	EClearBinding ColorBinding;

	union ClearValueType
	{
		float Color[4];
		DSVAlue DSValue;
	} Value;

	// common clear values
	static  const FClearValueBinding None;
	static  const FClearValueBinding Black;
	static  const FClearValueBinding White;
	static  const FClearValueBinding Transparent;
	static  const FClearValueBinding DepthOne;
	static  const FClearValueBinding DepthZero;
	static  const FClearValueBinding DepthNear;
	static  const FClearValueBinding DepthFar;	
	static  const FClearValueBinding Green;
	static  const FClearValueBinding DefaultNormal8Bit;
};

struct FRHIResourceCreateInfo
{
	FRHIResourceCreateInfo()
		: BulkData(nullptr)
		, ResourceArray(nullptr)
		, ClearValueBinding(FLinearColor::Transparent)
		, DebugName(NULL)
	{}

	// for CreateTexture calls
	FRHIResourceCreateInfo(FResourceBulkDataInterface* InBulkData)
		: BulkData(InBulkData)
		, ResourceArray(nullptr)
		, ClearValueBinding(FLinearColor::Transparent)
		, DebugName(NULL)
	{}

	// for CreateVertexBuffer/CreateStructuredBuffer calls
	FRHIResourceCreateInfo(FResourceArrayInterface* InResourceArray)
		: BulkData(nullptr)
		, ResourceArray(InResourceArray)
		, ClearValueBinding(FLinearColor::Transparent)
		, DebugName(NULL)
	{}

	FRHIResourceCreateInfo(const FClearValueBinding& InClearValueBinding)
		: BulkData(nullptr)
		, ResourceArray(nullptr)
		, ClearValueBinding(InClearValueBinding)
		, DebugName(NULL)
	{
	}

	// for CreateTexture calls
	FResourceBulkDataInterface* BulkData;
	// for CreateVertexBuffer/CreateStructuredBuffer calls
	FResourceArrayInterface* ResourceArray;

	// for binding clear colors to rendertargets.
	FClearValueBinding ClearValueBinding;
	const char* DebugName;
};

// Forward-declaration.
struct FResolveParams;

struct FResolveRect
{
	int32_t X1;
	int32_t Y1;
	int32_t X2;
	int32_t Y2;
	// e.g. for a a full 256 x 256 area starting at (0, 0) it would be 
	// the values would be 0, 0, 256, 256
	inline FResolveRect(int32_t InX1=-1, int32_t InY1=-1, int32_t InX2=-1, int32_t InY2=-1)
	:	X1(InX1)
	,	Y1(InY1)
	,	X2(InX2)
	,	Y2(InY2)
	{}

	inline FResolveRect(const FResolveRect& Other)
		: X1(Other.X1)
		, Y1(Other.Y1)
		, X2(Other.X2)
		, Y2(Other.Y2)
	{}

	bool IsValid() const
	{
		return X1 >= 0 && Y1 >= 0 && X2 - X1 > 0 && Y2 - Y1 > 0;
	}

	//friend FArchive& operator<<(FArchive& Ar,FResolveRect& ResolveRect)
	//{
	//	Ar << ResolveRect.X1;
	//	Ar << ResolveRect.Y1;
	//	Ar << ResolveRect.X2;
	//	Ar << ResolveRect.Y2;
	//	return Ar;
	//}
};

struct FResolveParams
{
	/** used to specify face when resolving to a cube map texture */
	ECubeFace CubeFace;
	/** resolve RECT bounded by [X1,Y1]..[X2,Y2]. Or -1 for fullscreen */
	FResolveRect Rect;
	FResolveRect DestRect;
	/** The mip index to resolve in both source and dest. */
	int32_t MipIndex;
	/** Array index to resolve in the source. */
	int32_t SourceArrayIndex;
	/** Array index to resolve in the dest. */
	int32_t DestArrayIndex;

	/** constructor */
	FResolveParams(
		const FResolveRect& InRect = FResolveRect(), 
		ECubeFace InCubeFace = CubeFace_PosX,
		int32_t InMipIndex = 0,
		int32_t InSourceArrayIndex = 0,
		int32_t InDestArrayIndex = 0,
		const FResolveRect& InDestRect = FResolveRect())
		:	CubeFace(InCubeFace)
		,	Rect(InRect)
		,	DestRect(InDestRect)
		,	MipIndex(InMipIndex)
		,	SourceArrayIndex(InSourceArrayIndex)
		,	DestArrayIndex(InDestArrayIndex)
	{}

	inline FResolveParams(const FResolveParams& Other)
		: CubeFace(Other.CubeFace)
		, Rect(Other.Rect)
		, DestRect(Other.DestRect)
		, MipIndex(Other.MipIndex)
		, SourceArrayIndex(Other.SourceArrayIndex)
		, DestArrayIndex(Other.DestArrayIndex)
	{}
};


struct FRHICopyTextureInfo
{
	// Number of texels to copy. Z Must be always be > 0.
	FIntVector Size = {0, 0, 1};

	/** The mip index to copy in both source and dest. */
	int32_t MipIndex = 0;

	/** Array index or cube face to resolve in the source. For Cubemap arrays this would be ArraySlice * 6 + FaceIndex. */
	int32_t SourceArraySlice = 0;
	/** Array index or cube face to resolve in the dest. */
	int32_t DestArraySlice = 0;
	// How many slices or faces to copy.
	int32_t NumArraySlices = 1;

	// 2D copy
	FRHICopyTextureInfo(int32_t InWidth, int32_t InHeight)
		: Size(InWidth, InHeight, 1)
	{
	}

	FRHICopyTextureInfo(const FIntVector& InSize)
		: Size(InSize)
	{
	}

	void AdvanceMip()
	{
		++MipIndex;
		Size.X = YMath::Max(Size.X / 2, 1);
		Size.Y = YMath::Max(Size.Y / 2, 1);
	}
};

enum class EResourceTransitionAccess
{
	EReadable, //transition from write-> read
	EWritable, //transition from read -> write	
	ERWBarrier, // Mostly for UAVs.  Transition to read/write state and always insert a resource barrier.
	ERWNoBarrier, //Mostly UAVs.  Indicates we want R/W access and do not require synchronization for the duration of the RW state.  The initial transition from writable->RWNoBarrier and readable->RWNoBarrier still requires a sync
	ERWSubResBarrier, //For special cases where read/write happens to different subresources of the same resource in the same call.  Inserts a barrier, but read validation will pass.  Temporary until we pass full subresource info to all transition calls.
	EMetaData,		  // For transitioning texture meta data, for example for making readable in shaders
	EMaxAccess,
};

enum class EResourceAliasability
{
	EAliasable, // Make the resource aliasable with other resources
	EUnaliasable, // Make the resource unaliasable with any other resources
};

class  FResourceTransitionUtility
{
public:
	static const std::string ResourceTransitionAccessStrings[(int32_t)EResourceTransitionAccess::EMaxAccess + 1];
};

enum class EResourceTransitionPipeline
{
	EGfxToCompute,
	EComputeToGfx,
	EGfxToGfx,
	EComputeToCompute,	
};

/** specifies an update region for a texture */
struct FUpdateTextureRegion2D
{
	/** offset in texture */
	uint32_t DestX;
	uint32_t DestY;
	
	/** offset in source image data */
	int32_t SrcX;
	int32_t SrcY;
	
	/** size of region to copy */
	uint32_t Width;
	uint32_t Height;

	FUpdateTextureRegion2D()
	{}

	FUpdateTextureRegion2D(uint32_t InDestX, uint32_t InDestY, int32_t InSrcX, int32_t InSrcY, uint32_t InWidth, uint32_t InHeight)
	:	DestX(InDestX)
	,	DestY(InDestY)
	,	SrcX(InSrcX)
	,	SrcY(InSrcY)
	,	Width(InWidth)
	,	Height(InHeight)
	{}

	/*friend FArchive& operator<<(FArchive& Ar,FUpdateTextureRegion2D& Region)
	{
		Ar << Region.DestX;
		Ar << Region.DestY;
		Ar << Region.SrcX;
		Ar << Region.SrcY;
		Ar << Region.Width;
		Ar << Region.Height;
		return Ar;
	}*/
};

/** specifies an update region for a texture */
struct FUpdateTextureRegion3D
{
	/** offset in texture */
	uint32_t DestX;
	uint32_t DestY;
	uint32_t DestZ;

	/** offset in source image data */
	int32_t SrcX;
	int32_t SrcY;
	int32_t SrcZ;

	/** size of region to copy */
	uint32_t Width;
	uint32_t Height;
	uint32_t Depth;

	FUpdateTextureRegion3D()
	{}

	FUpdateTextureRegion3D(uint32_t InDestX, uint32_t InDestY, uint32_t InDestZ, int32_t InSrcX, int32_t InSrcY, int32_t InSrcZ, uint32_t InWidth, uint32_t InHeight, uint32_t InDepth)
	:	DestX(InDestX)
	,	DestY(InDestY)
	,	DestZ(InDestZ)
	,	SrcX(InSrcX)
	,	SrcY(InSrcY)
	,	SrcZ(InSrcZ)
	,	Width(InWidth)
	,	Height(InHeight)
	,	Depth(InDepth)
	{}

	FUpdateTextureRegion3D(FIntVector InDest, FIntVector InSource, FIntVector InSourceSize)
		: DestX(InDest.X)
		, DestY(InDest.Y)
		, DestZ(InDest.Z)
		, SrcX(InSource.X)
		, SrcY(InSource.Y)
		, SrcZ(InSource.Z)
		, Width(InSourceSize.X)
		, Height(InSourceSize.Y)
		, Depth(InSourceSize.Z)
	{}

	//friend FArchive& operator<<(FArchive& Ar,FUpdateTextureRegion3D& Region)
	//{
	//	Ar << Region.DestX;
	//	Ar << Region.DestY;
	//	Ar << Region.DestZ;
	//	Ar << Region.SrcX;
	//	Ar << Region.SrcY;
	//	Ar << Region.SrcZ;
	//	Ar << Region.Width;
	//	Ar << Region.Height;
	//	Ar << Region.Depth;
	//	return Ar;
	//}
};

struct FRHIDispatchIndirectParameters
{
	uint32_t ThreadGroupCountX;
	uint32_t ThreadGroupCountY;
	uint32_t ThreadGroupCountZ;
};

struct FRHIDrawIndirectParameters
{
	uint32_t VertexCountPerInstance;
	uint32_t InstanceCount;
	uint32_t StartVertexLocation;
	uint32_t StartInstanceLocation;
};

struct FRHIDrawIndexedIndirectParameters
{
	uint32_t IndexCountPerInstance;
	uint32_t InstanceCount;
	uint32_t StartIndexLocation;
	int32_t BaseVertexLocation;
	uint32_t StartInstanceLocation;
};


struct FTextureMemoryStats
{
	// Hardware state (never change after device creation):

	// -1 if unknown, in bytes
	int64_t DedicatedVideoMemory;
	// -1 if unknown, in bytes
	int64_t DedicatedSystemMemory;
	// -1 if unknown, in bytes
	int64_t SharedSystemMemory;
	// Total amount of "graphics memory" that we think we can use for all our graphics resources, in bytes. -1 if unknown.
	int64_t TotalGraphicsMemory;

	// Size of allocated memory, in bytes
	int64_t AllocatedMemorySize;
	// Size of the largest memory fragment, in bytes
	int64_t LargestContiguousAllocation;
	// 0 if streaming pool size limitation is disabled, in bytes
	int64_t TexturePoolSize;
	// Upcoming adjustments to allocated memory, in bytes (async reallocations)
	int32_t PendingMemoryAdjustment;

	// defaults
	FTextureMemoryStats()
		: DedicatedVideoMemory(-1)
		, DedicatedSystemMemory(-1)
		, SharedSystemMemory(-1)
		, TotalGraphicsMemory(-1)
		, AllocatedMemorySize(0)
		, LargestContiguousAllocation(0)
		, TexturePoolSize(0)
		, PendingMemoryAdjustment(0)
	{
	}

	bool AreHardwareStatsValid() const
	{
//#if !PLATFORM_HTML5 // TODO: should this be tested with GRHISupportsRHIThread instead? -- seems this would be better done in SynthBenchmarkPrivate.cpp
#if 1 // TODO: should this be tested with GRHISupportsRHIThread instead? -- seems this would be better done in SynthBenchmarkPrivate.cpp
		return (DedicatedVideoMemory >= 0 && DedicatedSystemMemory >= 0 && SharedSystemMemory >= 0);
#else
		return false;
#endif
	}

	bool IsUsingLimitedPoolSize() const
	{
		return TexturePoolSize > 0;
	}

	int64_t ComputeAvailableMemorySize() const
	{
		return YMath::Max(TexturePoolSize - AllocatedMemorySize, (int64_t)0);
	}
};

// RHI counter stats.
//DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("DrawPrimitive calls"),STAT_RHIDrawPrimitiveCalls,STATGROUP_RHI,);
//DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Triangles drawn"),STAT_RHITriangles,STATGROUP_RHI,);
//DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Lines drawn"),STAT_RHILines,STATGROUP_RHI,);

#if STATS
	#define RHI_DRAW_CALL_INC() \
		INC_DWORD_STAT(STAT_RHIDrawPrimitiveCalls); \
		FPlatformAtomics::InterlockedIncrement(&GCurrentNumDrawCallsRHI);

	#define RHI_DRAW_CALL_STATS(PrimitiveType,NumPrimitives) \
		RHI_DRAW_CALL_INC(); \
		INC_DWORD_STAT_BY(STAT_RHITriangles,(uint32_t)(PrimitiveType != PT_LineList ? (NumPrimitives) : 0)); \
		INC_DWORD_STAT_BY(STAT_RHILines,(uint32_t)(PrimitiveType == PT_LineList ? (NumPrimitives) : 0)); \
		FPlatformAtomics::InterlockedAdd(&GCurrentNumPrimitivesDrawnRHI, NumPrimitives);
#else
	#define RHI_DRAW_CALL_INC() \
		FPlatformAtomics::InterlockedIncrement(&GCurrentNumDrawCallsRHI);

	#define RHI_DRAW_CALL_STATS(PrimitiveType,NumPrimitives) \
		FPlatformAtomics::InterlockedAdd(&GCurrentNumPrimitivesDrawnRHI, NumPrimitives); \
		FPlatformAtomics::InterlockedIncrement(&GCurrentNumDrawCallsRHI);
#endif

// RHI memory stats.
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Render target memory 2D"),STAT_RenderTargetMemory2D,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Render target memory 3D"),STAT_RenderTargetMemory3D,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Render target memory Cube"),STAT_RenderTargetMemoryCube,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Texture memory 2D"),STAT_TextureMemory2D,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Texture memory 3D"),STAT_TextureMemory3D,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Texture memory Cube"),STAT_TextureMemoryCube,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Uniform buffer memory"),STAT_UniformBufferMemory,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Index buffer memory"),STAT_IndexBufferMemory,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Vertex buffer memory"),STAT_VertexBufferMemory,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Structured buffer memory"),STAT_StructuredBufferMemory,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Pixel buffer memory"),STAT_PixelBufferMemory,STATGROUP_RHI,FPlatformMemory::MCR_GPU,);
//DECLARE_CYCLE_STAT_EXTERN(TEXT("Get/Create PSO"), STAT_GetOrCreatePSO, STATGROUP_RHI, );


// RHI base resource types.
//#include "RHIResources.h"
//#include "DynamicRHI.h"


/** Initializes the RHI. */
extern  void RHIInit(bool bHasEditorToken);

/** Performs additional RHI initialization before the render thread starts. */
extern  void RHIPostInit(const std::vector<uint32_t>& InPixelFormatByteWidth);

/** Shuts down the RHI. */
extern  void RHIExit();


// the following helper macros allow to safely convert shader types without much code clutter
#define GETSAFERHISHADER_PIXEL(Shader) ((Shader) ? (Shader)->GetPixelShader() : (FPixelShaderRHIParamRef)FPixelShaderRHIRef())
#define GETSAFERHISHADER_VERTEX(Shader) ((Shader) ? (Shader)->GetVertexShader() : (FVertexShaderRHIParamRef)FVertexShaderRHIRef())
#define GETSAFERHISHADER_HULL(Shader) ((Shader) ? (Shader)->GetHullShader() : (FHullShaderRHIParamRef)FHullShaderRHIRef())
#define GETSAFERHISHADER_DOMAIN(Shader) ((Shader) ? (Shader)->GetDomainShader() : (FDomainShaderRHIParamRef)FDomainShaderRHIRef())
#define GETSAFERHISHADER_GEOMETRY(Shader) ((Shader) ? (Shader)->GetGeometryShader() : (FGeometryShaderRHIParamRef)FGeometryShaderRHIRef())
#define GETSAFERHISHADER_COMPUTE(Shader) ((Shader) ? (Shader)->GetComputeShader() : (FComputeShaderRHIParamRef)FComputeShaderRHIRef())

// RHI utility functions that depend on the RHI definitions.
//#include "RHIUtilities.h"
