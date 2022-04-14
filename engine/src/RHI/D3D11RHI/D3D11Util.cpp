#include "RHI/D3D11RHI/D3D11Util.h"
#include <cassert>
#include <winerror.h>
#include "Engine/YCommonHeader.h"
#include <string>
#include "Utility/YStringFormat.h"
#include "Engine/YLog.h"
#include "RHI/DynamicRHI.h"

#define D3DERR(x) case x: ErrorCodeText = #x; break;

#ifndef _FACD3D 
#define _FACD3D  0x876
#endif	//_FACD3D 
#ifndef MAKE_D3DHRESULT
#define _FACD3D  0x876
#define MAKE_D3DHRESULT( code )  MAKE_HRESULT( 1, _FACD3D, code )
#endif	//MAKE_D3DHRESULT

#if WITH_D3DX_LIBS
#ifndef D3DERR_INVALIDCALL
#define D3DERR_INVALIDCALL MAKE_D3DHRESULT(2156)
#endif//D3DERR_INVALIDCALL
#ifndef D3DERR_WASSTILLDRAWING
#define D3DERR_WASSTILLDRAWING MAKE_D3DHRESULT(540)
#endif//D3DERR_WASSTILLDRAWING
#endif

static std::string GetD3D11DeviceHungErrorString(HRESULT ErrorCode)
{
	std::string  ErrorCodeText;

	switch (ErrorCode)
	{
		D3DERR(DXGI_ERROR_DEVICE_HUNG)
			D3DERR(DXGI_ERROR_DEVICE_REMOVED)
			D3DERR(DXGI_ERROR_DEVICE_RESET)
			D3DERR(DXGI_ERROR_DRIVER_INTERNAL_ERROR)
			D3DERR(DXGI_ERROR_INVALID_CALL)
	//default: ErrorCodeText = FString::Printf(TEXT("%08X"), (int32)ErrorCode);
	default: ErrorCodeText = StringFormat("%08X", (int32)ErrorCode);
	}

	return ErrorCodeText;
}

static std::string GetD3D11ErrorString(HRESULT ErrorCode, ID3D11Device* Device)
{
	std::string ErrorCodeText;

	switch (ErrorCode)
	{
		D3DERR(S_OK);
		D3DERR(D3D11_ERROR_FILE_NOT_FOUND)
			D3DERR(D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS)
#if WITH_D3DX_LIBS
			D3DERR(D3DERR_INVALIDCALL)
			D3DERR(D3DERR_WASSTILLDRAWING)
#endif	//WITH_D3DX_LIBS
			D3DERR(E_FAIL)
			D3DERR(E_INVALIDARG)
			D3DERR(E_OUTOFMEMORY)
			D3DERR(DXGI_ERROR_INVALID_CALL)
			D3DERR(E_NOINTERFACE)
			D3DERR(DXGI_ERROR_DEVICE_REMOVED)
	//default: ErrorCodeText = FString::Printf(TEXT("%08X"), (int32)ErrorCode);
	default: ErrorCodeText = StringFormat("%08X", (int32)ErrorCode);
	}

	if (ErrorCode == DXGI_ERROR_DEVICE_REMOVED && Device)
	{
		HRESULT hResDeviceRemoved = Device->GetDeviceRemovedReason();
		ErrorCodeText += (" " + GetD3D11DeviceHungErrorString(hResDeviceRemoved));
	}

	return ErrorCodeText;
}

#undef D3DERR



