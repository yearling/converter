#include "RHI/DirectX11/D3D11RHI.h"
#include "RHI/DirectX11/D3D11Util.h"
#include "Render/YRenderThread.h"
#include "Render/RenderUtils.h"
#include "Engine/List.h"
#include "Render/RenderResource.h"
#include <delayimp.h>


YD3D11DynamicRHI::YD3D11DynamicRHI(IDXGIFactory1* InDXGIFactory1, D3D_FEATURE_LEVEL InFeatureLevel, int32 InChosenAdapter, const DXGI_ADAPTER_DESC& InChosenDescription)
	:DXGIFactory1(InDXGIFactory1),
	FeatureLevel(InFeatureLevel),
	ChosenAdapter(InChosenAdapter),
	DXGIAdpaterDesc(InChosenDescription)
{
	// This should be called once at the start 
	check(ChosenAdapter >= 0);
	check(IsInGameThread());
	check(!GIsThreadedRendering);

	// Initialize the RHI capabilities.
	check(FeatureLevel == D3D_FEATURE_LEVEL_11_1 || FeatureLevel == D3D_FEATURE_LEVEL_11_0 || FeatureLevel == D3D_FEATURE_LEVEL_10_0);

	if (FeatureLevel == D3D_FEATURE_LEVEL_10_0)
	{
		GSupportsDepthFetchDuringDepthTest = false;
	}
	if (FeatureLevel == D3D_FEATURE_LEVEL_11_0 || FeatureLevel == D3D_FEATURE_LEVEL_11_1)
	{
		GMaxRHIShaderPlatform = SP_PCD3D_SM5;
	}
	else if (FeatureLevel == D3D_FEATURE_LEVEL_10_0)
	{
		GMaxRHIShaderPlatform = SP_PCD3D_SM4;
	}

	// Initialize the platform pixel format map.
	GPixelFormats[PF_Unknown].PlatformFormat = DXGI_FORMAT_UNKNOWN;
	GPixelFormats[PF_A32B32G32R32F].PlatformFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	GPixelFormats[PF_B8G8R8A8].PlatformFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
	GPixelFormats[PF_G8].PlatformFormat = DXGI_FORMAT_R8_UNORM;
	GPixelFormats[PF_G16].PlatformFormat = DXGI_FORMAT_R16_UNORM;
	GPixelFormats[PF_DXT1].PlatformFormat = DXGI_FORMAT_BC1_TYPELESS;
	GPixelFormats[PF_DXT3].PlatformFormat = DXGI_FORMAT_BC2_TYPELESS;
	GPixelFormats[PF_DXT5].PlatformFormat = DXGI_FORMAT_BC3_TYPELESS;
	GPixelFormats[PF_BC4].PlatformFormat = DXGI_FORMAT_BC4_UNORM;
	GPixelFormats[PF_UYVY].PlatformFormat = DXGI_FORMAT_UNKNOWN;		// TODO: Not supported in D3D11
//#if DEPTH_32_BIT_CONVERSION
#if 0
	GPixelFormats[PF_DepthStencil].PlatformFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
	GPixelFormats[PF_DepthStencil].BlockBytes = 5;
	GPixelFormats[PF_X24_G8].PlatformFormat = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
	GPixelFormats[PF_X24_G8].BlockBytes = 5;
#else
	GPixelFormats[PF_DepthStencil].PlatformFormat = DXGI_FORMAT_R24G8_TYPELESS;
	GPixelFormats[PF_DepthStencil].BlockBytes = 4;
	GPixelFormats[PF_X24_G8].PlatformFormat = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
	GPixelFormats[PF_X24_G8].BlockBytes = 4;
#endif
	GPixelFormats[PF_ShadowDepth].PlatformFormat = DXGI_FORMAT_R16_TYPELESS;
	GPixelFormats[PF_ShadowDepth].BlockBytes = 2;
	GPixelFormats[PF_R32_FLOAT].PlatformFormat = DXGI_FORMAT_R32_FLOAT;
	GPixelFormats[PF_G16R16].PlatformFormat = DXGI_FORMAT_R16G16_UNORM;
	GPixelFormats[PF_G16R16F].PlatformFormat = DXGI_FORMAT_R16G16_FLOAT;
	GPixelFormats[PF_G16R16F_FILTER].PlatformFormat = DXGI_FORMAT_R16G16_FLOAT;
	GPixelFormats[PF_G32R32F].PlatformFormat = DXGI_FORMAT_R32G32_FLOAT;
	GPixelFormats[PF_A2B10G10R10].PlatformFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	GPixelFormats[PF_A16B16G16R16].PlatformFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
	GPixelFormats[PF_D24].PlatformFormat = DXGI_FORMAT_R24G8_TYPELESS;
	GPixelFormats[PF_R16F].PlatformFormat = DXGI_FORMAT_R16_FLOAT;
	GPixelFormats[PF_R16F_FILTER].PlatformFormat = DXGI_FORMAT_R16_FLOAT;

	GPixelFormats[PF_FloatRGB].PlatformFormat = DXGI_FORMAT_R11G11B10_FLOAT;
	GPixelFormats[PF_FloatRGB].BlockBytes = 4;
	GPixelFormats[PF_FloatRGBA].PlatformFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	GPixelFormats[PF_FloatRGBA].BlockBytes = 8;

	GPixelFormats[PF_FloatR11G11B10].PlatformFormat = DXGI_FORMAT_R11G11B10_FLOAT;
	GPixelFormats[PF_FloatR11G11B10].BlockBytes = 4;
	GPixelFormats[PF_FloatR11G11B10].Supported = true;

	GPixelFormats[PF_V8U8].PlatformFormat = DXGI_FORMAT_R8G8_SNORM;
	GPixelFormats[PF_BC5].PlatformFormat = DXGI_FORMAT_BC5_UNORM;
	GPixelFormats[PF_A1].PlatformFormat = DXGI_FORMAT_R1_UNORM; // Not supported for rendering.
	GPixelFormats[PF_A8].PlatformFormat = DXGI_FORMAT_A8_UNORM;
	GPixelFormats[PF_R32_UINT].PlatformFormat = DXGI_FORMAT_R32_UINT;
	GPixelFormats[PF_R32_SINT].PlatformFormat = DXGI_FORMAT_R32_SINT;

	GPixelFormats[PF_R16_UINT].PlatformFormat = DXGI_FORMAT_R16_UINT;
	GPixelFormats[PF_R16_SINT].PlatformFormat = DXGI_FORMAT_R16_SINT;
	GPixelFormats[PF_R16G16B16A16_UINT].PlatformFormat = DXGI_FORMAT_R16G16B16A16_UINT;
	GPixelFormats[PF_R16G16B16A16_SINT].PlatformFormat = DXGI_FORMAT_R16G16B16A16_SINT;

	GPixelFormats[PF_R5G6B5_UNORM].PlatformFormat = DXGI_FORMAT_B5G6R5_UNORM;
	GPixelFormats[PF_R8G8B8A8].PlatformFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	GPixelFormats[PF_R8G8B8A8_UINT].PlatformFormat = DXGI_FORMAT_R8G8B8A8_UINT;
	GPixelFormats[PF_R8G8B8A8_SNORM].PlatformFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
	GPixelFormats[PF_R8G8].PlatformFormat = DXGI_FORMAT_R8G8_UNORM;
	GPixelFormats[PF_R32G32B32A32_UINT].PlatformFormat = DXGI_FORMAT_R32G32B32A32_UINT;
	GPixelFormats[PF_R16G16_UINT].PlatformFormat = DXGI_FORMAT_R16G16_UINT;

	GPixelFormats[PF_BC6H].PlatformFormat = DXGI_FORMAT_BC6H_UF16;
	GPixelFormats[PF_BC7].PlatformFormat = DXGI_FORMAT_BC7_TYPELESS;
	GPixelFormats[PF_R8_UINT].PlatformFormat = DXGI_FORMAT_R8_UINT;

	GPixelFormats[PF_R16G16B16A16_UNORM].PlatformFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
	GPixelFormats[PF_R16G16B16A16_SNORM].PlatformFormat = DXGI_FORMAT_R16G16B16A16_SNORM;

	if (FeatureLevel >= D3D_FEATURE_LEVEL_11_0)
	{
		GSupportsSeparateRenderTargetBlendState = true;
		GMaxTextureDimensions = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
		GMaxCubeTextureDimensions = D3D11_REQ_TEXTURECUBE_DIMENSION;
		GMaxTextureArrayLayers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
		GRHISupportsMSAADepthSampleAccess = true;
	}
	else if (FeatureLevel >= D3D_FEATURE_LEVEL_10_0)
	{
		GMaxTextureDimensions = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
		GMaxCubeTextureDimensions = D3D10_REQ_TEXTURECUBE_DIMENSION;
		GMaxTextureArrayLayers = D3D10_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
	}

	GMaxTextureMipCount = YMath::CeilLogTwo(GMaxTextureDimensions) + 1;
	GMaxTextureMipCount = YMath::Min<int32>(MAX_TEXTURE_MIP_COUNT, GMaxTextureMipCount);
	GMaxShadowDepthBufferSizeX = GMaxTextureDimensions;
	GMaxShadowDepthBufferSizeY = GMaxTextureDimensions;
	GSupportsTimestampRenderQueries = true;
	GRHISupportsResolveCubemapFaces = true;
}

YD3D11DynamicRHI::~YD3D11DynamicRHI()
{
	LOG_INFO("~YD3D11DynamicRHI");
}

void YD3D11DynamicRHI::Init()
{
	InitD3DDevice();
	GSupportsDepthBoundsTest = (IsRHIDeviceNVIDIA() || IsRHIDeviceAMD());
}

