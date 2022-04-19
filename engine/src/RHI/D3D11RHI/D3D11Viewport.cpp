// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D11Viewport.cpp: D3D viewport RHI implementation.
=============================================================================*/


#ifndef D3D11_WITH_DWMAPI
#if WINVER > 0x502		// Windows XP doesn't support DWM
	#define D3D11_WITH_DWMAPI	1
#else
	#define D3D11_WITH_DWMAPI	0
#endif
#endif

#if D3D11_WITH_DWMAPI
	#include "Windows/AllowWindowsPlatformTypes.h"
		#include <dwmapi.h>
#endif	//D3D11_WITH_DWMAPI
#include "Engine/YCommonHeader.h"
#include "RHI/D3D11RHI/D3D11Resources.h"
#include "RHI/D3D11RHI/D3D11Util.h"
#include "RHI/D3D11RHI/D3D11RHI.h"
#include "RHI/D3D11RHI/D3D11Viewport.h"
#include "RHI/RHIUtilities.h"
#include <dxgi1_2.h>
#include "Render/YRenderThread.h"

/**
 * RHI console variables used by viewports.
 */
namespace RHIConsoleVariables
{
	int32 bSyncWithDWM = 0;
	//static FAutoConsoleVariableRef CVarSyncWithDWM(
	//	TEXT("RHI.SyncWithDWM"),
	//	bSyncWithDWM,
	//	TEXT("If true, synchronize with the desktop window manager for vblank."),
	//	ECVF_RenderThreadSafe
	//	);

	float RefreshPercentageBeforePresent = 1.0f;
	//static FAutoConsoleVariableRef CVarRefreshPercentageBeforePresent(
	//	TEXT("RHI.RefreshPercentageBeforePresent"),
	//	RefreshPercentageBeforePresent,
	//	TEXT("The percentage of the refresh period to wait before presenting."),
	//	ECVF_RenderThreadSafe
	//	);

	int32 TargetRefreshRate = 0;
	//static FAutoConsoleVariableRef CVarTargetRefreshRate(
	//	TEXT("RHI.TargetRefreshRate"),
	//	TargetRefreshRate,
	//	TEXT("If non-zero, the display will never update more often than the target refresh rate (in Hz)."),
	//	ECVF_RenderThreadSafe
	//	);

	float SyncRefreshThreshold = 1.05f;
	//static FAutoConsoleVariableRef CVarSyncRefreshThreshold(
	//	TEXT("RHI.SyncRefreshThreshold"),
	//	SyncRefreshThreshold,
	//	TEXT("Threshold for time above which vsync will be disabled as a percentage of the refresh rate."),
	//	ECVF_RenderThreadSafe
	//	);

	int32 MaxSyncCounter = 8;
	//static FAutoConsoleVariableRef CVarMaxSyncCounter(
	//	TEXT("RHI.MaxSyncCounter"),
	//	MaxSyncCounter,
	//	TEXT("Maximum sync counter to smooth out vsync transitions."),
	//	ECVF_RenderThreadSafe
	//	);

	int32 SyncThreshold = 7;
	//static FAutoConsoleVariableRef CVarSyncThreshold(
	//	TEXT("RHI.SyncThreshold"),
	//	SyncThreshold,
	//	TEXT("Number of consecutive 'fast' frames before vsync is enabled."),
	//	ECVF_RenderThreadSafe
	//	);

	int32 MaximumFrameLatency = 3;
	//static FAutoConsoleVariableRef CVarMaximumFrameLatency(
	//	TEXT("RHI.MaximumFrameLatency"),
	//	MaximumFrameLatency,
	//	TEXT("Number of frames that can be queued for render."),
	//	ECVF_RenderThreadSafe
	//	);

};

extern void D3D11TextureAllocated2D( FD3D11Texture2D& Texture );

/**
 * Creates a FD3D11Surface to represent a swap chain's back buffer.
 */
