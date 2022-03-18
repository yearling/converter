// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RHI.cpp: Render Hardware Interface implementation.
=============================================================================*/

#include "RHI/RHI.h"
#include "Engine/LockFreeList.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIDefinitions.h"
#include <cassert>
#include "Template/Template.h"
#include "Math/NumericLimits.h"
//#include "Modules/ModuleManager.h"
//#include "Misc/ConfigCacheIni.h"
//#include "ProfilingDebugging/CsvProfiler.h"

//IMPLEMENT_MODULE(FDefaultModuleImpl, RHI);

/** RHI Logging. */
//DEFINE_LOG_CATEGORY(LogRHI);
//CSV_DEFINE_CATEGORY(RHI, true);

// Define counter stats.
//DEFINE_STAT(STAT_RHIDrawPrimitiveCalls);
//DEFINE_STAT(STAT_RHITriangles);
//DEFINE_STAT(STAT_RHILines);

// Define memory stats.
//DEFINE_STAT(STAT_RenderTargetMemory2D);
//DEFINE_STAT(STAT_RenderTargetMemory3D);
//DEFINE_STAT(STAT_RenderTargetMemoryCube);
//DEFINE_STAT(STAT_TextureMemory2D);
//DEFINE_STAT(STAT_TextureMemory3D);
//DEFINE_STAT(STAT_TextureMemoryCube);
//DEFINE_STAT(STAT_UniformBufferMemory);
//DEFINE_STAT(STAT_IndexBufferMemory);
//DEFINE_STAT(STAT_VertexBufferMemory);
//DEFINE_STAT(STAT_StructuredBufferMemory);
//DEFINE_STAT(STAT_PixelBufferMemory);
//DEFINE_STAT(STAT_GetOrCreatePSO);
//
//static FAutoConsoleVariable CVarUseVulkanRealUBs(
//	("r.Vulkan.UseRealUBs"),
//	1,
//	("0: Emulate uniform buffers on Vulkan SM4/SM5 [default]\n")
//	("1: Use real uniform buffers"),
//	ECVF_ReadOnly
//	);
static int32_t CVarUseVulkanRealUBs = 1;
//
//static TAutoConsoleVariable<int32_t> CVarDisableEngineAndAppRegistration(
//	("r.DisableEngineAndAppRegistration"),
//	0,
//	("If true, disables engine and app registration, to disable GPU driver optimizations during debugging and development\n")
//	("Changes will only take effect in new game/editor instances - can't be changed at runtime.\n"),
//	ECVF_Default);


const std::string FResourceTransitionUtility::ResourceTransitionAccessStrings[(int32_t)EResourceTransitionAccess::EMaxAccess + 1] =
{
	std::string(("EReadable")),
	std::string(("EWritable")),	
	std::string(("ERWBarrier")),
	std::string(("ERWNoBarrier")),
	std::string(("ERWSubResBarrier")),
	std::string(("EMetaData")),
	std::string(("EMaxAccess")),
};

#if STATS
#include "Stats/StatsData.h"
static void DumpRHIMemory(FOutputDevice& OutputDevice)
{
	std::vector<FStatMessage> Stats;
	GetPermanentStats(Stats);

	std::string NAME_STATGROUP_RHI(FStatGroup_STATGROUP_RHI::GetGroupName());
	OutputDevice.Logf(("RHI resource memory (not tracked by our allocator)"));
	int64_t TotalMemory = 0;
	for (int32_t Index = 0; Index < Stats.Num(); Index++)
	{
		FStatMessage const& Meta = Stats[Index];
		std::string LastGroup = Meta.NameAndInfo.GetGroupName();
		if (LastGroup == NAME_STATGROUP_RHI && Meta.NameAndInfo.GetFlag(EStatMetaFlags::IsMemory))
		{
			OutputDevice.Logf(("%s"), *FStatsUtils::DebugPrint(Meta));
			TotalMemory += Meta.GetValue_int64();
		}
	}
	OutputDevice.Logf(("%.3fMB total"), TotalMemory / 1024.f / 1024.f);
}

static FAutoConsoleCommandWithOutputDevice GDumpRHIMemoryCmd(
	("rhi.DumpMemory"),
	("Dumps RHI memory stats to the log"),
	FConsoleCommandWithOutputDeviceDelegate::CreateStatic(DumpRHIMemory)
	);
#endif