void YD3D11DynamicRHI::InitD3DDevice()
{
	assert(IsInGameThread());
	// Wait for the rendering thread to go idle.
	SCOPED_SUSPEND_RENDERING_THREAD(false);
	if (!d3d_device_)
	{
		LOG_INFO("Create a new d3d deviec");
		assert(!GIsRHIInitialized);

		// tod do  Clear shadowed shader resources.
		ClearState();

		// Determine the adapter and device type to use.
		TRefCountPtr<IDXGIAdapter> Adapter;

		// In Direct3D 11, if you are trying to create a hardware or a software device, set pAdapter != NULL which constrains the other inputs to be:
		//		DriverType must be D3D_DRIVER_TYPE_UNKNOWN 
		//		Software must be NULL. 
		D3D_DRIVER_TYPE DriverType = D3D_DRIVER_TYPE_UNKNOWN;

		//uint32 DeviceFlags = D3D11RHI_ShouldAllowAsyncResourceCreation() ? 0 : D3D11_CREATE_DEVICE_SINGLETHREADED;
		uint32 DeviceFlags = 0;
#if defined DEBUG || defined _DEBUG
		DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		LOG_INFO("InitD3dDevice with debug model");
#endif
		GTexturePoolSize = 0;

		TRefCountPtr<IDXGIAdapter> EnumAdapter;

		if (DXGIFactory1->EnumAdapters(ChosenAdapter, EnumAdapter.GetInitReference()) != DXGI_ERROR_NOT_FOUND)
		{
			if (EnumAdapter)// && EnumAdapter->CheckInterfaceSupport(__uuidof(ID3D11Device),NULL) == S_OK)
			{
				// we don't use AdapterDesc.Description as there is a bug with Optimus where it can report the wrong name
				DXGI_ADAPTER_DESC AdapterDesc = DXGIAdpaterDesc;
				Adapter = EnumAdapter;

				GRHIAdapterName = (char*)AdapterDesc.Description;
				GRHIVendorId = AdapterDesc.VendorId;
				GRHIDeviceId = AdapterDesc.DeviceId;
				GRHIDeviceRevision = AdapterDesc.Revision;
				FD3D11GlobalStats::GDedicatedVideoMemory = int64(AdapterDesc.DedicatedVideoMemory);
				FD3D11GlobalStats::GDedicatedSystemMemory = int64(AdapterDesc.DedicatedSystemMemory);
				FD3D11GlobalStats::GSharedSystemMemory = int64(AdapterDesc.SharedSystemMemory);


				FD3D11GlobalStats::GTotalGraphicsMemory = 0;
				int64 ConsideredSharedSystemMemory = 0;
				if (IsRHIDeviceIntel())
				{
					// It's all system memory.
					FD3D11GlobalStats::GTotalGraphicsMemory = FD3D11GlobalStats::GDedicatedVideoMemory;
					FD3D11GlobalStats::GTotalGraphicsMemory += FD3D11GlobalStats::GDedicatedSystemMemory;
					FD3D11GlobalStats::GTotalGraphicsMemory += ConsideredSharedSystemMemory;
				}
				else if (FD3D11GlobalStats::GDedicatedVideoMemory >= 200 * 1024 * 1024)
				{
					// Use dedicated video memory, if it's more than 200 MB
					FD3D11GlobalStats::GTotalGraphicsMemory = FD3D11GlobalStats::GDedicatedVideoMemory;
				}
				else if (FD3D11GlobalStats::GDedicatedSystemMemory >= 200 * 1024 * 1024)
				{
					// Use dedicated system memory, if it's more than 200 MB
					FD3D11GlobalStats::GTotalGraphicsMemory = FD3D11GlobalStats::GDedicatedSystemMemory;
				}
				else if (FD3D11GlobalStats::GSharedSystemMemory >= 400 * 1024 * 1024)
				{
					// Use some shared system memory, if it's more than 400 MB
					FD3D11GlobalStats::GTotalGraphicsMemory = ConsideredSharedSystemMemory;
				}
				else
				{
					// Otherwise consider 25% of total system memory for graphics.
					//FD3D11GlobalStats::GTotalGraphicsMemory = TotalPhysicalMemory / 4ll;
				}

				if (sizeof(SIZE_T) < 8)
				{
					// Clamp to 1 GB if we're less than 64-bit
					FD3D11GlobalStats::GTotalGraphicsMemory = YMath::Min(FD3D11GlobalStats::GTotalGraphicsMemory, 1024ll * 1024ll * 1024ll);
				}

				if (GPoolSizeVRAMPercentage > 0)
				{
					float PoolSize = float(GPoolSizeVRAMPercentage) * 0.01f * float(FD3D11GlobalStats::GTotalGraphicsMemory);

					// Truncate GTexturePoolSize to MB (but still counted in bytes)
					GTexturePoolSize = int64(YMath::TruncToFloat(PoolSize / 1024.0f / 1024.0f)) * 1024 * 1024;

					LOG_INFO(StringFormat("Texture pool is %llu MB (%d%% of %llu MB)",
						GTexturePoolSize / 1024 / 1024,
						GPoolSizeVRAMPercentage,
						FD3D11GlobalStats::GTotalGraphicsMemory / 1024 / 1024));
				}

			}
		}
		else
		{
			assert(0);
		}

		D3D_FEATURE_LEVEL ActualFeatureLevel = (D3D_FEATURE_LEVEL)0;
		// Creating the Direct3D device.
		VERIFYD3D11RESULT(D3D11CreateDevice(
			Adapter,
			DriverType,
			NULL,
			DeviceFlags,
			&FeatureLevel,
			1,
			D3D11_SDK_VERSION,
			d3d_device_.GetInitReference(),
			&ActualFeatureLevel,
			d3d_dc_.GetInitReference()
		));

		// We should get the feature level we asked for as earlier we checked to ensure it is supported.
		check(ActualFeatureLevel == FeatureLevel);
		StateCache.Init(d3d_dc_);
		// Check for async texture creation support.
		D3D11_FEATURE_DATA_THREADING ThreadingSupport = { 0 };
		VERIFYD3D11RESULT_EX(d3d_device_->CheckFeatureSupport(D3D11_FEATURE_THREADING, &ThreadingSupport, sizeof(ThreadingSupport)), d3d_device_);
		GRHISupportsAsyncTextureCreation = !!ThreadingSupport.DriverConcurrentCreates
			&& (DeviceFlags & D3D11_CREATE_DEVICE_SINGLETHREADED) == 0;

		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::ES2] = SP_PCD3D_ES2;
		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::ES3_1] = SP_PCD3D_ES3_1;
		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM4] = SP_PCD3D_SM4;
		GShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM5] = SP_PCD3D_SM5;

		if (IsRHIDeviceAMD() /*&& CVarAMDDisableAsyncTextureCreation.GetValueOnAnyThread()*/)
		{
			GRHISupportsAsyncTextureCreation = false;
		}


		if (IsRHIDeviceNVIDIA())
		{
			GSupportsDepthBoundsTest = true;
		}

		SetupAfterDeviceCreation();

		// Notify all initialized FRenderResources that there's a valid RHI device to create their RHI resources for now.
		for (TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList()); ResourceIt; ResourceIt.Next())
		{
		//	ResourceIt->InitRHI();
		}
		// Dynamic resources can have dependencies on static resources (with uniform buffers) and must initialized last!
		for (TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList()); ResourceIt; ResourceIt.Next())
		{
			ResourceIt->InitDynamicRHI();
		}
		if (DeviceFlags & D3D11_CREATE_DEVICE_DEBUG)
		{
			TRefCountPtr<ID3D11InfoQueue> InfoQueue;
			VERIFYD3D11RESULT_EX(d3d_device_->QueryInterface(IID_ID3D11InfoQueue, (void**)InfoQueue.GetInitReference()), d3d_device_);
			if (InfoQueue)
			{
				D3D11_INFO_QUEUE_FILTER NewFilter;
				memset(&NewFilter, 0, sizeof(NewFilter));

				// Turn off info msgs as these get really spewy
				D3D11_MESSAGE_SEVERITY DenySeverity = D3D11_MESSAGE_SEVERITY_INFO;
				NewFilter.DenyList.NumSeverities = 1;
				NewFilter.DenyList.pSeverityList = &DenySeverity;

				// Be sure to carefully comment the reason for any additions here!  Someone should be able to look at it later and get an idea of whether it is still necessary.
				D3D11_MESSAGE_ID DenyIds[] = {
					// OMSETRENDERTARGETS_INVALIDVIEW - d3d will complain if depth and color targets don't have the exact same dimensions, but actually
					//	if the color target is smaller then things are ok.  So turn off this error.  There is a manual check in FD3D11DynamicRHI::SetRenderTarget
					//	that tests for depth smaller than color and MSAA settings to match.
					D3D11_MESSAGE_ID_OMSETRENDERTARGETS_INVALIDVIEW,

					// QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS - The RHI exposes the interface to make and issue queries and a separate interface to use that data.
					//		Currently there is a situation where queries are issued and the results may be ignored on purpose.  Filtering out this message so it doesn't
					//		swarm the debug spew and mask other important warnings
					D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS,
					D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS,

					// D3D11_MESSAGE_ID_CREATEINPUTLAYOUT_EMPTY_LAYOUT - This is a warning that gets triggered if you use a null vertex declaration,
					//       which we want to do when the vertex shader is generating vertices based on ID.
					D3D11_MESSAGE_ID_CREATEINPUTLAYOUT_EMPTY_LAYOUT,

					// D3D11_MESSAGE_ID_DEVICE_DRAW_INDEX_BUFFER_TOO_SMALL - This warning gets triggered by Slate draws which are actually using a valid index range.
					//		The invalid warning seems to only happen when VS 2012 is installed.  Reported to MS.  
					//		There is now an assert in DrawIndexedPrimitive to catch any valid errors reading from the index buffer outside of range.
					D3D11_MESSAGE_ID_DEVICE_DRAW_INDEX_BUFFER_TOO_SMALL,

					// D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET - This warning gets triggered by shadow depth rendering because the shader outputs
					//		a color but we don't bind a color render target. That is safe as writes to unbound render targets are discarded.
					//		Also, batched elements triggers it when rendering outside of scene rendering as it outputs to the GBuffer containing normals which is not bound.
					(D3D11_MESSAGE_ID)3146081, // D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,

					// Spams constantly as we change the debug name on rendertargets that get reused.
					D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
				};

				NewFilter.DenyList.NumIDs = sizeof(DenyIds) / sizeof(D3D11_MESSAGE_ID);
				NewFilter.DenyList.pIDList = (D3D11_MESSAGE_ID*)&DenyIds;

				InfoQueue->PushStorageFilter(&NewFilter);

				// Break on D3D debug errors.
				InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);

				// Enable this to break on a specific id in order to quickly get a callstack
				InfoQueue->SetBreakOnID(D3D11_MESSAGE_ID_DEVICE_DRAW_CONSTANT_BUFFER_TOO_SMALL, true);

				//if (FParse::Param(FCommandLine::Get(), TEXT("d3dbreakonwarning")))
				//{
				InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
				//}
			}
		}
		GRHISupportsHDROutput = false;
		GRHISupportsTextureStreaming = true;
		GRHISupportsFirstInstance = true;
		GRHINeedsExtraDeletionLatency = true;
		// Set the RHI initialized flag.
		GIsRHIInitialized = true;
	}
}