FD3D11Texture2D* GetSwapChainSurface(FD3D11DynamicRHI* D3DRHI, EPixelFormat PixelFormat, IDXGISwapChain* SwapChain)
{
	// Grab the back buffer
	TRefCountPtr<ID3D11Texture2D> BackBufferResource;
	VERIFYD3D11RESULT_EX(SwapChain->GetBuffer(0,IID_ID3D11Texture2D,(void**)BackBufferResource.GetInitReference()), D3DRHI->GetDevice());

	// create the render target view
	TRefCountPtr<ID3D11RenderTargetView> BackBufferRenderTargetView;
	TRefCountPtr<ID3D11RenderTargetView> BackBufferRenderTargetViewRight; // right eye RTV
	
	// dx11.1 active stereoscopy initialization
	if (D3DRHI->IsQuadBufferStereoEnabled())
	{
		// left
		CD3D11_RENDER_TARGET_VIEW_DESC RTVDescCD3D11_left(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, DXGI_FORMAT_R10G10B10A2_UNORM, 0, 0, 1);
		VERIFYD3D11RESULT_EX(D3DRHI->GetDevice()->CreateRenderTargetView(BackBufferResource, &RTVDescCD3D11_left, BackBufferRenderTargetView.GetInitReference()), D3DRHI->GetDevice());
		
		// right
		CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewRightDesc_right(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, DXGI_FORMAT_R10G10B10A2_UNORM, 0, 1, 1);
		VERIFYD3D11RESULT_EX(D3DRHI->GetDevice()->CreateRenderTargetView(BackBufferResource, &renderTargetViewRightDesc_right, BackBufferRenderTargetViewRight.GetInitReference()), D3DRHI->GetDevice());
	}
	else
	{
		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT_UNKNOWN;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = 0;
		VERIFYD3D11RESULT_EX(D3DRHI->GetDevice()->CreateRenderTargetView(BackBufferResource, &RTVDesc, BackBufferRenderTargetView.GetInitReference()), D3DRHI->GetDevice());
	}

	D3D11_TEXTURE2D_DESC TextureDesc;
	BackBufferResource->GetDesc(&TextureDesc);

	std::vector<TRefCountPtr<ID3D11RenderTargetView> > RenderTargetViews;
	RenderTargetViews.push_back(BackBufferRenderTargetView);

	// add right eye render target view
	if (D3DRHI->IsQuadBufferStereoEnabled())
	{
		RenderTargetViews.push_back(BackBufferRenderTargetViewRight);
	}

	// create a shader resource view to allow using the backbuffer as a texture
	TRefCountPtr<ID3D11ShaderResourceView> BackBufferShaderResourceView;
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;
	VERIFYD3D11RESULT_EX(D3DRHI->GetDevice()->CreateShaderResourceView(BackBufferResource,&SRVDesc,BackBufferShaderResourceView.GetInitReference()), D3DRHI->GetDevice());

	FD3D11Texture2D* NewTexture = new FD3D11Texture2D(
		D3DRHI,
		BackBufferResource,
		BackBufferShaderResourceView,
		false,
		1,
		RenderTargetViews,
		NULL,
		TextureDesc.Width,
		TextureDesc.Height,
		1,
		1,
		1,
		PixelFormat,
		false,
		false,
		false,
		FClearValueBinding()
		);

	D3D11TextureAllocated2D(*NewTexture);

	NewTexture->DoNoDeferDelete();

	return NewTexture;
}

FD3D11Viewport::~FD3D11Viewport()
{
	check(IsInRenderingThread());

	// Turn off HDR display mode
	D3DRHI->ShutdownHDR();

	// If the swap chain was in fullscreen mode, switch back to windowed before releasing the swap chain.
	// DXGI throws an error otherwise.
	VERIFYD3D11RESULT_EX(SwapChain->SetFullscreenState(false,NULL), D3DRHI->GetDevice());

	FrameSyncEvent.ReleaseResource();

	//D3DRHI->Viewports.Remove(this);
	D3DRHI->Viewports.erase(std::remove(D3DRHI->Viewports.begin(), D3DRHI->Viewports.end(), this), D3DRHI->Viewports.end());
}

DXGI_MODE_DESC FD3D11Viewport::SetupDXGI_MODE_DESC() const
{
	DXGI_MODE_DESC Ret;

	Ret.Width = SizeX;
	Ret.Height = SizeY;
	Ret.RefreshRate.Numerator = 0;	// illamas: use 0 to avoid a potential mismatch with hw
	Ret.RefreshRate.Denominator = 0;	// illamas: ditto
	Ret.Format = GetRenderTargetFormat(PixelFormat);
	Ret.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	Ret.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	return Ret;
}