//DO NOT USE THE STATIC FLINEARCOLORS TO INITIALIZE THIS STUFF.  
//Static init order is undefined and you will likely end up with bad values on some platforms.
const FClearValueBinding FClearValueBinding::None(EClearBinding::ENoneBound);
const FClearValueBinding FClearValueBinding::Black(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
const FClearValueBinding FClearValueBinding::White(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
const FClearValueBinding FClearValueBinding::Transparent(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
const FClearValueBinding FClearValueBinding::DepthOne(1.0f, 0);
const FClearValueBinding FClearValueBinding::DepthZero(0.0f, 0);
const FClearValueBinding FClearValueBinding::DepthNear((float)ERHIZBuffer::NearPlane, 0);
const FClearValueBinding FClearValueBinding::DepthFar((float)ERHIZBuffer::FarPlane, 0);
const FClearValueBinding FClearValueBinding::Green(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f));
// Note: this is used as the default normal for DBuffer decals.  It must decode to a value of 0 in DecodeDBufferData.
const FClearValueBinding FClearValueBinding::DefaultNormal8Bit(FLinearColor(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1.0f));

TLockFreePointerListUnordered<FRHIResource, PLATFORM_CACHE_LINE_SIZE> FRHIResource::PendingDeletes;
FRHIResource* FRHIResource::CurrentlyDeleting = nullptr;
std::vector<FRHIResource::ResourcesToDelete> FRHIResource::DeferredDeletionQueue;
uint32_t FRHIResource::CurrentFrame = 0;

std::string FVertexElement::ToString() const
{
	assert(0);
	return "";
	/*
	return std::string::Printf(("<%u %u %u %u %u %u>")
		, uint32_t(StreamIndex)
		, uint32_t(Offset)
		, uint32_t(Type)
		, uint32_t(AttributeIndex)
		, uint32_t(Stride)
		, uint32_t(bUseInstanceIndex)
	);
	*/
}

void FVertexElement::FromString(const std::string& InSrc)
{
	assert(0);
	/*
	std::string Src = InSrc;
	Src.ReplaceInline(("\r"), (" "));
	Src.ReplaceInline(("\n"), (" "));
	Src.ReplaceInline(("\t"), (" "));
	Src.ReplaceInline(("<"), (" "));
	Src.ReplaceInline((">"), (" "));
	std::vector<std::string> Parts;
	Src.TrimStartAndEnd().ParseIntoArray(Parts, (" "));

	assert(Parts.Num() == 6 && sizeof(Type) == 1); //not a very robust parser
	LexFromString(StreamIndex, *Parts[0]);
	LexFromString(Offset, *Parts[1]);
	LexFromString((uint8&)Type, *Parts[2]);
	LexFromString(AttributeIndex, *Parts[3]);
	LexFromString(Stride, *Parts[4]);
	LexFromString(bUseInstanceIndex, *Parts[5]);
	*/
}

std::string FDepthStencilStateInitializerRHI::ToString() const
{
	assert(0);
	return "";
	/*
	return
		std::string::Printf(("<%u %u ")
			, uint32_t(!!bEnableDepthWrite)
			, uint32_t(DepthTest)
		)
		+ std::string::Printf(("%u %u %u %u %u ")
			, uint32_t(!!bEnableFrontFaceStencil)
			, uint32_t(FrontFaceStencilTest)
			, uint32_t(FrontFaceStencilFailStencilOp)
			, uint32_t(FrontFaceDepthFailStencilOp)
			, uint32_t(FrontFacePassStencilOp)
		)
		+ std::string::Printf(("%u %u %u %u %u ")
			, uint32_t(!!bEnableBackFaceStencil)
			, uint32_t(BackFaceStencilTest)
			, uint32_t(BackFaceStencilFailStencilOp)
			, uint32_t(BackFaceDepthFailStencilOp)
			, uint32_t(BackFacePassStencilOp)
		)
		+ std::string::Printf(("%u %u>")
			, uint32_t(StencilReadMask)
			, uint32_t(StencilWriteMask)
		);
		*/
}
void FDepthStencilStateInitializerRHI::FromString(const std::string& InSrc)
{
	assert(0);
	/*
	std::string Src = InSrc;
	Src.ReplaceInline(("\r"), (" "));
	Src.ReplaceInline(("\n"), (" "));
	Src.ReplaceInline(("\t"), (" "));
	Src.ReplaceInline(("<"), (" "));
	Src.ReplaceInline((">"), (" "));
	std::vector<std::string> Parts;
	Src.TrimStartAndEnd().ParseIntoArray(Parts, (" "));

	assert(Parts.Num() == 14 && sizeof(bool) == 1 && sizeof(FrontFaceStencilFailStencilOp) == 1 && sizeof(BackFaceStencilTest) == 1 && sizeof(BackFaceDepthFailStencilOp) == 1); //not a very robust parser

	LexFromString((uint8&)bEnableDepthWrite, *Parts[0]);
	LexFromString((uint8&)DepthTest, *Parts[1]);

	LexFromString((uint8&)bEnableFrontFaceStencil, *Parts[2]);
	LexFromString((uint8&)FrontFaceStencilTest, *Parts[3]);
	LexFromString((uint8&)FrontFaceStencilFailStencilOp, *Parts[4]);
	LexFromString((uint8&)FrontFaceDepthFailStencilOp, *Parts[5]);
	LexFromString((uint8&)FrontFacePassStencilOp, *Parts[6]);

	LexFromString((uint8&)bEnableBackFaceStencil, *Parts[7]);
	LexFromString((uint8&)BackFaceStencilTest, *Parts[8]);
	LexFromString((uint8&)BackFaceStencilFailStencilOp, *Parts[9]);
	LexFromString((uint8&)BackFaceDepthFailStencilOp, *Parts[10]);
	LexFromString((uint8&)BackFacePassStencilOp, *Parts[11]);

	LexFromString(StencilReadMask, *Parts[12]);
	LexFromString(StencilWriteMask, *Parts[13]);
	*/
}

std::string FBlendStateInitializerRHI::ToString() const
{
	assert(0);
	return "";
	/*
	std::string Result = ("<");
	for (int32_t Index = 0; Index < MaxSimultaneousRenderTargets; Index++)
	{
		Result += RenderTargets[Index].ToString();
	}
	Result += std::string::Printf(("%d>"), uint32_t(!!bUseIndependentRenderTargetBlendStates));
	return Result;
	*/
}

void FBlendStateInitializerRHI::FromString(const std::string& InSrc)
{
	assert(0);
	/*
	std::string Src = InSrc;
	Src.ReplaceInline(("\r"), (" "));
	Src.ReplaceInline(("\n"), (" "));
	Src.ReplaceInline(("\t"), (" "));
	Src.ReplaceInline(("<"), (" "));
	Src.ReplaceInline((">"), (" "));
	std::vector<std::string> Parts;
	Src.TrimStartAndEnd().ParseIntoArray(Parts, (" "));


	assert(Parts.Num() == MaxSimultaneousRenderTargets * FRenderTarget::NUM_STRING_FIELDS + 1 && sizeof(bool) == 1); //not a very robust parser
	for (int32_t Index = 0; Index < MaxSimultaneousRenderTargets; Index++)
	{
		RenderTargets[Index].FromString(Parts, FRenderTarget::NUM_STRING_FIELDS * Index);
	}
	LexFromString((int8&)bUseIndependentRenderTargetBlendStates, *Parts[MaxSimultaneousRenderTargets * FRenderTarget::NUM_STRING_FIELDS]);
	*/
}


std::string FBlendStateInitializerRHI::FRenderTarget::ToString() const
{
	assert(0);
	return "";
	/*
	return std::string::Printf(("%u %u %u %u %u %u %u ")
		, uint32_t(ColorBlendOp)
		, uint32_t(ColorSrcBlend)
		, uint32_t(ColorDestBlend)
		, uint32_t(AlphaBlendOp)
		, uint32_t(AlphaSrcBlend)
		, uint32_t(AlphaDestBlend)
		, uint32_t(ColorWriteMask)
	);
	*/
}

void FBlendStateInitializerRHI::FRenderTarget::FromString(const std::vector<std::string>& Parts, int32_t Index)
{
	assert(0);
	/*
	assert(Index + NUM_STRING_FIELDS <= Parts.Num());
	LexFromString((uint8&)ColorBlendOp, *Parts[Index++]);
	LexFromString((uint8&)ColorSrcBlend, *Parts[Index++]);
	LexFromString((uint8&)ColorDestBlend, *Parts[Index++]);
	LexFromString((uint8&)AlphaBlendOp, *Parts[Index++]);
	LexFromString((uint8&)AlphaSrcBlend, *Parts[Index++]);
	LexFromString((uint8&)AlphaDestBlend, *Parts[Index++]);
	LexFromString((uint8&)ColorWriteMask, *Parts[Index++]);
	*/
}

bool FRHIResource::Bypass()
{
	return GRHICommandList.Bypass();
}

//DECLARE_CYCLE_STAT(("Delete Resources"), STAT_DeleteResources, STATGROUP_RHICMDLIST);

void FRHIResource::FlushPendingDeletes(bool bFlushDeferredDeletes)
{
	//SCOPE_CYCLE_COUNTER(STAT_DeleteResources);

	assert(IsInRenderingThread());
	FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThread);
	FRHICommandListExecutor::CheckNoOutstandingCmdLists();
	if (GDynamicRHI)
	{
		GDynamicRHI->RHIPerFrameRHIFlushComplete();
	}

	auto Delete = [](std::vector<FRHIResource*>& ToDelete)
	{
		for (int32_t Index = 0; Index < ToDelete.size(); Index++)
		{
			FRHIResource* Ref = ToDelete[Index];
			assert(Ref->MarkedForDelete == 1);
			if (Ref->GetRefCount() == 0) // caches can bring dead objects back to life
			{
				CurrentlyDeleting = Ref;
				delete Ref;
				CurrentlyDeleting = nullptr;
			}
			else
			{
				Ref->MarkedForDelete = 0;
				FPlatformMisc::MemoryBarrier();
			}
		}
	};

	while (1)
	{
		if (PendingDeletes.IsEmpty())
		{
			break;
		}
		if (PlatformNeedsExtraDeletionLatency())
		{
			//const int32_t Index = DeferredDeletionQueue.AddDefaulted();
			DeferredDeletionQueue.push_back(ResourcesToDelete());
			const int32_t Index = DeferredDeletionQueue.size();
			ResourcesToDelete& ResourceBatch = DeferredDeletionQueue[Index];
			ResourceBatch.FrameDeleted = CurrentFrame;
			PendingDeletes.PopAll(ResourceBatch.Resources);
			assert(ResourceBatch.Resources.size());
		}
		else
		{
			std::vector<FRHIResource*> ToDelete;
			PendingDeletes.PopAll(ToDelete);
			assert(ToDelete.size());
			Delete(ToDelete);
		}
	}

	const uint32_t NumFramesToExpire = 3;

	if (DeferredDeletionQueue.size())
	{
		if (bFlushDeferredDeletes)
		{
			FRHICommandListExecutor::GetImmediateCommandList().BlockUntilGPUIdle();

			for (int32_t Idx = 0; Idx < DeferredDeletionQueue.size(); ++Idx)
			{
				ResourcesToDelete& ResourceBatch = DeferredDeletionQueue[Idx];
				Delete(ResourceBatch.Resources);
			}

			DeferredDeletionQueue.clear();
		}
		else
		{
			int32_t DeletedBatchCount = 0;
			while (DeletedBatchCount < DeferredDeletionQueue.size())
			{
				ResourcesToDelete& ResourceBatch = DeferredDeletionQueue[DeletedBatchCount];
				if (((ResourceBatch.FrameDeleted + NumFramesToExpire) < CurrentFrame) || !GIsRHIInitialized)
				{
					Delete(ResourceBatch.Resources);
					++DeletedBatchCount;
				}
				else
				{
					break;
				}
			}

			if (DeletedBatchCount)
			{
				//DeferredDeletionQueue.RemoveAt(0, DeletedBatchCount);
				DeferredDeletionQueue.erase(DeferredDeletionQueue.begin(), DeferredDeletionQueue.begin()+DeletedBatchCount);
			}
		}

		++CurrentFrame;
	}
}

static_assert(ERHIZBuffer::FarPlane != ERHIZBuffer::NearPlane, "Near and Far planes must be different!");
static_assert((int32_t)ERHIZBuffer::NearPlane == 0 || (int32_t)ERHIZBuffer::NearPlane == 1, "Invalid Values for Near Plane, can only be 0 or 1!");
static_assert((int32_t)ERHIZBuffer::FarPlane == 0 || (int32_t)ERHIZBuffer::FarPlane == 1, "Invalid Values for Far Plane, can only be 0 or 1");


/**
 * RHI configuration settings.
 */

//static TAutoConsoleVariable<int32_t> ResourceTableCachingCvar(
//	("rhi.ResourceTableCaching"),
//	1,
//	("If 1, the RHI will cache resource table contents within a frame. Otherwise resource tables are rebuilt for every draw call.")
//	);
static int32_t ResourceTableCachingCvar = 1;
//static TAutoConsoleVariable<int32_t> GSaveScreenshotAfterProfilingGPUCVar(
//	("r.ProfileGPU.Screenshot"),
//	1,
//	("Whether a screenshot should be taken when profiling the GPU. 0:off, 1:on (default)"),
//	ECVF_RenderThreadSafe);
static int32_t GSaveScreenshotAfterProfilingGPUCVar = 1;
//static TAutoConsoleVariable<int32_t> GShowProfilerAfterProfilingGPUCVar(
//	("r.ProfileGPU.ShowUI"),
//	1,
//	("Whether the user interface profiler should be displayed after profiling the GPU.\n")
//	("The results will always go to the log/console\n")
//	("0:off, 1:on (default)"),
//	ECVF_RenderThreadSafe);
static int32_t GShowProfilerAfterProfilingGPUCVar = 1;
//static TAutoConsoleVariable<float> GGPUHitchThresholdCVar(
//	("RHI.GPUHitchThreshold"),
//	100.0f,
//	("Threshold for detecting hitches on the GPU (in milliseconds).")
//	);
static float GGPUHitchThresholdCVar = 100.0f;

//static TAutoConsoleVariable<int32_t> GCVarRHIRenderPass(
//	("r.RHIRenderPasses"),
//	0,
//	(""),
//	ECVF_Default);
//
static int32_t GCVarRHIRenderPass = 0;

//static TAutoConsoleVariable<int32_t> CVarGPUCrashDebugging(
//	("r.GPUCrashDebugging"),
//	0,
//	("Enable vendor specific GPU crash analysis tools"),
//	ECVF_ReadOnly
//	);
static int32_t CVarGPUCrashDebugging = 0;

namespace RHIConfig
{
	bool ShouldSaveScreenshotAfterProfilingGPU()
	{
		return GSaveScreenshotAfterProfilingGPUCVar != 0;
	}

	bool ShouldShowProfilerAfterProfilingGPU()
	{
		//return GShowProfilerAfterProfilingGPUCVar.GetValueOnAnyThread() != 0;
		return GShowProfilerAfterProfilingGPUCVar != 0;
	}

	float GetGPUHitchThreshold()
	{
		return GGPUHitchThresholdCVar * 0.001f;
	}
}

/**
 * RHI globals.
 */

bool GIsRHIInitialized = false;
int32_t GMaxTextureMipCount = MAX_TEXTURE_MIP_COUNT;
bool GSupportsQuadBufferStereo = false;
bool GSupportsDepthFetchDuringDepthTest = true;
std::string GRHIAdapterName;
std::string GRHIAdapterInternalDriverVersion;
std::string GRHIAdapterUserDriverVersion;
std::string GRHIAdapterDriverDate;
uint32_t GRHIVendorId = 0;
uint32_t GRHIDeviceId = 0;
uint32_t GRHIDeviceRevision = 0;
bool GRHIDeviceIsAMDPreGCNArchitecture = false;
bool GSupportsRenderDepthTargetableShaderResources = true;
TRHIGlobal<bool> GSupportsRenderTargetFormat_PF_G8(true);
TRHIGlobal<bool> GSupportsRenderTargetFormat_PF_FloatRGBA(true);
bool GSupportsShaderFramebufferFetch = false;
bool GSupportsShaderDepthStencilFetch = false;
bool GSupportsTimestampRenderQueries = false;
bool GRHISupportsGPUTimestampBubblesRemoval = false;
bool GRHISupportsFrameCyclesBubblesRemoval = false;
bool GHardwareHiddenSurfaceRemoval = false;
bool GRHISupportsAsyncTextureCreation = false;
bool GRHISupportsQuadTopology = false;
bool GRHISupportsRectTopology = false;
bool GSupportsParallelRenderingTasksWithSeparateRHIThread = true;
bool GRHIThreadNeedsKicking = false;
int32_t GRHIMaximumReccommendedOustandingOcclusionQueries = MAX_int32;
bool GRHISupportsExactOcclusionQueries = true;
bool GSupportsVolumeTextureRendering = true;
bool GSupportsSeparateRenderTargetBlendState = false;
bool GSupportsDepthRenderTargetWithoutColorRenderTarget = true;
bool GRHINeedsUnatlasedCSMDepthsWorkaround = false;
bool GSupportsTexture3D = true;
bool GSupportsMobileMultiView = false;
bool GSupportsImageExternal = false;
bool GSupportsResourceView = true;
TRHIGlobal<bool> GSupportsMultipleRenderTargets(true);
bool GSupportsWideMRT = true;
float GMinClipZ = 0.0f;
float GProjectionSignY = 1.0f;
bool GRHINeedsExtraDeletionLatency = false;
TRHIGlobal<int32_t> GMaxShadowDepthBufferSizeX(2048);
TRHIGlobal<int32_t> GMaxShadowDepthBufferSizeY(2048);
TRHIGlobal<int32_t> GMaxTextureDimensions(2048);
TRHIGlobal<int32_t> GMaxCubeTextureDimensions(2048);
int32_t GMaxTextureArrayLayers = 256;
int32_t GMaxTextureSamplers = 16;
bool GUsingNullRHI = false;
int32_t GDrawUPVertexCheckCount = MAX_int32;
int32_t GDrawUPIndexCheckCount = MAX_int32;
bool GTriggerGPUProfile = false;
std::string GGPUTraceFileName;
bool GRHISupportsTextureStreaming = false;
bool GSupportsDepthBoundsTest = false;
bool GSupportsEfficientAsyncCompute = false;
bool GRHISupportsBaseVertexIndex = true;
TRHIGlobal<bool> GRHISupportsInstancing(true);
bool GRHISupportsFirstInstance = false;
bool GRHISupportsDynamicResolution = false;
bool GRHISupportsRHIThread = false;
bool GRHISupportsRHIOnTaskThread = false;
bool GRHISupportsParallelRHIExecute = false;
bool GSupportsHDR32bppEncodeModeIntrinsic = false;
bool GSupportsParallelOcclusionQueries = false;
bool GSupportsRenderTargetWriteMask = false;
bool GSupportsTransientResourceAliasing = false;
bool GRHIRequiresRenderTargetForPixelShaderUAVs = false;

bool GRHISupportsMSAADepthSampleAccess = false;
bool GRHISupportsResolveCubemapFaces = false;

bool GRHISupportsHDROutput = false;
EPixelFormat GRHIHDRDisplayOutputFormat = PF_FloatRGBA;

uint64_t GRHIPresentCounter = 1;

/** Whether we are profiling GPU hitches. */
bool GTriggerGPUHitchProfile = false;

FVertexElementTypeSupportInfo GVertexElementTypeSupport;

 int32_t volatile GCurrentTextureMemorySize = 0;
 int32_t volatile GCurrentRendertargetMemorySize = 0;
 int64_t GTexturePoolSize = 0 * 1024 * 1024;
 int32_t GPoolSizeVRAMPercentage = 0;

 EShaderPlatform GShaderPlatformForFeatureLevel[ERHIFeatureLevel::Num] = {SP_NumPlatforms,SP_NumPlatforms,SP_NumPlatforms,SP_NumPlatforms};

// simple stats about draw calls. GNum is the previous frame and 
// GCurrent is the current frame.
 int32_t GCurrentNumDrawCallsRHI = 0;
 int32_t GNumDrawCallsRHI = 0;
 int32_t GCurrentNumPrimitivesDrawnRHI = 0;
 int32_t GNumPrimitivesDrawnRHI = 0;

/** Called once per frame only from within an RHI. */
void RHIPrivateBeginFrame()
{
	GNumDrawCallsRHI = GCurrentNumDrawCallsRHI;
	GNumPrimitivesDrawnRHI = GCurrentNumPrimitivesDrawnRHI;
	//CSV_CUSTOM_STAT(RHI, DrawCalls, GNumDrawCallsRHI, ECsvCustomStatOp::Set);
	//CSV_CUSTOM_STAT(RHI, PrimitivesDrawn, GNumPrimitivesDrawnRHI, ECsvCustomStatOp::Set);
	GCurrentNumDrawCallsRHI = GCurrentNumPrimitivesDrawnRHI = 0;
}

/** Whether to initialize 3D textures using a bulk data (or through a mip update if false). */
 bool GUseTexture3DBulkDataRHI = false;

//
// The current shader platform.
//

 EShaderPlatform GMaxRHIShaderPlatform = SP_PCD3D_SM5;

/** The maximum feature level supported on this machine */
 ERHIFeatureLevel::Type GMaxRHIFeatureLevel = ERHIFeatureLevel::SM5;

std::string FeatureLevelNames[] = 
{
	std::string(("ES2")),
	std::string(("ES3_1")),
	std::string(("SM4")),
	std::string(("SM5")),
};

static_assert(ARRAY_COUNT(FeatureLevelNames) == ERHIFeatureLevel::Num, "Missing entry from feature level names.");

 bool GetFeatureLevelFromName(std::string Name, ERHIFeatureLevel::Type& OutFeatureLevel)
{
	for (int32_t NameIndex = 0; NameIndex < ARRAY_COUNT(FeatureLevelNames); NameIndex++)
	{
		if (FeatureLevelNames[NameIndex] == Name)
		{
			OutFeatureLevel = (ERHIFeatureLevel::Type)NameIndex;
			return true;
		}
	}

	OutFeatureLevel = ERHIFeatureLevel::Num;
	return false;
}

// void GetFeatureLevelName(ERHIFeatureLevel::Type InFeatureLevel, std::string& OutName)
//{
//	assert(InFeatureLevel < ARRAY_COUNT(FeatureLevelNames));
//	if (InFeatureLevel < ARRAY_COUNT(FeatureLevelNames))
//	{
//		//FeatureLevelNames[(int32_t)InFeatureLevel].ToString(OutName);
//		OutName = FeatureLevelNames[(int32_t)InFeatureLevel];
//	}
//	else
//	{
//		OutName = ("InvalidFeatureLevel");
//	}	
//}

static std::string InvalidFeatureLevelName(("InvalidFeatureLevel"));
 void GetFeatureLevelName(ERHIFeatureLevel::Type InFeatureLevel, std::string& OutName)
{
	assert(InFeatureLevel < ARRAY_COUNT(FeatureLevelNames));
	if (InFeatureLevel < ARRAY_COUNT(FeatureLevelNames))
	{
		OutName = FeatureLevelNames[(int32_t)InFeatureLevel];
	}
	else
	{
		
		OutName = InvalidFeatureLevelName;
	}
}

static std::string NAME_PCD3D_SM5(("PCD3D_SM5"));
static std::string NAME_PCD3D_SM4(("PCD3D_SM4"));
static std::string NAME_PCD3D_ES3_1(("PCD3D_ES31"));
static std::string NAME_PCD3D_ES2(("PCD3D_ES2"));
static std::string NAME_GLSL_150(("GLSL_150"));
static std::string NAME_SF_PS4(("SF_PS4"));
static std::string NAME_SF_XBOXONE_D3D12(("SF_XBOXONE_D3D12"));
static std::string NAME_GLSL_430(("GLSL_430"));
static std::string NAME_GLSL_150_ES2(("GLSL_150_ES2"));
static std::string NAME_GLSL_150_ES2_NOUB(("GLSL_150_ES2_NOUB"));
static std::string NAME_GLSL_150_ES31(("GLSL_150_ES31"));
static std::string NAME_GLSL_ES2(("GLSL_ES2"));
static std::string NAME_GLSL_ES2_WEBGL(("GLSL_ES2_WEBGL"));
static std::string NAME_GLSL_ES2_IOS(("GLSL_ES2_IOS"));
static std::string NAME_SF_METAL(("SF_METAL"));
static std::string NAME_SF_METAL_MRT(("SF_METAL_MRT"));
static std::string NAME_SF_METAL_MRT_MAC(("SF_METAL_MRT_MAC"));
static std::string NAME_GLSL_310_ES_EXT(("GLSL_310_ES_EXT"));
static std::string NAME_GLSL_ES3_1_ANDROID(("GLSL_ES3_1_ANDROID"));
static std::string NAME_SF_METAL_SM5(("SF_METAL_SM5"));
static std::string NAME_VULKAN_ES3_1_ANDROID(("SF_VULKAN_ES31_ANDROID"));
static std::string NAME_VULKAN_ES3_1_ANDROID_NOUB(("SF_VULKAN_ES31_ANDROID_NOUB"));
static std::string NAME_VULKAN_ES3_1_LUMIN(("SF_VULKAN_ES31_LUMIN"));
static std::string NAME_VULKAN_ES3_1(("SF_VULKAN_ES31"));
static std::string NAME_VULKAN_ES3_1_NOUB(("SF_VULKAN_ES31_NOUB"));
static std::string NAME_VULKAN_SM4_NOUB(("SF_VULKAN_SM4_NOUB"));
static std::string NAME_VULKAN_SM4(("SF_VULKAN_SM4"));
static std::string NAME_VULKAN_SM5_NOUB(("SF_VULKAN_SM5_NOUB"));
static std::string NAME_VULKAN_SM5(("SF_VULKAN_SM5"));
static std::string NAME_VULKAN_SM5_LUMIN(("SF_VULKAN_SM5_LUMIN"));
static std::string NAME_SF_METAL_SM5_NOTESS(("SF_METAL_SM5_NOTESS"));
static std::string NAME_SF_METAL_MACES3_1(("SF_METAL_MACES3_1"));
static std::string NAME_SF_METAL_MACES2(("SF_METAL_MACES2"));
static std::string NAME_GLSL_SWITCH(("GLSL_SWITCH"));
static std::string NAME_GLSL_SWITCH_FORWARD(("GLSL_SWITCH_FORWARD"));

std::string LegacyShaderPlatformToShaderFormat(EShaderPlatform Platform)
{
	switch(Platform)
	{
	case SP_PCD3D_SM5:
		return NAME_PCD3D_SM5;
	case SP_PCD3D_SM4:
		return NAME_PCD3D_SM4;
	case SP_PCD3D_ES3_1:
		return NAME_PCD3D_ES3_1;
	case SP_PCD3D_ES2:
		return NAME_PCD3D_ES2;
	case SP_OPENGL_SM4:
		return NAME_GLSL_150;
	case SP_PS4:
		return NAME_SF_PS4;
	case SP_XBOXONE_D3D12:
		return NAME_SF_XBOXONE_D3D12;
	case SP_OPENGL_SM5:
		return NAME_GLSL_430;
	case SP_OPENGL_PCES2:
	{
		//static auto* CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(("OpenGL.UseEmulatedUBs"));
		//return (CVar && CVar->GetValueOnAnyThread() != 0) ? NAME_GLSL_150_ES2_NOUB : NAME_GLSL_150_ES2;
		return NAME_GLSL_150_ES2;
	}
	case SP_OPENGL_PCES3_1:
		return NAME_GLSL_150_ES31;
	case SP_OPENGL_ES2_ANDROID:
		return NAME_GLSL_ES2;
	case SP_OPENGL_ES2_WEBGL:
		return NAME_GLSL_ES2_WEBGL;
	case SP_OPENGL_ES2_IOS:
		return NAME_GLSL_ES2_IOS;
	case SP_METAL:
		return NAME_SF_METAL;
	case SP_METAL_MRT:
		return NAME_SF_METAL_MRT;
	case SP_METAL_MRT_MAC:
		return NAME_SF_METAL_MRT_MAC;
	case SP_METAL_SM5_NOTESS:
		return NAME_SF_METAL_SM5_NOTESS;
	case SP_METAL_SM5:
		return NAME_SF_METAL_SM5;
	case SP_METAL_MACES3_1:
		return NAME_SF_METAL_MACES3_1;
	case SP_METAL_MACES2:
		return NAME_SF_METAL_MACES2;
	case SP_OPENGL_ES31_EXT:
		return NAME_GLSL_310_ES_EXT;
	case SP_OPENGL_ES3_1_ANDROID:
		return NAME_GLSL_ES3_1_ANDROID;
	case SP_VULKAN_SM4:
		//return (CVarUseVulkanRealUBs->GetInt() != 0) ? NAME_VULKAN_SM4 : NAME_VULKAN_SM4_NOUB;
		return  NAME_VULKAN_SM4; 
	case SP_VULKAN_SM5:
		//return (CVarUseVulkanRealUBs->GetInt() != 0) ? NAME_VULKAN_SM5 : NAME_VULKAN_SM5_NOUB;
		return  NAME_VULKAN_SM5; 
	case SP_VULKAN_SM5_LUMIN:
		return NAME_VULKAN_SM5_LUMIN;
	case SP_VULKAN_ES3_1_LUMIN:
		return NAME_VULKAN_ES3_1_LUMIN;
	case SP_VULKAN_PCES3_1:
		//return (CVarUseVulkanRealUBs->GetInt() != 0) ? NAME_VULKAN_ES3_1 : NAME_VULKAN_ES3_1_NOUB;
		return  NAME_VULKAN_ES3_1;
	case SP_VULKAN_ES3_1_ANDROID:
		//return (CVarUseVulkanRealUBs->GetInt() != 0) ? NAME_VULKAN_ES3_1_ANDROID : NAME_VULKAN_ES3_1_ANDROID_NOUB;
		return NAME_VULKAN_ES3_1_ANDROID;
	case SP_SWITCH:
		return NAME_GLSL_SWITCH;
	case SP_SWITCH_FORWARD:
		return NAME_GLSL_SWITCH_FORWARD;

	default:
		assert(0);
		return NAME_PCD3D_SM5;
	}
}

EShaderPlatform ShaderFormatToLegacyShaderPlatform(std::string ShaderFormat)
{
	if (ShaderFormat == NAME_PCD3D_SM5)					return SP_PCD3D_SM5;	
	if (ShaderFormat == NAME_PCD3D_SM4)					return SP_PCD3D_SM4;
	if (ShaderFormat == NAME_PCD3D_ES3_1)				return SP_PCD3D_ES3_1;
	if (ShaderFormat == NAME_PCD3D_ES2)					return SP_PCD3D_ES2;
	if (ShaderFormat == NAME_GLSL_150)					return SP_OPENGL_SM4;
	if (ShaderFormat == NAME_SF_PS4)					return SP_PS4;
	if (ShaderFormat == NAME_SF_XBOXONE_D3D12)			return SP_XBOXONE_D3D12;
	if (ShaderFormat == NAME_GLSL_430)					return SP_OPENGL_SM5;
	if (ShaderFormat == NAME_GLSL_150_ES2)				return SP_OPENGL_PCES2;
	if (ShaderFormat == NAME_GLSL_150_ES2_NOUB)			return SP_OPENGL_PCES2;
	if (ShaderFormat == NAME_GLSL_150_ES31)				return SP_OPENGL_PCES3_1;
	if (ShaderFormat == NAME_GLSL_ES2)					return SP_OPENGL_ES2_ANDROID;
	if (ShaderFormat == NAME_GLSL_ES2_WEBGL)			return SP_OPENGL_ES2_WEBGL;
	if (ShaderFormat == NAME_GLSL_ES2_IOS)				return SP_OPENGL_ES2_IOS;
	if (ShaderFormat == NAME_SF_METAL)					return SP_METAL;
	if (ShaderFormat == NAME_SF_METAL_MRT)				return SP_METAL_MRT;
	if (ShaderFormat == NAME_SF_METAL_MRT_MAC)			return SP_METAL_MRT_MAC;
	if (ShaderFormat == NAME_GLSL_310_ES_EXT)			return SP_OPENGL_ES31_EXT;
	if (ShaderFormat == NAME_SF_METAL_SM5)				return SP_METAL_SM5;
	if (ShaderFormat == NAME_VULKAN_SM4)				return SP_VULKAN_SM4;
	if (ShaderFormat == NAME_VULKAN_SM5)				return SP_VULKAN_SM5;
	if (ShaderFormat == NAME_VULKAN_SM5_LUMIN)			return SP_VULKAN_SM5_LUMIN;
	if (ShaderFormat == NAME_VULKAN_ES3_1_ANDROID)		return SP_VULKAN_ES3_1_ANDROID;
	if (ShaderFormat == NAME_VULKAN_ES3_1_ANDROID_NOUB)	return SP_VULKAN_ES3_1_ANDROID;
	if (ShaderFormat == NAME_VULKAN_ES3_1_LUMIN)		return SP_VULKAN_ES3_1_LUMIN;
	if (ShaderFormat == NAME_VULKAN_ES3_1)				return SP_VULKAN_PCES3_1;
	if (ShaderFormat == NAME_VULKAN_ES3_1_NOUB)			return SP_VULKAN_PCES3_1;
	if (ShaderFormat == NAME_VULKAN_SM4_NOUB)			return SP_VULKAN_SM4;
	if (ShaderFormat == NAME_VULKAN_SM5_NOUB)			return SP_VULKAN_SM5;
	if (ShaderFormat == NAME_SF_METAL_SM5_NOTESS)		return SP_METAL_SM5_NOTESS;
	if (ShaderFormat == NAME_SF_METAL_MACES3_1)			return SP_METAL_MACES3_1;
	if (ShaderFormat == NAME_SF_METAL_MACES2)			return SP_METAL_MACES2;
	if (ShaderFormat == NAME_GLSL_ES3_1_ANDROID)		return SP_OPENGL_ES3_1_ANDROID;
	if (ShaderFormat == NAME_GLSL_SWITCH)				return SP_SWITCH;
	if (ShaderFormat == NAME_GLSL_SWITCH_FORWARD)		return SP_SWITCH_FORWARD;
	
	return SP_NumPlatforms;
}


 bool IsRHIDeviceAMD()
{
	assert(GRHIVendorId != 0);
	// AMD's drivers tested on July 11 2013 have hitching problems with async resource streaming, setting single threaded for now until fixed.
	return GRHIVendorId == 0x1002;
}

 bool IsRHIDeviceIntel()
{
	assert(GRHIVendorId != 0);
	// Intel GPUs are integrated and use both DedicatedVideoMemory and SharedSystemMemory.
	return GRHIVendorId == 0x8086;
}

 bool IsRHIDeviceNVIDIA()
{
	assert(GRHIVendorId != 0);
	// NVIDIA GPUs are discrete and use DedicatedVideoMemory only.
	return GRHIVendorId == 0x10DE;
}

 const TCHAR* RHIVendorIdToString()
{
	switch (GRHIVendorId)
	{
	case 0x1002: return ("AMD");
	case 0x1010: return ("ImgTec");
	case 0x10DE: return ("NVIDIA");
	case 0x13B5: return ("ARM");
	case 0x5143: return ("Qualcomm");
	case 0x8086: return ("Intel");
	default: return ("Unknown");
	}
}

 uint32_t RHIGetShaderLanguageVersion(const EShaderPlatform Platform)
{
	uint32_t Version = 0;
	if (IsMetalPlatform(Platform))
	{
		if (IsPCPlatform(Platform))
		{
			static int32_t MaxShaderVersion = -1;
			if (MaxShaderVersion < 0)
			{
				MaxShaderVersion = 2;
				//if(!GConfig->GetInt(("/Script/MacTargetPlatform.MacTargetSettings"), ("MaxShaderLanguageVersion"), MaxShaderVersion, GEngineIni))
				//{
					//MaxShaderVersion = 2;
				//}
			}
			Version = (uint32_t)MaxShaderVersion;
		}
		else
		{
			static int32_t MaxShaderVersion = -1;
			if (MaxShaderVersion < 0)
			{
				MaxShaderVersion = 0;
				//if(!GConfig->GetInt(("/Script/IOSRuntimeSettings.IOSRuntimeSettings"), ("MaxShaderLanguageVersion"), MaxShaderVersion, GEngineIni))
				//{
					//MaxShaderVersion = 0;
				//}
			}
			Version = (uint32_t)MaxShaderVersion;
		}
	}
	return Version;
}

 bool RHISupportsTessellation(const EShaderPlatform Platform)
{
	if (IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && !IsMetalPlatform(Platform))
	{
		return (Platform == SP_PCD3D_SM5) || (Platform == SP_XBOXONE_D3D12) || (Platform == SP_OPENGL_SM5) || (Platform == SP_OPENGL_ES31_EXT)/* || (IsVulkanSM5Platform(Platform)*/;
	}
	// For Metal we can only support tessellation if we are willing to sacrifice backward compatibility with OS versions.
	// As such it becomes an opt-in project setting.
	else if (Platform == SP_METAL_SM5)
	{
		return (RHIGetShaderLanguageVersion(Platform) >= 2);
	}
	return false;
}

 bool RHISupportsPixelShaderUAVs(const EShaderPlatform Platform)
{
	if (IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && !IsMetalPlatform(Platform))
	{
		return true;
	}
	else if (Platform == SP_METAL_SM5 || Platform == SP_METAL_SM5_NOTESS || Platform == SP_METAL_MRT || Platform == SP_METAL_MRT_MAC)
	{
		return (RHIGetShaderLanguageVersion(Platform) >= 2);
	}
	return false;
}

 bool RHISupportsIndexBufferUAVs(const EShaderPlatform Platform)
{
	return Platform == SP_PCD3D_SM5 || IsVulkanPlatform(Platform) || Platform == SP_XBOXONE_D3D12 || Platform == SP_PS4;
}

static ERHIFeatureLevel::Type GRHIMobilePreviewFeatureLevel = ERHIFeatureLevel::Num;
 void RHISetMobilePreviewFeatureLevel(ERHIFeatureLevel::Type MobilePreviewFeatureLevel)
{
	assert(MobilePreviewFeatureLevel == ERHIFeatureLevel::ES2 || MobilePreviewFeatureLevel == ERHIFeatureLevel::ES3_1);
	assert(GRHIMobilePreviewFeatureLevel == ERHIFeatureLevel::Num);
	//assert(!GIsEditor);
	GRHIMobilePreviewFeatureLevel = MobilePreviewFeatureLevel;
}

bool RHIGetPreviewFeatureLevel(ERHIFeatureLevel::Type& PreviewFeatureLevelOUT)
{
	//static bool bForceFeatureLevelES2 = !GIsEditor && FParse::Param(FCommandLine::Get(), ("FeatureLevelES2"));
	//static bool bForceFeatureLevelES3_1 = !GIsEditor && (FParse::Param(FCommandLine::Get(), ("FeatureLevelES31")) || FParse::Param(FCommandLine::Get(), ("FeatureLevelES3_1")));
	static bool bForceFeatureLevelES2 = false;
	static bool bForceFeatureLevelES3_1 = false;
	if (bForceFeatureLevelES2)
	{
		PreviewFeatureLevelOUT = ERHIFeatureLevel::ES2;
	}
	else if (bForceFeatureLevelES3_1)
	{
		PreviewFeatureLevelOUT = ERHIFeatureLevel::ES3_1;
	}
	else if (/*!GIsEditor &&*/ GRHIMobilePreviewFeatureLevel != ERHIFeatureLevel::Num)
	{
		PreviewFeatureLevelOUT = GRHIMobilePreviewFeatureLevel;
	}
	else
	{
		return false;
	}
	return true;
}

void FRHIRenderPassInfo::ConvertToRenderTargetsInfo(FRHISetRenderTargetsInfo& OutRTInfo) const
{
	for (int32_t Index = 0; Index < MaxSimultaneousRenderTargets; ++Index)
	{
		if (!ColorRenderTargets[Index].RenderTarget)
		{
			break;
		}

		OutRTInfo.ColorRenderTarget[Index].Texture = ColorRenderTargets[Index].RenderTarget;
		ERenderTargetLoadAction LoadAction = GetLoadAction(ColorRenderTargets[Index].Action);
		OutRTInfo.ColorRenderTarget[Index].LoadAction = LoadAction;
		OutRTInfo.ColorRenderTarget[Index].StoreAction = GetStoreAction(ColorRenderTargets[Index].Action);
		OutRTInfo.ColorRenderTarget[Index].ArraySliceIndex = ColorRenderTargets[Index].ArraySlice;
		OutRTInfo.ColorRenderTarget[Index].MipIndex = ColorRenderTargets[Index].MipIndex;
		++OutRTInfo.NumColorRenderTargets;

		OutRTInfo.bClearColor |= (LoadAction == ERenderTargetLoadAction::EClear);
	}

	ERenderTargetActions DepthActions = GetDepthActions(DepthStencilRenderTarget.Action);
	ERenderTargetActions StencilActions = GetStencilActions(DepthStencilRenderTarget.Action);
	ERenderTargetLoadAction DepthLoadAction = GetLoadAction(DepthActions);
	ERenderTargetStoreAction DepthStoreAction = GetStoreAction(DepthActions);
	ERenderTargetLoadAction StencilLoadAction = GetLoadAction(StencilActions);
	ERenderTargetStoreAction StencilStoreAction = GetStoreAction(StencilActions);

	OutRTInfo.DepthStencilRenderTarget = FRHIDepthRenderTargetView(DepthStencilRenderTarget.DepthStencilTarget,
		DepthLoadAction,
		GetStoreAction(DepthActions),
		StencilLoadAction,
		GetStoreAction(StencilActions),
		DepthStencilRenderTarget.ExclusiveDepthStencil);
	OutRTInfo.bClearDepth = (DepthLoadAction == ERenderTargetLoadAction::EClear);
	OutRTInfo.bClearStencil = (StencilLoadAction == ERenderTargetLoadAction::EClear);

	if (NumUAVs > 0)
	{
		assert(UAVIndex != -1);
		assert(UAVIndex >= OutRTInfo.NumColorRenderTargets);
		OutRTInfo.NumColorRenderTargets = UAVIndex;
		for (int32_t Index = 0; Index < NumUAVs; ++Index)
		{
			OutRTInfo.UnorderedAccessView[Index] = UAVs[Index];
		}
		OutRTInfo.NumUAVs = NumUAVs;
	}
}


void FRHIRenderPassInfo::Validate() const
{
	int32_t NumSamples = -1;	// -1 means nothing found yet
	int32_t ColorIndex = 0;
	for (; ColorIndex < MaxSimultaneousRenderTargets; ++ColorIndex)
	{
		const FColorEntry& Entry = ColorRenderTargets[ColorIndex];
		if (Entry.RenderTarget)
		{
			// Ensure NumSamples matches amongst all color RTs
			if (NumSamples == -1)
			{
				NumSamples = Entry.RenderTarget->GetNumSamples();
			}
			else
			{
				assert(Entry.RenderTarget->GetNumSamples() == NumSamples);
			}

			ERenderTargetStoreAction Store = GetStoreAction(Entry.Action);
			// Don't try to resolve a non-msaa
			assert(Store != ERenderTargetStoreAction::EMultisampleResolve || Entry.RenderTarget->GetNumSamples() > 1);
			// Don't resolve to null
			assert(Store != ERenderTargetStoreAction::EMultisampleResolve || Entry.ResolveTarget);
		}
		else
		{
			break;
		}
	}

	int32_t NumColorRenderTargets = ColorIndex;
	for (; ColorIndex < MaxSimultaneousRenderTargets; ++ColorIndex)
	{
		// Gap in the sequence of valid render targets (ie RT0, null, RT2, ...)
		//ensureMsgf(!ColorRenderTargets[ColorIndex].RenderTarget, ("Missing color render target on slot %d"), ColorIndex - 1);
		assert(!ColorRenderTargets[ColorIndex].RenderTarget);
	}

	if (bGeneratingMips)
	{
		if (NumColorRenderTargets == 0)
		{
			//ensureMsgf(0, ("Missing color render target for which to generate mips!"));
			assert(0);
		}
		else if (NumColorRenderTargets > 1)
		{
			//ensureMsgf(0, ("Too many color render targets for which to generate mips!"));
			assert(0);
		}
	}

	if (DepthStencilRenderTarget.DepthStencilTarget)
	{
		//ensureMsgf(!bGeneratingMips, ("Can't (currently) generate mip maps off a depth/stencil target!"));
		assert(!bGeneratingMips);
		// Ensure NumSamples matches with color RT
		if (NumSamples != -1)
		{
			assert(DepthStencilRenderTarget.DepthStencilTarget->GetNumSamples() == NumSamples);
		}
		ERenderTargetStoreAction DepthStore = GetStoreAction(GetDepthActions(DepthStencilRenderTarget.Action));
		ERenderTargetStoreAction StencilStore = GetStoreAction(GetStencilActions(DepthStencilRenderTarget.Action));
		bool bIsMSAAResolve = (DepthStore == ERenderTargetStoreAction::EMultisampleResolve) || (StencilStore == ERenderTargetStoreAction::EMultisampleResolve);
		// Don't try to resolve a non-msaa
		assert(!bIsMSAAResolve || DepthStencilRenderTarget.DepthStencilTarget->GetNumSamples() > 1);
		// Don't resolve to null
		assert(DepthStencilRenderTarget.ResolveTarget || DepthStore != ERenderTargetStoreAction::EStore);
		// Don't write to depth if read-only
		assert(DepthStencilRenderTarget.ExclusiveDepthStencil.IsDepthWrite() || DepthStore != ERenderTargetStoreAction::EStore);
		assert(DepthStencilRenderTarget.ExclusiveDepthStencil.IsStencilWrite() || StencilStore != ERenderTargetStoreAction::EStore);
	}
	else
	{
		assert(DepthStencilRenderTarget.Action == EDepthStencilTargetActions::DontLoad_DontStore);
		assert(DepthStencilRenderTarget.ExclusiveDepthStencil == FExclusiveDepthStencil::DepthNop_StencilNop);
	}
}
