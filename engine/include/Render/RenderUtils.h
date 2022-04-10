#pragma once
#include "RHI/RHIDefinitions.h"
#include "Engine/YCommonHeader.h"
/** Information about a pixel format. */
struct FPixelFormatInfo
{
	const char* Name;
	int32				BlockSizeX,
		BlockSizeY,
		BlockSizeZ,
		BlockBytes,
		NumComponents;
	/** Platform specific token, e.g. D3DFORMAT with D3DDrv										*/
	uint32			PlatformFormat;
	/** Whether the texture format is supported on the current platform/ rendering combination	*/
	bool			Supported;
	EPixelFormat	UnrealFormat;
};

extern  FPixelFormatInfo GPixelFormats[PF_MAX];		// Maps members of EPixelFormat to a FPixelFormatInfo describing the format.