void FD3D11Viewport::Resize(uint32 InSizeX, uint32 InSizeY, bool bInIsFullscreen, EPixelFormat PreferredPixelFormat)
{
	// Unbind any dangling references to resources
	D3DRHI->RHISetRenderTargets(0, nullptr, nullptr, 0, nullptr);
	D3DRHI->ClearState();
	D3DRHI->GetDeviceContext()->Flush(); // Potential perf hit

	if (IsValidRef(CustomPresent))
	{
		CustomPresent->OnBackBufferResize();
	}

	// Release our backbuffer reference, as required by DXGI before calling ResizeBuffers.
	if (IsValidRef(BackBuffer))
	{
		check(BackBuffer->GetRefCount() == 1);

		checkComRefCount(BackBuffer->GetResource(),1);
		checkComRefCount(BackBuffer->GetRenderTargetView(0, -1),1);
		checkComRefCount(BackBuffer->GetShaderResourceView(),1);
	}
	BackBuffer.SafeRelease();

	if(SizeX != InSizeX || SizeY != InSizeY || PixelFormat != PreferredPixelFormat)
	{
		SizeX = InSizeX;
		SizeY = InSizeY;
		PixelFormat = PreferredPixelFormat;

		check(SizeX > 0);
		check(SizeY > 0);

		// Resize the swap chain.
		DXGI_FORMAT RenderTargetFormat = GetRenderTargetFormat(PixelFormat);
		VERIFYD3D11RESIZEVIEWPORTRESULT(SwapChain->ResizeBuffers(1,SizeX,SizeY,RenderTargetFormat,DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH),SizeX,SizeY,RenderTargetFormat, D3DRHI->GetDevice());

		if(bInIsFullscreen)
		{
			DXGI_MODE_DESC BufferDesc = SetupDXGI_MODE_DESC();

			if (FAILED(SwapChain->ResizeTarget(&BufferDesc)))
			{
				ConditionalResetSwapChain(true);
			}
		}
	}

	if(bIsFullscreen != bInIsFullscreen)
	{
		bIsFullscreen = bInIsFullscreen;
		bIsValid = false;

		// Use ConditionalResetSwapChain to call SetFullscreenState, to handle the failure case.
		// Ignore the viewport's focus state; since Resize is called as the result of a user action we assume authority without waiting for Focus.
		ConditionalResetSwapChain(true);
	}

	// Float RGBA backbuffers are requested whenever HDR mode is desired
	if (PixelFormat == GRHIHDRDisplayOutputFormat && bIsFullscreen)
	{
		D3DRHI->EnableHDR();
	}
	else
	{
		D3DRHI->ShutdownHDR();
	}

	// Create a RHI surface to represent the viewport's back buffer.
	BackBuffer = GetSwapChainSurface(D3DRHI, PixelFormat, SwapChain);
}

/** Returns true if desktop composition is enabled. */
static bool IsCompositionEnabled()
{
	BOOL bDwmEnabled = false;
#if D3D11_WITH_DWMAPI
	DwmIsCompositionEnabled(&bDwmEnabled);
#endif	//D3D11_WITH_DWMAPI
	return !!bDwmEnabled;
}

/** Presents the swap chain checking the return result. */
bool FD3D11Viewport::PresentChecked(int32 SyncInterval)
{
	HRESULT Result = S_OK;
	bool bNeedNativePresent = true;

	if (IsValidRef(CustomPresent))
	{
		bNeedNativePresent = CustomPresent->Present(SyncInterval);
	}

	if (bNeedNativePresent)
	{
		// Present the back buffer to the viewport window.
		Result = SwapChain->Present(SyncInterval, 0);

		if (IsValidRef(CustomPresent))
		{
			CustomPresent->PostPresent();
		}
	}

	VERIFYD3D11RESULT_EX(Result, D3DRHI->GetDevice());
	
	return bNeedNativePresent;
}