void YD3D11DynamicRHI::PostInit()
{
}
void YD3D11DynamicRHI::CleanupD3DDevice()
{
	LOG_INFO("Clean up d3d device");
	if (GIsRHIInitialized)
	{
		assert(d3d_device_);
		assert(d3d_dc_);
		GIsRHIInitialized = false;
		assert(!GIsCriticalError);
	}

	// Ask all initialized FRenderResources to release their RHI resources.
	for (TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList()); ResourceIt; ResourceIt.Next())
	{
		FRenderResource* Resource = *ResourceIt;
		check(Resource->IsInitialized());
		Resource->ReleaseRHI();
	}

	for (TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList()); ResourceIt; ResourceIt.Next())
	{
		ResourceIt->ReleaseDynamicRHI();
	}



#if defined DEBUG || _DEBUG
	{
		d3d_dc_->ClearState();
		d3d_dc_->Flush();

		// Perform a detailed live object report (with resource types)
		ID3D11Debug* D3D11Debug;
		d3d_device_->QueryInterface(__uuidof(ID3D11Debug), (void**)(&D3D11Debug));
		if (D3D11Debug)
		{
			D3D11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		}
	}
#endif

}


void YD3D11DynamicRHI::ClearState()
{
	StateCache.ClearState();
}

void YD3D11DynamicRHI::SetupAfterDeviceCreation()
{
	// without that the first RHIClear would get a scissor rect of (0,0)-(0,0) which means we get a draw call clear 
	RHISetScissorRect(false, 0, 0, 0, 0);
	UpdateMSAASettings();

	if (GRHISupportsAsyncTextureCreation)
	{
		//UE_LOG(LogD3D11RHI, Log, TEXT("Async texture creation enabled"));
		LOG_INFO("Async texture creation enabled");
	}
	else
	{
		//UE_LOG(LogD3D11RHI, Log, TEXT("Async texture creation disabled: %s"),
			//D3D11RHI_ShouldAllowAsyncResourceCreation() ? TEXT("no driver support") : TEXT("disabled by user"));
		LOG_INFO("Async texture creation disabled");
	}

#if _WIN32 
	IUnknown* RenderDoc;
	IID RenderDocID;
	if (SUCCEEDED(IIDFromString(L"{A7AA6116-9C8D-4BBA-9083-B4D816B71B78}", &RenderDocID)))
	{
		if (SUCCEEDED(d3d_device_->QueryInterface(RenderDocID, (void**)(&RenderDoc))))
		{
			// Running under RenderDoc, so enable capturing mode
			GDynamicRHI->EnableIdealGPUCaptureOptions(true);
		}
	}
#endif
}

void YD3D11DynamicRHI::UpdateMSAASettings()
{
	check(DX_MAX_MSAA_COUNT == 8);

	// quality levels are only needed for CSAA which we cannot use with custom resolves

	// 0xffffffff means not available
	AvailableMSAAQualities[0] = 0xffffffff;
	AvailableMSAAQualities[1] = 0xffffffff;
	AvailableMSAAQualities[2] = 0;
	AvailableMSAAQualities[3] = 0xffffffff;
	AvailableMSAAQualities[4] = 0;
	AvailableMSAAQualities[5] = 0xffffffff;
	AvailableMSAAQualities[6] = 0xffffffff;
	AvailableMSAAQualities[7] = 0xffffffff;
	AvailableMSAAQualities[8] = 0;
}

uint32 YD3D11DynamicRHI::GetMaxMSAAQuality(uint32 SampleCount)
{
	if (SampleCount <= DX_MAX_MSAA_COUNT)
	{
		// 0 has better quality (a more even distribution)
		// higher quality levels might be useful for non box filtered AA or when using weighted samples 
		return 0;
		//return AvailableMSAAQualities[SampleCount];
	}
	// not supported
	return 0xffffffff;
}

void YD3D11DynamicRHI::Shutdown()
{
	//throw std::logic_error("The method or operation is not implemented.");
	LOG_INFO("YD3D11DynamicRHI::ShutDown");
	assert(IsInGameThread() && IsInRenderingThread());
	// Cleanup the D3D device.
	CleanupD3DDevice();
}

const TCHAR* YD3D11DynamicRHI::GetName()
{
	throw std::logic_error("The method or operation is not implemented.");
}

FSamplerStateRHIRef YD3D11DynamicRHI::RHICreateSamplerState(const FSamplerStateInitializerRHI& Initializer)
{
	//assert(d3d_device_);
	//HRESULT hr = S_OK;
	//D3D11_SAMPLER_DESC samDesc;
	//ZeroMemory(&samDesc, sizeof(samDesc));
	//switch(Initializer.Filter)
	//{
	//case ESamplerFilter::SF_Point:
	//	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	//}
	//samDesc.Filter = 
	//samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	//samDesc.MaxAnisotropy = 1;
	//samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	//samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	//if (FAILED(hr = d3d_device_->CreateSamplerState(&samDesc, &sample))) {
	//	return false;
	//}
	assert(0);
	FSamplerStateRHIRef tmp;
	return tmp;
}