static void TerminateOnDeviceRemoved(HRESULT D3DResult, ID3D11Device* Direct3DDevice)
{
	if (GDynamicRHI)
	{
		GDynamicRHI->CheckGpuHeartbeat();
	}

	if (D3DResult == DXGI_ERROR_DEVICE_REMOVED)
	{
#if NV_AFTERMATH
		uint32 Result = 0xffffffff;
		uint32 bDeviceActive = 0;
		if (GDX11NVAfterMathEnabled)
		{
			GFSDK_Aftermath_Device_Status Status;
			auto Res = GFSDK_Aftermath_GetDeviceStatus(&Status);
			Result = uint32(Res);
			if (Res == GFSDK_Aftermath_Result_Success)
			{
				bDeviceActive = Status == GFSDK_Aftermath_Device_Status_Active ? 1 : 0;
			}
		}
		UE_LOG(LogD3D11RHI, Log, TEXT("[Aftermath] GDynamicRHI=%p, GDX11NVAfterMathEnabled=%d, Result=0x%08X, bDeviceActive=%d"), GDynamicRHI, GDX11NVAfterMathEnabled, Result, bDeviceActive);
#else
		//UE_LOG(LogD3D11RHI, Log, TEXT("[Aftermath] NV_AFTERMATH is not set"));
#endif

		GIsGPUCrashed = true;
		if (Direct3DDevice)
		{
			HRESULT hRes = Direct3DDevice->GetDeviceRemovedReason();

			const TCHAR* Reason = TEXT("?");
			switch (hRes)
			{
			case DXGI_ERROR_DEVICE_HUNG:			Reason = TEXT("HUNG"); break;
			case DXGI_ERROR_DEVICE_REMOVED:			Reason = TEXT("REMOVED"); break;
			case DXGI_ERROR_DEVICE_RESET:			Reason = TEXT("RESET"); break;
			case DXGI_ERROR_DRIVER_INTERNAL_ERROR:	Reason = TEXT("INTERNAL_ERROR"); break;
			case DXGI_ERROR_INVALID_CALL:			Reason = TEXT("INVALID_CALL"); break;
			case S_OK:								Reason = TEXT("S_OK"); break;
			}

			// We currently don't support removed devices because FTexture2DResource can't recreate its RHI resources from scratch.
			// We would also need to recreate the viewport swap chains from scratch.			
			//UE_LOG(LogD3D11RHI, Fatal, TEXT("Unreal Engine is exiting due to D3D device being lost. (Error: 0x%X - '%s')"), hRes, Reason);
			ERROR_INFO(StringFormat("Unreal Engine is exiting due to D3D device being lost. (Error: 0x%X - '%s')", hRes, Reason));
		}
		else
		{
			//UE_LOG(LogD3D11RHI, Fatal, TEXT("Unreal Engine is exiting due to D3D device being lost. D3D device was not available to assertain DXGI cause."));
			ERROR_INFO(StringFormat("Unreal Engine is exiting due to D3D device being lost. D3D device was not available to assertain DXGI cause."));
		}

		// Workaround for the fact that in non-monolithic builds the exe gets into a weird state and exception handling fails. 
		// @todo investigate why non-monolithic builds fail to capture the exception when graphics driver crashes.
//#if !IS_MONOLITHIC
		FPlatformMisc::RequestExit(true);
//#endif
	}
}

static void TerminateOnOutOfMemory(HRESULT D3DResult, bool bCreatingTextures)
{
	if (D3DResult == E_OUTOFMEMORY)
	{
		if (bCreatingTextures)
		{
			//FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *LOCTEXT("OutOfVideoMemoryTextures", "Out of video memory trying to allocate a texture! Make sure your video card has the minimum required memory, try lowering the resolution and/or closing other applications that are running. Exiting...").ToString(), TEXT("Error"));
			ERROR_INFO("Out of video memory trying to allocate a texture! Make sure your video card has the minimum required memory, try lowering the resolution and/or closing other applications that are running. Exiting...");
		}
		else
		{
			//FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *NSLOCTEXT("D3D11RHI", "OutOfMemory", "Out of video memory trying to allocate a rendering resource. Make sure your video card has the minimum required memory, try lowering the resolution and/or closing other applications that are running. Exiting...").ToString(), TEXT("Error"));
			ERROR_INFO("Out of video memory trying to allocate a rendering resource.Make sure your video card has the minimum required memory, try lowering the resolution and /or closing other applications that are running.Exiting...");
		}
#if STATS
		GetRendererModule().DebugLogOnCrash();
#endif
		FPlatformMisc::RequestExit(true);
	}
}

void VerifyD3D11Result(HRESULT D3DResult, const char* Code, const char* Filename, uint32_t Line, ID3D11Device* Device)
{
	assert(FAILED(D3DResult));

	const std::string& ErrorString = GetD3D11ErrorString(D3DResult, Device);

	//UE_LOG(LogD3D11RHI, Error, TEXT("%s failed \n at %s:%u \n with error %s"), ANSI_TO_TCHAR(Code), ANSI_TO_TCHAR(Filename), Line, *ErrorString);
	ERROR_INFO(Code, " failed \n at ", Filename, ":", Line, "\n with error ", ErrorString);

	TerminateOnDeviceRemoved(D3DResult, Device);
	TerminateOnOutOfMemory(D3DResult, false);

	//UE_LOG(LogD3D11RHI, Fatal, TEXT("%s failed \n at %s:%u \n with error %s"), ANSI_TO_TCHAR(Code), ANSI_TO_TCHAR(Filename), Line, *ErrorString);
	ERROR_INFO(StringFormat("%s failed \n at %s:%u \n with error %s", Code,Filename,Line, ErrorString));
}