/** Blocks the CPU to synchronize with vblank by communicating with DWM. */
void FD3D11Viewport::PresentWithVsyncDWM()
{
#if D3D11_WITH_DWMAPI
	LARGE_INTEGER Cycles;
	DWM_TIMING_INFO TimingInfo;

	// Find out how long since we last flipped and query DWM for timing information.
	QueryPerformanceCounter(&Cycles);
	FMemory::Memzero(TimingInfo);
	TimingInfo.cbSize = sizeof(DWM_TIMING_INFO);
	// Starting at windows 8.1 null must be passed into this method for it to work.  null also works on previous versions
	DwmGetCompositionTimingInfo(nullptr, &TimingInfo);

	uint64 QpcAtFlip = Cycles.QuadPart;
	uint64 CyclesSinceLastFlip = Cycles.QuadPart - LastFlipTime;
	float CPUTime = FPlatformTime::ToMilliseconds(CyclesSinceLastFlip);
	float GPUTime = FPlatformTime::ToMilliseconds(TimingInfo.qpcFrameComplete - LastCompleteTime);
	float DisplayRefreshPeriod = FPlatformTime::ToMilliseconds(TimingInfo.qpcRefreshPeriod);

	// Find the smallest multiple of the refresh rate that is >= 33ms, our target frame rate.
	float RefreshPeriod = DisplayRefreshPeriod;
	if(RHIConsoleVariables::TargetRefreshRate > 0 && RefreshPeriod > 1.0f)
	{
		while(RefreshPeriod - (1000.0f / RHIConsoleVariables::TargetRefreshRate) < -1.0f)
		{
			RefreshPeriod *= 2.0f;
		}
	}


	// If the last frame hasn't completed yet, we don't know how long the GPU took.
	bool bValidGPUTime = (TimingInfo.cFrameComplete > LastFrameComplete);
	if (bValidGPUTime)
	{
		GPUTime /= (float)(TimingInfo.cFrameComplete - LastFrameComplete);
	}

	// Update the sync counter depending on how much time it took to complete the previous frame.
	float FrameTime = FMath::Max<float>(CPUTime, GPUTime);
	if (FrameTime >= RHIConsoleVariables::SyncRefreshThreshold * RefreshPeriod)
	{
		SyncCounter--;
	}
	else if (bValidGPUTime)
	{
		SyncCounter++;
	}
	SyncCounter = FMath::Clamp<int32>(SyncCounter, 0, RHIConsoleVariables::MaxSyncCounter);

	// If frames are being completed quickly enough, block for vsync.
	bool bSync = (SyncCounter >= RHIConsoleVariables::SyncThreshold);
	if (bSync)
	{
		// This flushes the previous present call and blocks until it is made available to DWM.
		D3DRHI->GetDeviceContext()->Flush();
		DwmFlush();

		// We sleep a percentage of the remaining time. The trick is to get the
		// present call in after the vblank we just synced for but with time to
		// spare for the next vblank.
		float MinFrameTime = RefreshPeriod * RHIConsoleVariables::RefreshPercentageBeforePresent;
		float TimeToSleep;
		do 
		{
			QueryPerformanceCounter(&Cycles);
			float TimeSinceFlip = FPlatformTime::ToMilliseconds(Cycles.QuadPart - LastFlipTime);
			TimeToSleep = (MinFrameTime - TimeSinceFlip);
			if (TimeToSleep > 0.0f)
			{
				FPlatformProcess::Sleep(TimeToSleep * 0.001f);
			}
		}
		while (TimeToSleep > 0.0f);
	}

	// Present.
	PresentChecked(/*SyncInterval=*/ 0);

	// If we are forcing <= 30Hz, block the CPU an additional amount of time if needed.
	// This second block is only needed when RefreshPercentageBeforePresent < 1.0.
	if (bSync)
	{
		LARGE_INTEGER LocalCycles;
		float TimeToSleep;
		bool bSaveCycles = false;
		do 
		{
			QueryPerformanceCounter(&LocalCycles);
			float TimeSinceFlip = FPlatformTime::ToMilliseconds(LocalCycles.QuadPart - LastFlipTime);
			TimeToSleep = (RefreshPeriod - TimeSinceFlip);
			if (TimeToSleep > 0.0f)
			{
				bSaveCycles = true;
				FPlatformProcess::Sleep(TimeToSleep * 0.001f);
			}
		}
		while (TimeToSleep > 0.0f);

		if (bSaveCycles)
		{
			Cycles = LocalCycles;
		}
	}

	// If we are dropping vsync reset the counter. This provides a debounce time
	// before which we try to vsync again.
	if (!bSync && bSyncedLastFrame)
	{
		SyncCounter = 0;
	}

	if (bSync != bSyncedLastFrame || UE_LOG_ACTIVE(LogRHI,VeryVerbose))
	{
		UE_LOG(LogRHI,Verbose,TEXT("BlockForVsync[%d]: CPUTime:%.2fms GPUTime[%d]:%.2fms Blocked:%.2fms Pending/Complete:%d/%d"),
			bSync,
			CPUTime,
			bValidGPUTime,
			GPUTime,
			FPlatformTime::ToMilliseconds(Cycles.QuadPart - QpcAtFlip),
			TimingInfo.cFramePending,
			TimingInfo.cFrameComplete);
	}

	// Remember if we synced, when the frame completed, etc.
	bSyncedLastFrame = bSync;
	LastFlipTime = Cycles.QuadPart;
	LastFrameComplete = TimingInfo.cFrameComplete;
	LastCompleteTime = TimingInfo.qpcFrameComplete;
#endif	//D3D11_WITH_DWMAPI
}