FRasterizerStateRHIRef YD3D11DynamicRHI::RHICreateRasterizerState(const FRasterizerStateInitializerRHI& Initializer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FDepthStencilStateRHIRef YD3D11DynamicRHI::RHICreateDepthStencilState(const FDepthStencilStateInitializerRHI& Initializer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FBlendStateRHIRef YD3D11DynamicRHI::RHICreateBlendState(const FBlendStateInitializerRHI& Initializer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FVertexDeclarationRHIRef YD3D11DynamicRHI::RHICreateVertexDeclaration(const FVertexDeclarationElementList& Elements)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FPixelShaderRHIRef YD3D11DynamicRHI::RHICreatePixelShader(const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FPixelShaderRHIRef YD3D11DynamicRHI::RHICreatePixelShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FVertexShaderRHIRef YD3D11DynamicRHI::RHICreateVertexShader(const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FVertexShaderRHIRef YD3D11DynamicRHI::RHICreateVertexShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FHullShaderRHIRef YD3D11DynamicRHI::RHICreateHullShader(const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FHullShaderRHIRef YD3D11DynamicRHI::RHICreateHullShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FDomainShaderRHIRef YD3D11DynamicRHI::RHICreateDomainShader(const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FDomainShaderRHIRef YD3D11DynamicRHI::RHICreateDomainShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGeometryShaderRHIRef YD3D11DynamicRHI::RHICreateGeometryShader(const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGeometryShaderRHIRef YD3D11DynamicRHI::RHICreateGeometryShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGeometryShaderRHIRef YD3D11DynamicRHI::RHICreateGeometryShaderWithStreamOutput(const std::vector<uint8>& Code, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGeometryShaderRHIRef YD3D11DynamicRHI::RHICreateGeometryShaderWithStreamOutput(const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::FlushPendingLogs()
{
	throw std::logic_error("The method or operation is not implemented.");
}

FComputeShaderRHIRef YD3D11DynamicRHI::RHICreateComputeShader(const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FComputeShaderRHIRef YD3D11DynamicRHI::RHICreateComputeShader(FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FRHIShaderLibraryRef YD3D11DynamicRHI::RHICreateShaderLibrary(EShaderPlatform Platform, std::string const& FilePath, std::string const& Name)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FComputeFenceRHIRef YD3D11DynamicRHI::RHICreateComputeFence(const std::string& Name)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGPUFenceRHIRef YD3D11DynamicRHI::RHICreateGPUFence(const std::string& Name)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FStagingBufferRHIRef YD3D11DynamicRHI::RHICreateStagingBuffer()
{
	throw std::logic_error("The method or operation is not implemented.");
}

FBoundShaderStateRHIRef YD3D11DynamicRHI::RHICreateBoundShaderState(FVertexDeclarationRHIParamRef VertexDeclaration, FVertexShaderRHIParamRef VertexShader, FHullShaderRHIParamRef HullShader, FDomainShaderRHIParamRef DomainShader, FPixelShaderRHIParamRef PixelShader, FGeometryShaderRHIParamRef GeometryShader)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGraphicsPipelineStateRHIRef YD3D11DynamicRHI::RHICreateGraphicsPipelineState(const FGraphicsPipelineStateInitializer& Initializer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGraphicsPipelineStateRHIRef YD3D11DynamicRHI::RHICreateGraphicsPipelineState(const FGraphicsPipelineStateInitializer& Initializer, FRHIPipelineBinaryLibraryParamRef PipelineBinary)
{
	throw std::logic_error("The method or operation is not implemented.");
}

TRefCountPtr<FRHIComputePipelineState> YD3D11DynamicRHI::RHICreateComputePipelineState(FRHIComputeShader* ComputeShader)
{
	throw std::logic_error("The method or operation is not implemented.");
}

TRefCountPtr<FRHIComputePipelineState> YD3D11DynamicRHI::RHICreateComputePipelineState(FRHIComputeShader* ComputeShader, FRHIPipelineBinaryLibraryParamRef PipelineBinary)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUniformBufferRHIRef YD3D11DynamicRHI::RHICreateUniformBuffer(const void* Contents, const FRHIUniformBufferLayout& Layout, EUniformBufferUsage Usage)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FIndexBufferRHIRef YD3D11DynamicRHI::RHICreateIndexBuffer(uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::RHILockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer, uint32 Offset, uint32 Size, EResourceLockMode LockMode)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUnlockIndexBuffer(FIndexBufferRHIParamRef IndexBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FVertexBufferRHIRef YD3D11DynamicRHI::RHICreateVertexBuffer(uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::RHILockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUnlockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHICopyVertexBuffer(FVertexBufferRHIParamRef SourceBuffer, FVertexBufferRHIParamRef DestBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FStructuredBufferRHIRef YD3D11DynamicRHI::RHICreateStructuredBuffer(uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::RHILockStructuredBuffer(FStructuredBufferRHIParamRef StructuredBuffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUnlockStructuredBuffer(FStructuredBufferRHIParamRef StructuredBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUnorderedAccessViewRHIRef YD3D11DynamicRHI::RHICreateUnorderedAccessView(FStructuredBufferRHIParamRef StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUnorderedAccessViewRHIRef YD3D11DynamicRHI::RHICreateUnorderedAccessView(FTextureRHIParamRef Texture, uint32 MipLevel)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUnorderedAccessViewRHIRef YD3D11DynamicRHI::RHICreateUnorderedAccessView(FVertexBufferRHIParamRef VertexBuffer, uint8 Format)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUnorderedAccessViewRHIRef YD3D11DynamicRHI::RHICreateUnorderedAccessView(FIndexBufferRHIParamRef IndexBuffer, uint8 Format)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView(FStructuredBufferRHIParamRef StructuredBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView(FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint8 Format)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView(FIndexBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView(FTexture2DRHIParamRef Texture2DRHI, uint8 MipLevel)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView(FTexture2DRHIParamRef Texture2DRHI, uint8 MipLevel, uint8 NumMipLevels, uint8 Format)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView(FTexture3DRHIParamRef Texture3DRHI, uint8 MipLevel)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView(FTexture2DArrayRHIParamRef Texture2DArrayRHI, uint8 MipLevel)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView(FTextureCubeRHIParamRef TextureCubeRHI, uint8 MipLevel)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIGenerateMips(FTextureRHIParamRef Texture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

uint32 YD3D11DynamicRHI::RHIComputeMemorySize(FTextureRHIParamRef TextureRHI)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DRHIRef YD3D11DynamicRHI::RHIAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, int32 NewMipCount, int32 NewSizeX, int32 NewSizeY, FThreadSafeCounter* RequestStatus)
{
	throw std::logic_error("The method or operation is not implemented.");
}

ETextureReallocationStatus YD3D11DynamicRHI::RHIFinalizeAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted)
{
	throw std::logic_error("The method or operation is not implemented.");
}

ETextureReallocationStatus YD3D11DynamicRHI::RHICancelAsyncReallocateTexture2D(FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::RHILockTexture2D(FTexture2DRHIParamRef Texture, uint32 MipIndex, EResourceLockMode LockMode, uint32& DestStride, bool bLockWithinMiptail)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUnlockTexture2D(FTexture2DRHIParamRef Texture, uint32 MipIndex, bool bLockWithinMiptail)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::RHILockTexture2DArray(FTexture2DArrayRHIParamRef Texture, uint32 TextureIndex, uint32 MipIndex, EResourceLockMode LockMode, uint32& DestStride, bool bLockWithinMiptail)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUnlockTexture2DArray(FTexture2DArrayRHIParamRef Texture, uint32 TextureIndex, uint32 MipIndex, bool bLockWithinMiptail)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUpdateTexture2D(FTexture2DRHIParamRef Texture, uint32 MipIndex, const struct FUpdateTextureRegion2D& UpdateRegion, uint32 SourcePitch, const uint8* SourceData)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUpdateTexture3D(FTexture3DRHIParamRef Texture, uint32 MipIndex, const struct FUpdateTextureRegion3D& UpdateRegion, uint32 SourceRowPitch, uint32 SourceDepthPitch, const uint8* SourceData)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTextureCubeRHIRef YD3D11DynamicRHI::RHICreateTextureCube(uint32 Size, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTextureCubeRHIRef YD3D11DynamicRHI::RHICreateTextureCubeArray(uint32 Size, uint32 ArraySize, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::RHILockTextureCubeFace(FTextureCubeRHIParamRef Texture, uint32 FaceIndex, uint32 ArrayIndex, uint32 MipIndex, EResourceLockMode LockMode, uint32& DestStride, bool bLockWithinMiptail)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUnlockTextureCubeFace(FTextureCubeRHIParamRef Texture, uint32 FaceIndex, uint32 ArrayIndex, uint32 MipIndex, bool bLockWithinMiptail)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBindDebugLabelName(FTextureRHIParamRef Texture, const TCHAR* Name)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBindDebugLabelName(FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI, const TCHAR* Name)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIReadSurfaceData(FTextureRHIParamRef Texture, FIntRect Rect, std::vector<FColor>& OutData, FReadSurfaceDataFlags InFlags)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIReadSurfaceData(FTextureRHIParamRef Texture, FIntRect Rect, std::vector<FLinearColor>& OutData, FReadSurfaceDataFlags InFlags)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIMapStagingSurface(FTextureRHIParamRef Texture, void*& OutData, int32& OutWidth, int32& OutHeight)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUnmapStagingSurface(FTextureRHIParamRef Texture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIReadSurfaceFloatData(FTextureRHIParamRef Texture, FIntRect Rect, std::vector<FFloat16Color>& OutData, ECubeFace CubeFace, int32 ArrayIndex, int32 MipIndex)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIRead3DSurfaceFloatData(FTextureRHIParamRef Texture, FIntRect Rect, FIntPoint ZMinMax, std::vector<FFloat16Color>& OutData)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FRenderQueryRHIRef YD3D11DynamicRHI::RHICreateRenderQuery(ERenderQueryType QueryType)
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool YD3D11DynamicRHI::RHIGetRenderQueryResult(FRenderQueryRHIParamRef RenderQuery, uint64& OutResult, bool bWait)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DRHIRef YD3D11DynamicRHI::RHIGetViewportBackBuffer(FViewportRHIParamRef Viewport)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUnorderedAccessViewRHIRef YD3D11DynamicRHI::RHIGetViewportBackBufferUAV(FViewportRHIParamRef ViewportRHI)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DRHIRef YD3D11DynamicRHI::RHIGetFMaskTexture(FTextureRHIParamRef SourceTextureRHI)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIAdvanceFrameForGetViewportBackBuffer(FViewportRHIParamRef Viewport)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIAcquireThreadOwnership()
{
}

void YD3D11DynamicRHI::RHIReleaseThreadOwnership()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIFlushResources()
{
}

uint32 YD3D11DynamicRHI::RHIGetGPUFrameCycles()
{
	throw std::logic_error("The method or operation is not implemented.");
}

FViewportRHIRef YD3D11DynamicRHI::RHICreateViewport(void* WindowHandle, uint32 SizeX, uint32 SizeY, bool bIsFullscreen, EPixelFormat PreferredPixelFormat)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIResizeViewport(FViewportRHIParamRef Viewport, uint32 SizeX, uint32 SizeY, bool bIsFullscreen)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIResizeViewport(FViewportRHIParamRef Viewport, uint32 SizeX, uint32 SizeY, bool bIsFullscreen, EPixelFormat PreferredPixelFormat)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHITick(float DeltaTime)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetStreamOutTargets(uint32 NumTargets, const FVertexBufferRHIParamRef* VertexBuffers, const uint32* Offsets)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBlockUntilGPUIdle()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISubmitCommandsAndFlushGPU()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginSuspendRendering()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISuspendRendering()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIResumeRendering()
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool YD3D11DynamicRHI::RHIIsRenderingSuspended()
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool YD3D11DynamicRHI::RHIEnqueueDecompress(uint8_t* SrcBuffer, uint8_t* DestBuffer, int CompressedSize, void* ErrorCodeBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool YD3D11DynamicRHI::RHIEnqueueCompress(uint8_t* SrcBuffer, uint8_t* DestBuffer, int UnCompressedSize, void* ErrorCodeBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIRecreateRecursiveBoundShaderStates()
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool YD3D11DynamicRHI::RHIGetAvailableResolutions(FScreenResolutionArray& Resolutions, bool bIgnoreRefreshRate)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIGetSupportedResolution(uint32& Width, uint32& Height)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIVirtualTextureSetFirstMipInMemory(FTexture2DRHIParamRef Texture, uint32 FirstMip)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIVirtualTextureSetFirstMipVisible(FTexture2DRHIParamRef Texture, uint32 FirstMip)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIPerFrameRHIFlushComplete()
{
}

void YD3D11DynamicRHI::RHIExecuteCommandList(FRHICommandList* CmdList)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::RHIGetNativeDevice()
{
	throw std::logic_error("The method or operation is not implemented.");
}

IRHICommandContext* YD3D11DynamicRHI::RHIGetDefaultContext()
{
	return this;
}

class IRHICommandContextContainer* YD3D11DynamicRHI::RHIGetCommandContextContainer(int32 Index, int32 Num)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FVertexBufferRHIRef YD3D11DynamicRHI::CreateAndLockVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo, void*& OutDataBuffer)
{
	//throw std::logic_error("The method or operation is not implemented.");
	return nullptr;
}

FIndexBufferRHIRef YD3D11DynamicRHI::CreateAndLockIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo, void*& OutDataBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FVertexBufferRHIRef YD3D11DynamicRHI::CreateVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FStructuredBufferRHIRef YD3D11DynamicRHI::CreateStructuredBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::CreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint8 Format)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::CreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::LockVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::UnlockVertexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DRHIRef YD3D11DynamicRHI::AsyncReallocateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2D, int32 NewMipCount, int32 NewSizeX, int32 NewSizeY, FThreadSafeCounter* RequestStatus)
{
	throw std::logic_error("The method or operation is not implemented.");
}

ETextureReallocationStatus YD3D11DynamicRHI::FinalizeAsyncReallocateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted)
{
	throw std::logic_error("The method or operation is not implemented.");
}

ETextureReallocationStatus YD3D11DynamicRHI::CancelAsyncReallocateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FIndexBufferRHIRef YD3D11DynamicRHI::CreateIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Stride, uint32 Size, uint32 InUsage, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::LockIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef IndexBuffer, uint32 Offset, uint32 SizeRHI, EResourceLockMode LockMode)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::UnlockIndexBuffer_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef IndexBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FVertexDeclarationRHIRef YD3D11DynamicRHI::CreateVertexDeclaration_RenderThread(class FRHICommandListImmediate& RHICmdList, const FVertexDeclarationElementList& Elements)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FVertexShaderRHIRef YD3D11DynamicRHI::CreateVertexShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FVertexShaderRHIRef YD3D11DynamicRHI::CreateVertexShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FPixelShaderRHIRef YD3D11DynamicRHI::CreatePixelShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FPixelShaderRHIRef YD3D11DynamicRHI::CreatePixelShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGeometryShaderRHIRef YD3D11DynamicRHI::CreateGeometryShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGeometryShaderRHIRef YD3D11DynamicRHI::CreateGeometryShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGeometryShaderRHIRef YD3D11DynamicRHI::CreateGeometryShaderWithStreamOutput_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FGeometryShaderRHIRef YD3D11DynamicRHI::CreateGeometryShaderWithStreamOutput_RenderThread(class FRHICommandListImmediate& RHICmdList, const FStreamOutElementList& ElementList, uint32 NumStrides, const uint32* Strides, int32 RasterizedStream, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FComputeShaderRHIRef YD3D11DynamicRHI::CreateComputeShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FComputeShaderRHIRef YD3D11DynamicRHI::CreateComputeShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FHullShaderRHIRef YD3D11DynamicRHI::CreateHullShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FHullShaderRHIRef YD3D11DynamicRHI::CreateHullShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FDomainShaderRHIRef YD3D11DynamicRHI::CreateDomainShader_RenderThread(class FRHICommandListImmediate& RHICmdList, const std::vector<uint8>& Code)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FDomainShaderRHIRef YD3D11DynamicRHI::CreateDomainShader_RenderThread(class FRHICommandListImmediate& RHICmdList, FRHIShaderLibraryParamRef Library, FSHAHash Hash)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::LockTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture, uint32 MipIndex, EResourceLockMode LockMode, uint32& DestStride, bool bLockWithinMiptail, bool bNeedsDefaultRHIFlush /*= true*/)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::UnlockTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture, uint32 MipIndex, bool bLockWithinMiptail, bool bNeedsDefaultRHIFlush /*= true*/)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::UpdateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture, uint32 MipIndex, const struct FUpdateTextureRegion2D& UpdateRegion, uint32 SourcePitch, const uint8* SourceData)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUpdateTexture3DData YD3D11DynamicRHI::BeginUpdateTexture3D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture3DRHIParamRef Texture, uint32 MipIndex, const struct FUpdateTextureRegion3D& UpdateRegion)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::EndUpdateTexture3D_RenderThread(class FRHICommandListImmediate& RHICmdList, FUpdateTexture3DData& UpdateData)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::UpdateTexture3D_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture3DRHIParamRef Texture, uint32 MipIndex, const struct FUpdateTextureRegion3D& UpdateRegion, uint32 SourceRowPitch, uint32 SourceDepthPitch, const uint8* SourceData)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FRHIShaderLibraryRef YD3D11DynamicRHI::RHICreateShaderLibrary_RenderThread(class FRHICommandListImmediate& RHICmdList, EShaderPlatform Platform, std::string FilePath, std::string Name)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTextureReferenceRHIRef YD3D11DynamicRHI::RHICreateTextureReference_RenderThread(class FRHICommandListImmediate& RHICmdList, FLastRenderTimeContainer* LastRenderTime)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DRHIRef YD3D11DynamicRHI::RHICreateTexture2D_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DRHIRef YD3D11DynamicRHI::RHICreateTextureExternal2D_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DArrayRHIRef YD3D11DynamicRHI::RHICreateTexture2DArray_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture3DRHIRef YD3D11DynamicRHI::RHICreateTexture3D_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUnorderedAccessViewRHIRef YD3D11DynamicRHI::RHICreateUnorderedAccessView_RenderThread(class FRHICommandListImmediate& RHICmdList, FStructuredBufferRHIParamRef StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUnorderedAccessViewRHIRef YD3D11DynamicRHI::RHICreateUnorderedAccessView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTextureRHIParamRef Texture, uint32 MipLevel)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUnorderedAccessViewRHIRef YD3D11DynamicRHI::RHICreateUnorderedAccessView_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer, uint8 Format)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FUnorderedAccessViewRHIRef YD3D11DynamicRHI::RHICreateUnorderedAccessView_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef IndexBuffer, uint8 Format)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2DRHI, uint8 MipLevel)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture2DRHI, uint8 MipLevel, uint8 NumMipLevels, uint8 Format)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture3DRHIParamRef Texture3DRHI, uint8 MipLevel)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DArrayRHIParamRef Texture2DArrayRHI, uint8 MipLevel)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FTextureCubeRHIParamRef TextureCubeRHI, uint8 MipLevel)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FVertexBufferRHIParamRef VertexBuffer, uint32 Stride, uint8 Format)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FIndexBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FShaderResourceViewRHIRef YD3D11DynamicRHI::RHICreateShaderResourceView_RenderThread(class FRHICommandListImmediate& RHICmdList, FStructuredBufferRHIParamRef StructuredBuffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTextureCubeRHIRef YD3D11DynamicRHI::RHICreateTextureCube_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Size, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTextureCubeRHIRef YD3D11DynamicRHI::RHICreateTextureCubeArray_RenderThread(class FRHICommandListImmediate& RHICmdList, uint32 Size, uint32 ArraySize, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FRenderQueryRHIRef YD3D11DynamicRHI::RHICreateRenderQuery_RenderThread(class FRHICommandListImmediate& RHICmdList, ERenderQueryType QueryType)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void* YD3D11DynamicRHI::RHILockTextureCubeFace_RenderThread(class FRHICommandListImmediate& RHICmdList, FTextureCubeRHIParamRef Texture, uint32 FaceIndex, uint32 ArrayIndex, uint32 MipIndex, EResourceLockMode LockMode, uint32& DestStride, bool bLockWithinMiptail)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUnlockTextureCubeFace_RenderThread(class FRHICommandListImmediate& RHICmdList, FTextureCubeRHIParamRef Texture, uint32 FaceIndex, uint32 ArrayIndex, uint32 MipIndex, bool bLockWithinMiptail)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIAcquireTransientResource_RenderThread(FTextureRHIParamRef Texture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIAcquireTransientResource_RenderThread(FVertexBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIAcquireTransientResource_RenderThread(FStructuredBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDiscardTransientResource_RenderThread(FTextureRHIParamRef Texture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDiscardTransientResource_RenderThread(FVertexBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDiscardTransientResource_RenderThread(FStructuredBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIReadSurfaceFloatData_RenderThread(class FRHICommandListImmediate& RHICmdList, FTextureRHIParamRef Texture, FIntRect Rect, std::vector<FFloat16Color>& OutData, ECubeFace CubeFace, int32 ArrayIndex, int32 MipIndex)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::EnableIdealGPUCaptureOptions(bool bEnable)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetResourceAliasability_RenderThread(class FRHICommandListImmediate& RHICmdList, EResourceAliasability AliasMode, FTextureRHIParamRef* InTextures, int32 NumTextures)
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool YD3D11DynamicRHI::CheckGpuHeartbeat() const
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::VirtualTextureSetFirstMipInMemory_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture, uint32 FirstMip)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::VirtualTextureSetFirstMipVisible_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef Texture, uint32 FirstMip)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHICopySubTextureRegion_RenderThread(class FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef SourceTexture, FTexture2DRHIParamRef DestinationTexture, FBox2D SourceBox, FBox2D DestinationBox)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHICopySubTextureRegion(FTexture2DRHIParamRef SourceTexture, FTexture2DRHIParamRef DestinationTexture, FBox2D SourceBox, FBox2D DestinationBox)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FRHIFlipDetails YD3D11DynamicRHI::RHIWaitForFlip(double TimeoutInSeconds)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISignalFlipEvent()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHICalibrateTimers()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIWaitComputeFence(FComputeFenceRHIParamRef InFence)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetComputeShader(FComputeShaderRHIParamRef ComputeShader)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDispatchComputeShader(uint32_t ThreadGroupCountX, uint32_t ThreadGroupCountY, uint32_t ThreadGroupCountZ)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDispatchIndirectComputeShader(FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetAsyncComputeBudget(EAsyncComputeBudget Budget)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIAutomaticCacheFlushAfterComputeShader(bool bEnable)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIFlushComputeShaderCache()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetMultipleViewports(uint32_t Count, const FViewportBounds* Data)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIClearTinyUAV(FUnorderedAccessViewRHIParamRef UnorderedAccessViewRHI, const uint32_t* Values)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHICopyToResolveTarget(FTextureRHIParamRef SourceTexture, FTextureRHIParamRef DestTexture, const FResolveParams& ResolveParams)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHITransitionResources(EResourceTransitionAccess TransitionType, FTextureRHIParamRef* InTextures, int32_t NumTextures)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHITransitionResources(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32_t NumUAVs, FComputeFenceRHIParamRef WriteComputeFence)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginRenderQuery(FRenderQueryRHIParamRef RenderQuery)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEndRenderQuery(FRenderQueryRHIParamRef RenderQuery)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISubmitCommandsHint()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDiscardRenderTargets(bool Depth, bool Stencil, uint32_t ColorBitMask)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginDrawingViewport(FViewportRHIParamRef Viewport, FTextureRHIParamRef RenderTargetRHI)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEndDrawingViewport(FViewportRHIParamRef Viewport, bool bPresent, bool bLockToVsync)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginFrame()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEndFrame()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginScene()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEndScene()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginUpdateMultiFrameResource(FTextureRHIParamRef Texture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginUpdateMultiFrameResource(FUnorderedAccessViewRHIParamRef UAV)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEndUpdateMultiFrameResource(FTextureRHIParamRef Texture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEndUpdateMultiFrameResource(FUnorderedAccessViewRHIParamRef UAV)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetStreamSource(uint32_t StreamIndex, FVertexBufferRHIParamRef VertexBuffer, uint32_t Offset)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetViewport(uint32_t MinX, uint32_t MinY, float MinZ, uint32_t MaxX, uint32_t MaxY, float MaxZ)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetStereoViewport(uint32_t LeftMinX, uint32_t RightMinX, uint32_t LeftMinY, uint32_t RightMinY, float MinZ, uint32_t LeftMaxX, uint32_t RightMaxX, uint32_t LeftMaxY, uint32_t RightMaxY, float MaxZ)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetScissorRect(bool bEnable, uint32_t MinX, uint32_t MinY, uint32_t MaxX, uint32_t MaxY)
{
	if (bEnable)
	{
		D3D11_RECT ScissorRect;
		ScissorRect.left = MinX;
		ScissorRect.right = MaxX;
		ScissorRect.top = MinY;
		ScissorRect.bottom = MaxY;
		d3d_dc_->RSSetScissorRects(1, &ScissorRect);
	}
	else
	{
		D3D11_RECT ScissorRect;
		ScissorRect.left = 0;
		ScissorRect.right = GetMax2DTextureDimension();
		ScissorRect.top = 0;
		ScissorRect.bottom = GetMax2DTextureDimension();
		d3d_dc_->RSSetScissorRects(1, &ScissorRect);
	}
}

void YD3D11DynamicRHI::RHISetGraphicsPipelineState(FGraphicsPipelineStateRHIParamRef GraphicsState)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderTexture(FVertexShaderRHIParamRef VertexShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderTexture(FHullShaderRHIParamRef HullShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderTexture(FDomainShaderRHIParamRef DomainShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderTexture(FGeometryShaderRHIParamRef GeometryShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderTexture(FPixelShaderRHIParamRef PixelShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderTexture(FComputeShaderRHIParamRef PixelShader, uint32_t TextureIndex, FTextureRHIParamRef NewTexture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderSampler(FComputeShaderRHIParamRef ComputeShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderSampler(FVertexShaderRHIParamRef VertexShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderSampler(FGeometryShaderRHIParamRef GeometryShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderSampler(FDomainShaderRHIParamRef DomainShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderSampler(FHullShaderRHIParamRef HullShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderSampler(FPixelShaderRHIParamRef PixelShader, uint32_t SamplerIndex, FSamplerStateRHIParamRef NewState)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetUAVParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t UAVIndex, FUnorderedAccessViewRHIParamRef UAV, uint32_t InitialCount)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderResourceViewParameter(FPixelShaderRHIParamRef PixelShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderResourceViewParameter(FVertexShaderRHIParamRef VertexShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderResourceViewParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderResourceViewParameter(FHullShaderRHIParamRef HullShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderResourceViewParameter(FDomainShaderRHIParamRef DomainShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderResourceViewParameter(FGeometryShaderRHIParamRef GeometryShader, uint32_t SamplerIndex, FShaderResourceViewRHIParamRef SRV)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderUniformBuffer(FVertexShaderRHIParamRef VertexShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderUniformBuffer(FHullShaderRHIParamRef HullShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderUniformBuffer(FDomainShaderRHIParamRef DomainShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderUniformBuffer(FGeometryShaderRHIParamRef GeometryShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderUniformBuffer(FPixelShaderRHIParamRef PixelShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderUniformBuffer(FComputeShaderRHIParamRef ComputeShader, uint32_t BufferIndex, FUniformBufferRHIParamRef Buffer)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderParameter(FVertexShaderRHIParamRef VertexShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderParameter(FPixelShaderRHIParamRef PixelShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderParameter(FHullShaderRHIParamRef HullShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderParameter(FDomainShaderRHIParamRef DomainShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderParameter(FGeometryShaderRHIParamRef GeometryShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetShaderParameter(FComputeShaderRHIParamRef ComputeShader, uint32_t BufferIndex, uint32_t BaseIndex, uint32_t NumBytes, const void* NewValue)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetStencilRef(uint32_t StencilRef)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetBlendFactor(const FLinearColor& BlendFactor)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetRenderTargets(uint32_t NumSimultaneousRenderTargets, const FRHIRenderTargetView* NewRenderTargets, const FRHIDepthRenderTargetView* NewDepthStencilTarget, uint32_t NumUAVs, const FUnorderedAccessViewRHIParamRef* UAVs)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetRenderTargetsAndClear(const FRHISetRenderTargetsInfo& RenderTargetsInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBindClearMRTValues(bool bClearColor, bool bClearDepth, bool bClearStencil)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDrawPrimitive(uint32_t PrimitiveType, uint32_t BaseVertexIndex, uint32_t NumPrimitives, uint32_t NumInstances)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDrawPrimitiveIndirect(uint32_t PrimitiveType, FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDrawIndexedIndirect(FIndexBufferRHIParamRef IndexBufferRHI, uint32_t PrimitiveType, FStructuredBufferRHIParamRef ArgumentsBufferRHI, int32_t DrawArgumentsIndex, uint32_t NumInstances)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDrawIndexedPrimitive(FIndexBufferRHIParamRef IndexBuffer, uint32_t PrimitiveType, int32_t BaseVertexIndex, uint32_t FirstInstance, uint32_t NumVertices, uint32_t StartIndex, uint32_t NumPrimitives, uint32_t NumInstances)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIDrawIndexedPrimitiveIndirect(uint32_t PrimitiveType, FIndexBufferRHIParamRef IndexBuffer, FVertexBufferRHIParamRef ArgumentBuffer, uint32_t ArgumentOffset)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginDrawPrimitiveUP(uint32_t PrimitiveType, uint32_t NumPrimitives, uint32_t NumVertices, uint32_t VertexDataStride, void*& OutVertexData)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEndDrawPrimitiveUP()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginDrawIndexedPrimitiveUP(uint32_t PrimitiveType, uint32_t NumPrimitives, uint32_t NumVertices, uint32_t VertexDataStride, void*& OutVertexData, uint32_t MinVertexIndex, uint32_t NumIndices, uint32_t IndexDataStride, void*& OutIndexData)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEndDrawIndexedPrimitiveUP()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetDepthBounds(float MinDepth, float MaxDepth)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIPushEvent(const TCHAR* Name, FColor Color)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIPopEvent()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIUpdateTextureReference(FTextureReferenceRHIParamRef TextureRef, FTextureRHIParamRef NewTexture)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginRenderPass(const FRHIRenderPassInfo& InInfo, const TCHAR* InName)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEndRenderPass()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIBeginComputePass(const TCHAR* InName)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEndComputePass()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHICopyTexture(FTextureRHIParamRef SourceTexture, FTextureRHIParamRef DestTexture, const FRHICopyTextureInfo& CopyInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetComputePipelineState(FRHIComputePipelineState* ComputePipelineState)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIInvalidateCachedState()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetBoundShaderState(FBoundShaderStateRHIParamRef BoundShaderState)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetDepthStencilState(FDepthStencilStateRHIParamRef NewState, uint32_t StencilRef)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetRasterizerState(FRasterizerStateRHIParamRef NewState)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHISetBlendState(FBlendStateRHIParamRef NewState, const FLinearColor& BlendFactor)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIEnableDepthBoundsTest(bool bEnable)
{
	throw std::logic_error("The method or operation is not implemented.");
}

uint64 YD3D11DynamicRHI::RHICalcTexture2DPlatformSize(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, uint32& OutAlign)
{
	throw std::logic_error("The method or operation is not implemented.");
}

uint64 YD3D11DynamicRHI::RHICalcTexture3DPlatformSize(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, uint32& OutAlign)
{
	throw std::logic_error("The method or operation is not implemented.");
}

uint64 YD3D11DynamicRHI::RHICalcTextureCubePlatformSize(uint32 Size, uint8 Format, uint32 NumMips, uint32 Flags, uint32& OutAlign)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIGetTextureMemoryStats(FTextureMemoryStats& OutStats)
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool YD3D11DynamicRHI::RHIGetTextureMemoryVisualizeData(FColor* TextureData, int32 SizeX, int32 SizeY, int32 Pitch, int32 PixelSize)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTextureReferenceRHIRef YD3D11DynamicRHI::RHICreateTextureReference(FLastRenderTimeContainer* LastRenderTime)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DRHIRef YD3D11DynamicRHI::RHICreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DRHIRef YD3D11DynamicRHI::RHICreateTextureExternal2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FStructuredBufferRHIRef YD3D11DynamicRHI::RHICreateRTWriteMaskBuffer(FTexture2DRHIParamRef RenderTarget)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DRHIRef YD3D11DynamicRHI::RHIAsyncCreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 Flags, void** InitialMipData, uint32 NumInitialMips)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHICopySharedMips(FTexture2DRHIParamRef DestTexture2D, FTexture2DRHIParamRef SrcTexture2D)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture2DArrayRHIRef YD3D11DynamicRHI::RHICreateTexture2DArray(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

FTexture3DRHIRef YD3D11DynamicRHI::RHICreateTexture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void YD3D11DynamicRHI::RHIGetResourceInfo(FTextureRHIParamRef Ref, FRHIResourceInfo& OutInfo)
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool YD3D11DynamicRHIModule::IsSupported()
{
	// if not computed yet
	if (!adapter_.IsValid())
	{
		FindAdapter();
	}

	// The hardware must support at least 10.0 (usually 11_0, 10_0 or 10_1).
	return adapter_.IsValid();
}

/** This function is used as a SEH filter to catch only delay load exceptions. */
static bool IsDelayLoadException(PEXCEPTION_POINTERS ExceptionPointers)
{
#if WINVER > 0x502	// Windows SDK 7.1 doesn't define VcppException
	switch (ExceptionPointers->ExceptionRecord->ExceptionCode)
	{
	case VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
	case VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND):
		return EXCEPTION_EXECUTE_HANDLER;
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}
#else
	return EXCEPTION_EXECUTE_HANDLER;
#endif
}


/**
 * Since CreateDXGIFactory1 is a delay loaded import from the D3D11 DLL, if the user
 * doesn't have VistaSP2/DX10, calling CreateDXGIFactory1 will throw an exception.
 * We use SEH to detect that case and fail gracefully.
 */
static void SafeCreateDXGIFactory(IDXGIFactory1** DXGIFactory1)
{
#if !defined(D3D11_CUSTOM_VIEWPORT_CONSTRUCTOR) || !D3D11_CUSTOM_VIEWPORT_CONSTRUCTOR
	__try
	{
		//		if (FParse::Param(FCommandLine::Get(), TEXT("quad_buffer_stereo")))
		//		{
		//			// CreateDXGIFactory2 is only available on Win8.1+, find it if it exists
		//			HMODULE DxgiDLL = LoadLibraryA("dxgi.dll");
		//			if (DxgiDLL)
		//			{
		//#pragma warning(push)
		//#pragma warning(disable: 4191) // disable the "unsafe conversion from 'FARPROC' to 'blah'" warning
		//				CreateDXGIFactory2FnPtr = (FCreateDXGIFactory2)(GetProcAddress(DxgiDLL, "CreateDXGIFactory2"));
		//#pragma warning(pop)
		//				FreeLibrary(DxgiDLL);
		//			}
		//			if (CreateDXGIFactory2FnPtr)
		//			{
		//				bIsQuadBufferStereoEnabled = true;
		//			}
		//			else
		//			{
		//				UE_LOG(LogD3D11RHI, Warning, TEXT("Win8.1 or above ir required for quad_buffer_stereo support."));
		//			}
		//		}

				// IDXGIFactory2 required for dx11.1 active stereo (dxgi1.2)
			/*	if (bIsQuadBufferStereoEnabled && CreateDXGIFactory2FnPtr)
				{
					CreateDXGIFactory2FnPtr(0, __uuidof(IDXGIFactory2), (void**)DXGIFactory1);
				}
				else*/
		{
			CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)DXGIFactory1);
		}
	}
	__except (IsDelayLoadException(GetExceptionInformation()))
	{
		// We suppress warning C6322: Empty _except block. Appropriate checks are made upon returning. 
		//CA_SUPPRESS(6322);
	}
#endif	//!D3D11_CUSTOM_VIEWPORT_CONSTRUCTOR
}


FDynamicRHI* YD3D11DynamicRHIModule::CreateRHI(ERHIFeatureLevel::Type RequestedFeatureLevel /*= ERHIFeatureLevel::Num*/)
{
	TRefCountPtr<IDXGIFactory1> DXGIFactory1;
	SafeCreateDXGIFactory(DXGIFactory1.GetInitReference());
	check(DXGIFactory1);
	return new YD3D11DynamicRHI(DXGIFactory1, adapter_.MaxSupportedFeatureLevel, adapter_.AdapterIndex, ChosenDescription);
}

/**
 * Attempts to create a D3D11 device for the adapter using at most MaxFeatureLevel.
 * If creation is successful, true is returned and the supported feature level is set in OutFeatureLevel.
 */
static bool SafeTestD3D11CreateDevice(IDXGIAdapter* Adapter, D3D_FEATURE_LEVEL MaxFeatureLevel, D3D_FEATURE_LEVEL* OutFeatureLevel)
{
	ID3D11Device* D3DDevice = NULL;
	ID3D11DeviceContext* D3DDeviceContext = NULL;
	uint32 DeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

	// Use a debug device if specified on the command line.
	//if (D3D11RHI_ShouldCreateWithD3DDebug())
#if defined(DEBUG) || defined(_DEBUG)
	{
		DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	D3D_FEATURE_LEVEL RequestedFeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0
	};

	int32 FirstAllowedFeatureLevel = 0;
	int32 NumAllowedFeatureLevels = ARRAY_COUNT(RequestedFeatureLevels);
	while (FirstAllowedFeatureLevel < NumAllowedFeatureLevels)
	{
		if (RequestedFeatureLevels[FirstAllowedFeatureLevel] == MaxFeatureLevel)
		{
			break;
		}
		FirstAllowedFeatureLevel++;
	}
	NumAllowedFeatureLevels -= FirstAllowedFeatureLevel;

	if (NumAllowedFeatureLevels == 0)
	{
		return false;
	}

	__try
	{
		// We don't want software renderer. Ideally we specify D3D_DRIVER_TYPE_HARDWARE on creation but
		// when we specify an adapter we need to specify D3D_DRIVER_TYPE_UNKNOWN (otherwise the call fails).
		// We cannot check the device type later (seems this is missing functionality in D3D).

		if (SUCCEEDED(D3D11CreateDevice(
			Adapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			NULL,
			DeviceFlags,
			&RequestedFeatureLevels[FirstAllowedFeatureLevel],
			NumAllowedFeatureLevels,
			D3D11_SDK_VERSION,
			&D3DDevice,
			OutFeatureLevel,
			&D3DDeviceContext
		)))
		{
			D3DDevice->Release();
			D3DDeviceContext->Release();
			return true;
		}
	}
	__except (IsDelayLoadException(GetExceptionInformation()))
	{
		// We suppress warning C6322: Empty _except block. Appropriate checks are made upon returning. 
		//CA_SUPPRESS(6322);
	}

	return false;
}


static uint32 CountAdapterOutputs(TRefCountPtr<IDXGIAdapter>& Adapter)
{
	uint32 OutputCount = 0;
	for (;;)
	{
		TRefCountPtr<IDXGIOutput> Output;
		HRESULT hr = Adapter->EnumOutputs(OutputCount, Output.GetInitReference());
		if (FAILED(hr))
		{
			break;
		}
		++OutputCount;
	}

	return OutputCount;
}
static const char* GetFeatureLevelString(D3D_FEATURE_LEVEL FeatureLevel)
{
	switch (FeatureLevel)
	{
	case D3D_FEATURE_LEVEL_9_1:		return ("9_1");
	case D3D_FEATURE_LEVEL_9_2:		return ("9_2");
	case D3D_FEATURE_LEVEL_9_3:		return ("9_3");
	case D3D_FEATURE_LEVEL_10_0:	return ("10_0");
	case D3D_FEATURE_LEVEL_10_1:	return ("10_1");
	case D3D_FEATURE_LEVEL_11_0:	return ("11_0");
	case D3D_FEATURE_LEVEL_11_1:	return ("11_1");
	}
	return ("X_X");
}
void YD3D11DynamicRHIModule::FindAdapter()
{
	assert(!adapter_.IsValid());
	// Try to create the DXGIFactory1.  This will fail if we're not running Vista SP2 or higher.
	TRefCountPtr<IDXGIFactory1> DXGIFactory1;
	SafeCreateDXGIFactory(DXGIFactory1.GetInitReference());
	if (!DXGIFactory1)
	{
		return;
	}

	TRefCountPtr<IDXGIAdapter> TempAdapter;
	//D3D_FEATURE_LEVEL MaxAllowedFeatureLevel = GetAllowedD3DFeatureLevel();
	D3D_FEATURE_LEVEL MaxAllowedFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	FD3D11Adapter FirstWithoutIntegratedAdapter;
	FD3D11Adapter FirstAdapter;
	FD3D11Adapter FirstIntegrateAdapter;
	// indexed by AdapterIndex, we store it instead of query it later from the created device to prevent some Optimus bug reporting the data/name of the wrong adapter
	std::vector<DXGI_ADAPTER_DESC> AdapterDescription;

	bool bIsAnyAMD = false;
	bool bIsAnyIntel = false;
	bool bIsAnyNVIDIA = false;

	LOG_INFO("D3D11 adapters:");

	//int PreferredVendor = D3D11RHI_PreferAdaperVendor();
	int PreferredVendor = -1;

	bool bAllowSoftwareFallback = false;

	// Enumerate the DXGIFactory's adapters.
	for (uint32 AdapterIndex = 0; DXGIFactory1->EnumAdapters(AdapterIndex, TempAdapter.GetInitReference()) != DXGI_ERROR_NOT_FOUND; ++AdapterIndex)
	{
		// to make sure the array elements can be indexed with AdapterIndex
		AdapterDescription.push_back(DXGI_ADAPTER_DESC());
		DXGI_ADAPTER_DESC& AdapterDesc = AdapterDescription[AdapterDescription.size() - 1];
		memset(&AdapterDesc, 0, sizeof(DXGI_ADAPTER_DESC));

		// Check that if adapter supports D3D11.
		if (TempAdapter)
		{
			D3D_FEATURE_LEVEL ActualFeatureLevel = (D3D_FEATURE_LEVEL)0;
			if (SafeTestD3D11CreateDevice(TempAdapter, MaxAllowedFeatureLevel, &ActualFeatureLevel))
			{
				// Log some information about the available D3D11 adapters.
				VERIFYD3D11RESULT(TempAdapter->GetDesc(&AdapterDesc));
				uint32 OutputCount = CountAdapterOutputs(TempAdapter);

				/*	UE_LOG(LogD3D11RHI, Log,
						TEXT("  %2u. '%s' (Feature Level %s)"),
						AdapterIndex,
						AdapterDesc.Description,
						GetFeatureLevelString(ActualFeatureLevel)
					);*/
				LOG_INFO(StringFormat("  %2u. '%s' (Feature Level %s)", AdapterIndex, AdapterDesc.Description, GetFeatureLevelString(ActualFeatureLevel)));
				/*	UE_LOG(LogD3D11RHI, Log,
						TEXT("      %u/%u/%u MB DedicatedVideo/DedicatedSystem/SharedSystem, Outputs:%d, VendorId:0x%x"),
						(uint32)(AdapterDesc.DedicatedVideoMemory / (1024 * 1024)),
						(uint32)(AdapterDesc.DedicatedSystemMemory / (1024 * 1024)),
						(uint32)(AdapterDesc.SharedSystemMemory / (1024 * 1024)),
						OutputCount,
						AdapterDesc.VendorId
					);*/
				LOG_INFO(StringFormat("        % u / % u / % u MB DedicatedVideo / DedicatedSystem / SharedSystem, Outputs: % d, VendorId : 0x % x", (uint32)(AdapterDesc.DedicatedVideoMemory / (1024 * 1024)),
					(uint32)(AdapterDesc.DedicatedSystemMemory / (1024 * 1024)),
					(uint32)(AdapterDesc.SharedSystemMemory / (1024 * 1024)), OutputCount, AdapterDesc.VendorId));

				bool bIsAMD = AdapterDesc.VendorId == 0x1002;
				bool bIsIntel = AdapterDesc.VendorId == 0x8086;
				bool bIsNVIDIA = AdapterDesc.VendorId == 0x10DE;
				bool bIsMicrosoft = AdapterDesc.VendorId == 0x1414;

				if (bIsAMD) bIsAnyAMD = true;
				if (bIsIntel) bIsAnyIntel = true;
				if (bIsNVIDIA) bIsAnyNVIDIA = true;

				// Simple heuristic but without profiling it's hard to do better
				const bool bIsIntegrated = bIsIntel;
				// PerfHUD is for performance profiling
				const bool bIsPerfHUD = !strcmp((const char*)AdapterDesc.Description, ("NVIDIA PerfHUD"));

				FD3D11Adapter CurrentAdapter(AdapterIndex, ActualFeatureLevel);
				if (bIsIntegrated)
				{
					FirstIntegrateAdapter = CurrentAdapter;
				}

				// Add special check to support HMDs, which do not have associated outputs.
				// To reject the software emulation, unless the cvar wants it.
				// https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#WARP_new_for_Win8
				// Before we tested for no output devices but that failed where a laptop had a Intel (with output) and NVidia (with no output)
				const bool bSkipSoftwareAdapter = bIsMicrosoft && !bAllowSoftwareFallback/* && CVarExplicitAdapterValue < 0 && HmdGraphicsAdapterLuid == 0*/;

				// we don't allow the PerfHUD adapter
				const bool bSkipPerfHUDAdapter = bIsPerfHUD/* && !bAllowPerfHUD*/;

				// the HMD wants a specific adapter, not this one
				//const bool bSkipHmdGraphicsAdapter = HmdGraphicsAdapterLuid != 0 && FMemory::Memcmp(&HmdGraphicsAdapterLuid, &AdapterDesc.AdapterLuid, sizeof(LUID)) != 0;

				// the user wants a specific adapter, not this one
				//const bool bSkipExplicitAdapter = CVarExplicitAdapterValue >= 0 && AdapterIndex != CVarExplicitAdapterValue;

				const bool bSkipAdapter = bSkipSoftwareAdapter || bSkipPerfHUDAdapter/* || bSkipHmdGraphicsAdapter || bSkipExplicitAdapter*/;

				if (!bSkipAdapter)
				{
					if (!bIsIntegrated && !FirstWithoutIntegratedAdapter.IsValid())
					{
						FirstWithoutIntegratedAdapter = CurrentAdapter;
					}
					else if (PreferredVendor == AdapterDesc.VendorId && FirstWithoutIntegratedAdapter.IsValid())
					{
						FirstWithoutIntegratedAdapter = CurrentAdapter;
					}

					if (!FirstAdapter.IsValid())
					{
						FirstAdapter = CurrentAdapter;
					}
					else if (PreferredVendor == AdapterDesc.VendorId && FirstAdapter.IsValid())
					{
						FirstAdapter = CurrentAdapter;
					}
				}
			}
		}
	}

	if (/*bFavorNonIntegrated && */(bIsAnyAMD || bIsAnyNVIDIA))
	{
		adapter_ = FirstWithoutIntegratedAdapter;

		// We assume Intel is integrated graphics (slower than discrete) than NVIDIA or AMD cards and rather take a different one
		if (!adapter_.IsValid())
		{
			adapter_ = FirstAdapter;
		}
	}
	else
	{
		adapter_ = FirstAdapter;
	}

	const bool force_use_integrate_adapter = true;
	if (force_use_integrate_adapter && FirstIntegrateAdapter.IsValid())
	{
		adapter_ = FirstIntegrateAdapter;
	}

	if (adapter_.IsValid())
	{
		ChosenDescription = AdapterDescription[adapter_.AdapterIndex];
		//UE_LOG(LogD3D11RHI, Log, TEXT("Chosen D3D11 Adapter: %u"), ChosenAdapter.AdapterIndex);
		LOG_INFO("Chosen D3D11 Adapter ", adapter_.AdapterIndex);
	}
	else
	{
		//UE_LOG(LogD3D11RHI, Error, TEXT("Failed to choose a D3D11 Adapter."));
		ERROR_INFO("Failed to choose a D3D11 Adapter.");
	}

	// Workaround to force specific IHVs to SM4.0
	if (adapter_.IsValid() && adapter_.MaxSupportedFeatureLevel != D3D_FEATURE_LEVEL_10_0)
	{
		DXGI_ADAPTER_DESC AdapterDesc;
		ZeroMemory(&AdapterDesc, sizeof(DXGI_ADAPTER_DESC));

		DXGIFactory1->EnumAdapters(adapter_.AdapterIndex, TempAdapter.GetInitReference());
		VERIFYD3D11RESULT(TempAdapter->GetDesc(&AdapterDesc));

		const bool bIsAMD = AdapterDesc.VendorId == 0x1002;
		const bool bIsIntel = AdapterDesc.VendorId == 0x8086;
		const bool bIsNVIDIA = AdapterDesc.VendorId == 0x10DE;

		/*if ((bIsAMD && CVarForceAMDToSM4.GetValueOnGameThread() > 0) ||
			(bIsIntel && CVarForceIntelToSM4.GetValueOnGameThread() > 0) ||
			(bIsNVIDIA && CVarForceNvidiaToSM4.GetValueOnGameThread() > 0))
		{
			ChosenAdapter.MaxSupportedFeatureLevel = D3D_FEATURE_LEVEL_10_0;
		}*/
	}
}