bool FD3D11Viewport::Present(bool bLockToVsync)
{
	bool bNativelyPresented = true;
#if	D3D11_WITH_DWMAPI
	// We can't call Present if !bIsValid, as it waits a window message to be processed, but the main thread may not be pumping the message handler.
	if(bIsValid)
	{
		// Check if the viewport's swap chain has been invalidated by DXGI.
		BOOL bSwapChainFullscreenState;
		TRefCountPtr<IDXGIOutput> SwapChainOutput;
		VERIFYD3D11RESULT_EX(SwapChain->GetFullscreenState(&bSwapChainFullscreenState,SwapChainOutput.GetInitReference()), D3DRHI->GetDevice());
		// Can't compare BOOL with bool...
		if ( (!!bSwapChainFullscreenState)  != bIsFullscreen )
		{
			bIsValid = false;
		}
	}

	if (MaximumFrameLatency != RHIConsoleVariables::MaximumFrameLatency)
	{
		MaximumFrameLatency = RHIConsoleVariables::MaximumFrameLatency;	
		TRefCountPtr<IDXGIDevice1> DXGIDevice;
		VERIFYD3D11RESULT_EX(D3DRHI->GetDevice()->QueryInterface(IID_IDXGIDevice, (void**)DXGIDevice.GetInitReference()), D3DRHI->GetDevice());
		DXGIDevice->SetMaximumFrameLatency(MaximumFrameLatency);
	}

	// When desktop composition is enabled, locking to vsync via the Present
	// call is unreliable. Instead, communicate with the desktop window manager
	// directly to enable vsync.
	const bool bSyncWithDWM = bLockToVsync && !bIsFullscreen && RHIConsoleVariables::bSyncWithDWM && IsCompositionEnabled();
	if (bSyncWithDWM)
	{
		PresentWithVsyncDWM();
	}
	else
#endif	//D3D11_WITH_DWMAPI
	{
		// Present the back buffer to the viewport window.
		bNativelyPresented = PresentChecked(bLockToVsync ? RHIGetSyncInterval() : 0);
	}
	return bNativelyPresented;
}
extern FD3D11Texture2D* GetSwapChainSurface(FD3D11DynamicRHI* D3DRHI, EPixelFormat PixelFormat, IDXGISwapChain* SwapChain);

FD3D11Viewport::FD3D11Viewport(FD3D11DynamicRHI* InD3DRHI, HWND InWindowHandle, uint32 InSizeX, uint32 InSizeY, bool bInIsFullscreen, EPixelFormat InPreferredPixelFormat) :
	D3DRHI(InD3DRHI),
	LastFlipTime(0),
	LastFrameComplete(0),
	LastCompleteTime(0),
	SyncCounter(0),
	bSyncedLastFrame(false),
	WindowHandle(InWindowHandle),
	MaximumFrameLatency(3),
	SizeX(InSizeX),
	SizeY(InSizeY),
	bIsFullscreen(bInIsFullscreen),
	PixelFormat(InPreferredPixelFormat),
	bIsValid(true),
	FrameSyncEvent(InD3DRHI)
{
	check(IsInGameThread());
	D3DRHI->Viewports.push_back(this);

	// Ensure that the D3D device has been created.
	D3DRHI->InitD3DDevice();

	// Create a backbuffer/swapchain for each viewport
	TRefCountPtr<IDXGIDevice> DXGIDevice;
	VERIFYD3D11RESULT_EX(D3DRHI->GetDevice()->QueryInterface(IID_IDXGIDevice, (void**)DXGIDevice.GetInitReference()), D3DRHI->GetDevice());

	// If requested, keep a handle to a DXGIOutput so we can force that display on fullscreen swap
	//uint32 DisplayIndex = D3DRHI->GetHDRDetectedDisplayIndex();
	//bForcedFullscreenDisplay = FParse::Value(FCommandLine::Get(), TEXT("FullscreenDisplay="), DisplayIndex);
	bForcedFullscreenDisplay = false;

	if (bForcedFullscreenDisplay || GRHISupportsHDROutput)
	{
		TRefCountPtr<IDXGIAdapter> DXGIAdapter;
		DXGIDevice->GetAdapter((IDXGIAdapter**)DXGIAdapter.GetInitReference());

	/*	if (S_OK != DXGIAdapter->EnumOutputs(DisplayIndex, ForcedFullscreenOutput.GetInitReference()))
		{
			UE_LOG(LogD3D11RHI, Log, TEXT("Failed to find requested output display (%i)."), DisplayIndex);
			ForcedFullscreenOutput = nullptr;
			bForcedFullscreenDisplay = false;
		}*/
		assert(0);
	}
	else
	{
		ForcedFullscreenOutput = nullptr;
	}

	if (PixelFormat == PF_FloatRGBA && bIsFullscreen)
	{
		// Send HDR meta data to enable
		D3DRHI->EnableHDR();
	}

	// Create the swapchain.
	if (InD3DRHI->IsQuadBufferStereoEnabled())
	{
		IDXGIFactory2* Factory2 = (IDXGIFactory2*)D3DRHI->GetFactory();

		BOOL stereoEnabled = Factory2->IsWindowedStereoEnabled();
		if (stereoEnabled)
		{
			DXGI_SWAP_CHAIN_DESC1 SwapChainDesc1;
			ZeroMemory(&SwapChainDesc1, sizeof(DXGI_SWAP_CHAIN_DESC1));

			// Enable stereo 
			SwapChainDesc1.Stereo = true;
			// MSAA Sample count
			SwapChainDesc1.SampleDesc.Count = 1;
			SwapChainDesc1.SampleDesc.Quality = 0;

			SwapChainDesc1.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
			SwapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
			// Double buffering required to create stereo swap chain
			SwapChainDesc1.BufferCount = 2;
			SwapChainDesc1.Scaling = DXGI_SCALING_NONE;
			SwapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			SwapChainDesc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

			IDXGISwapChain1* SwapChain1 = nullptr;
			VERIFYD3D11RESULT_EX((Factory2->CreateSwapChainForHwnd(D3DRHI->GetDevice(), WindowHandle, &SwapChainDesc1, nullptr, nullptr, &SwapChain1)), D3DRHI->GetDevice());
			SwapChain = SwapChain1;
		}
		else
		{
			//UE_LOG(LogD3D11RHI, Log, TEXT("FD3D11Viewport::FD3D11Viewport was not able to create stereo SwapChain; Please enable stereo in driver settings."));
			ERROR_INFO("FD3D11Viewport::FD3D11Viewport was not able to create stereo SwapChain; Please enable stereo in driver settings.");
			InD3DRHI->DisableQuadBufferStereo();
		}
	}

	// if stereo was not activated or not enabled in settings
	if (SwapChain == nullptr)
	{
		// Create the swapchain.
		DXGI_SWAP_CHAIN_DESC SwapChainDesc;
		ZeroMemory(&SwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

		SwapChainDesc.BufferDesc = SetupDXGI_MODE_DESC();
		// MSAA Sample count
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.SampleDesc.Quality = 0;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
		// 1:single buffering, 2:double buffering, 3:triple buffering
		SwapChainDesc.BufferCount = 1;
		SwapChainDesc.OutputWindow = WindowHandle;
		SwapChainDesc.Windowed = !bIsFullscreen;
		// DXGI_SWAP_EFFECT_DISCARD / DXGI_SWAP_EFFECT_SEQUENTIAL
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		VERIFYD3D11RESULT_EX(D3DRHI->GetFactory()->CreateSwapChain(DXGIDevice, &SwapChainDesc, SwapChain.GetInitReference()), D3DRHI->GetDevice());
	}

	// Set the DXGI message hook to not change the window behind our back.
	D3DRHI->GetFactory()->MakeWindowAssociation(WindowHandle, DXGI_MWA_NO_WINDOW_CHANGES);

	// Create a RHI surface to represent the viewport's back buffer.
	BackBuffer = GetSwapChainSurface(D3DRHI, PixelFormat, SwapChain);

	// Tell the window to redraw when they can.
	// @todo: For Slate viewports, it doesn't make sense to post WM_PAINT messages (we swallow those.)
	::PostMessage(WindowHandle, WM_PAINT, 0, 0);

	BeginInitResource(&FrameSyncEvent);
}

void FD3D11Viewport::ConditionalResetSwapChain(bool bIgnoreFocus)
{
	if (!bIsValid)
	{
		// Check if the viewport's window is focused before resetting the swap chain's fullscreen state.
		HWND FocusWindow = ::GetFocus();
		const bool bIsFocused = FocusWindow == WindowHandle;
		const bool bIsIconic = !!::IsIconic(WindowHandle);
		if (bIgnoreFocus || (bIsFocused && !bIsIconic))
		{
			FlushRenderingCommands();

			// Explicit output selection in fullscreen only (commandline or HDR enabled)
			bool bNeedsForcedDisplay = bIsFullscreen && (bForcedFullscreenDisplay || PixelFormat == PF_FloatRGBA);
			HRESULT Result = SwapChain->SetFullscreenState(bIsFullscreen, bNeedsForcedDisplay ? ForcedFullscreenOutput : nullptr);

			if (SUCCEEDED(Result))
			{
				bIsValid = true;
			}
			else
			{
				// Even though the docs say SetFullscreenState always returns S_OK, that doesn't always seem to be the case.
				//UE_LOG(LogD3D11RHI, Log, TEXT("IDXGISwapChain::SetFullscreenState returned %08x; waiting for the next frame to try again."), Result);
				LOG_INFO(StringFormat("IDXGISwapChain::SetFullscreenState returned %08x; waiting for the next frame to try again.", Result));
			}
		}
	}
}


/*=============================================================================
 *	The following RHI functions must be called from the main thread.
 *=============================================================================*/
FViewportRHIRef FD3D11DynamicRHI::RHICreateViewport(void* WindowHandle,uint32 SizeX,uint32 SizeY,bool bIsFullscreen,EPixelFormat PreferredPixelFormat)
{
	check( IsInGameThread() );

	// Use a default pixel format if none was specified	
	if (PreferredPixelFormat == EPixelFormat::PF_Unknown)
	{
		//static const auto CVarDefaultBackBufferPixelFormat = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DefaultBackBufferPixelFormat"));
		//PreferredPixelFormat = EDefaultBackBufferPixelFormat::Convert2PixelFormat(EDefaultBackBufferPixelFormat::FromInt(CVarDefaultBackBufferPixelFormat->GetValueOnGameThread()));
		PreferredPixelFormat = PF_B8G8R8A8;
	}

	return new FD3D11Viewport(this,(HWND)WindowHandle,SizeX,SizeY,bIsFullscreen,PreferredPixelFormat);
}

void FD3D11DynamicRHI::RHIResizeViewport(FViewportRHIParamRef ViewportRHI,uint32 SizeX,uint32 SizeY,bool bIsFullscreen)
{
	FD3D11Viewport* Viewport = ResourceCast(ViewportRHI);

	check( IsInGameThread() );
	Viewport->Resize(SizeX,SizeY,bIsFullscreen, PF_Unknown);
}

void FD3D11DynamicRHI::RHIResizeViewport(FViewportRHIParamRef ViewportRHI, uint32 SizeX, uint32 SizeY, bool bIsFullscreen, EPixelFormat PreferredPixelFormat)
{
	check(IsInGameThread());

	// Use a default pixel format if none was specified	
	if (PreferredPixelFormat == EPixelFormat::PF_Unknown)
	{
		//static const auto CVarDefaultBackBufferPixelFormat = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DefaultBackBufferPixelFormat"));
		//PreferredPixelFormat = EDefaultBackBufferPixelFormat::Convert2PixelFormat(EDefaultBackBufferPixelFormat::FromInt(CVarDefaultBackBufferPixelFormat->GetValueOnGameThread()));
		PreferredPixelFormat = PF_B8G8R8A8;
	}

	FD3D11Viewport* Viewport = ResourceCast(ViewportRHI);
	Viewport->Resize(SizeX, SizeY, bIsFullscreen, PreferredPixelFormat);
}

void FD3D11DynamicRHI::RHITick( float DeltaTime )
{
	check( IsInGameThread() );
	
	// Check if any swap chains have been invalidated.
	for(int32 ViewportIndex = 0; ViewportIndex < Viewports.size(); ViewportIndex++)
	{
		Viewports[ViewportIndex]->ConditionalResetSwapChain(false);
	}
}

/*=============================================================================
 *	Viewport functions.
 *=============================================================================*/

void FD3D11DynamicRHI::RHIBeginDrawingViewport(FViewportRHIParamRef ViewportRHI, FTextureRHIParamRef RenderTarget)
{
	FD3D11Viewport* Viewport = ResourceCast(ViewportRHI);

	SCOPE_CYCLE_COUNTER(STAT_D3D11PresentTime);

	check(!DrawingViewport);
	DrawingViewport = Viewport;

	// Set the render target and viewport.
	if( RenderTarget == NULL )
	{
		RenderTarget = Viewport->GetBackBuffer();
		RHITransitionResources(EResourceTransitionAccess::EWritable, &RenderTarget, 1);
	}
	FRHIRenderTargetView View(RenderTarget, ERenderTargetLoadAction::ELoad);
	RHISetRenderTargets(1,&View,nullptr,0,NULL);

	// Set an initially disabled scissor rect.
	RHISetScissorRect(false,0,0,0,0);
}

void FD3D11DynamicRHI::RHIEndDrawingViewport(FViewportRHIParamRef ViewportRHI,bool bPresent,bool bLockToVsync)
{
	++PresentCounter;
	FD3D11Viewport* Viewport = ResourceCast(ViewportRHI);

	SCOPE_CYCLE_COUNTER(STAT_D3D11PresentTime);

	check(DrawingViewport.GetReference() == Viewport);
	DrawingViewport = NULL;

	// Clear references the device might have to resources.
	CurrentDepthTexture = NULL;
	CurrentDepthStencilTarget = NULL;
	CurrentDSVAccessType = FExclusiveDepthStencil::DepthWrite_StencilWrite;
	CurrentRenderTargets[0] = NULL;
	for(uint32 RenderTargetIndex = 1;RenderTargetIndex < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;++RenderTargetIndex)
	{
		CurrentRenderTargets[RenderTargetIndex] = NULL;
	}

	ClearAllShaderResources();

	CommitRenderTargetsAndUAVs();

	StateCache.SetVertexShader(nullptr);

	uint16 NullStreamStrides[MaxVertexElementCount] = {0};
	StateCache.SetStreamStrides(NullStreamStrides);
	for (uint32 StreamIndex = 0; StreamIndex < MaxVertexElementCount; ++StreamIndex)
	{
		StateCache.SetStreamSource(nullptr, StreamIndex, 0, 0);
	}

	StateCache.SetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
	StateCache.SetPixelShader(nullptr);
	StateCache.SetHullShader(nullptr);
	StateCache.SetDomainShader(nullptr);
	StateCache.SetGeometryShader(nullptr);
	// Compute Shader is set to NULL after each Dispatch call, so no need to clear it here

	bool bNativelyPresented = Viewport->Present(bLockToVsync);

	// Don't wait on the GPU when using SLI, let the driver determine how many frames behind the GPU should be allowed to get
	if (GNumAlternateFrameRenderingGroups == 1)
	{
		if (bNativelyPresented)
		{ 
			//static const auto CFinishFrameVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FinishCurrentFrame"));
			int CFinishFrameVar = 0;
			//if (!CFinishFrameVar->GetValueOnRenderThread())
			if (!CFinishFrameVar )
			{
				// Wait for the GPU to finish rendering the previous frame before finishing this frame.
				Viewport->WaitForFrameEventCompletion();
				Viewport->IssueFrameEvent();
			}
			else
			{
				// Finish current frame immediately to reduce latency
				Viewport->IssueFrameEvent();
				Viewport->WaitForFrameEventCompletion();
			}
		}

		// If the input latency timer has been triggered, block until the GPU is completely
		// finished displaying this frame and calculate the delta time.
		if ( GInputLatencyTimer.RenderThreadTrigger )
		{
			Viewport->WaitForFrameEventCompletion();
			uint32 EndTime = FPlatformTime::Cycles();
			GInputLatencyTimer.DeltaTime = EndTime - GInputLatencyTimer.StartTime;
			GInputLatencyTimer.RenderThreadTrigger = false;
		}
	}

#if CHECK_SRV_TRANSITIONS
	check(UnresolvedTargetsConcurrencyGuard.Increment() == 1);
	UnresolvedTargets.Reset();
	check(UnresolvedTargetsConcurrencyGuard.Decrement() == 0);
#endif
}

void FD3D11DynamicRHI::RHIAdvanceFrameForGetViewportBackBuffer(FViewportRHIParamRef Viewport)
{
}

FTexture2DRHIRef FD3D11DynamicRHI::RHIGetViewportBackBuffer(FViewportRHIParamRef ViewportRHI)
{
	FD3D11Viewport* Viewport = ResourceCast(ViewportRHI);

	return Viewport->GetBackBuffer();
}

//#if D3D11_WITH_DWMAPI
//	#include "Windows/HideWindowsPlatformTypes.h"
//#endif	//D3D11_WITH_DWMAPI
