#include "Render/RenderUtils.h"
#include "Engine/YCommonHeader.h"
#include "Math/YMath.h"
#include "Math/IntPoint.h"
#include "Engine/YPixelFormat.h"

//
//	Pixel format information.
//

FPixelFormatInfo	GPixelFormats[PF_FORMAT_MAX] =
{
	// Name						BlockSizeX	BlockSizeY	BlockSizeZ	BlockBytes	NumComponents	PlatformFormat	Supported		UnrealFormat

	{ ("unknown"),			0,			0,			0,			0,			0,				0,				0,				PF_Unknown			},
	{ ("A32B32G32R32F"),	1,			1,			1,			16,			4,				0,				1,				PF_A32B32G32R32F	},
	{ ("B8G8R8A8"),			1,			1,			1,			4,			4,				0,				1,				PF_B8G8R8A8			},
	{ ("G8"),				1,			1,			1,			1,			1,				0,				1,				PF_G8				},
	{ ("G16"),				1,			1,			1,			2,			1,				0,				1,				PF_G16				},
	{ ("DXT1"),				4,			4,			1,			8,			3,				0,				1,				PF_DXT1				},
	{ ("DXT3"),				4,			4,			1,			16,			4,				0,				1,				PF_DXT3				},
	{ ("DXT5"),				4,			4,			1,			16,			4,				0,				1,				PF_DXT5				},
	{ ("UYVY"),				2,			1,			1,			4,			4,				0,				0,				PF_UYVY				},
	{ ("FloatRGB"),			1,			1,			1,			4,			3,				0,				1,				PF_FloatRGB			},
	{ ("FloatRGBA"),		1,			1,			1,			8,			4,				0,				1,				PF_FloatRGBA		},
	{ ("DepthStencil"),		1,			1,			1,			4,			1,				0,				0,				PF_DepthStencil		},
	{ ("ShadowDepth"),		1,			1,			1,			4,			1,				0,				0,				PF_ShadowDepth		},
	{ ("R32_FLOAT"),		1,			1,			1,			4,			1,				0,				1,				PF_R32_FLOAT		},
	{ ("G16R16"),			1,			1,			1,			4,			2,				0,				1,				PF_G16R16			},
	{ ("G16R16F"),			1,			1,			1,			4,			2,				0,				1,				PF_G16R16F			},
	{ ("G16R16F_FILTER"),	1,			1,			1,			4,			2,				0,				1,				PF_G16R16F_FILTER	},
	{ ("G32R32F"),			1,			1,			1,			8,			2,				0,				1,				PF_G32R32F			},
	{ ("A2B10G10R10"),      1,          1,          1,          4,          4,              0,              1,				PF_A2B10G10R10		},
	{ ("A16B16G16R16"),		1,			1,			1,			8,			4,				0,				1,				PF_A16B16G16R16		},
	{ ("D24"),				1,			1,			1,			4,			1,				0,				1,				PF_D24				},
	{ ("PF_R16F"),			1,			1,			1,			2,			1,				0,				1,				PF_R16F				},
	{ ("PF_R16F_FILTER"),	1,			1,			1,			2,			1,				0,				1,				PF_R16F_FILTER		},
	{ ("BC5"),				4,			4,			1,			16,			2,				0,				1,				PF_BC5				},
	{ ("V8U8"),				1,			1,			1,			2,			2,				0,				1,				PF_V8U8				},
	{ ("A1"),				1,			1,			1,			1,			1,				0,				0,				PF_A1				},
	{ ("FloatR11G11B10"),	1,			1,			1,			4,			3,				0,				0,				PF_FloatR11G11B10	},
	{ ("A8"),				1,			1,			1,			1,			1,				0,				1,				PF_A8				},
	{ ("R32_UINT"),			1,			1,			1,			4,			1,				0,				1,				PF_R32_UINT			},
	{ ("R32_SINT"),			1,			1,			1,			4,			1,				0,				1,				PF_R32_SINT			},

	// IOS Support
	{ ("PVRTC2"),			8,			4,			1,			8,			4,				0,				0,				PF_PVRTC2			},
	{ ("PVRTC4"),			4,			4,			1,			8,			4,				0,				0,				PF_PVRTC4			},

	{ ("R16_UINT"),			1,			1,			1,			2,			1,				0,				1,				PF_R16_UINT			},
	{ ("R16_SINT"),			1,			1,			1,			2,			1,				0,				1,				PF_R16_SINT			},
	{ ("R16G16B16A16_UINT"),1,			1,			1,			8,			4,				0,				1,				PF_R16G16B16A16_UINT},
	{ ("R16G16B16A16_SINT"),1,			1,			1,			8,			4,				0,				1,				PF_R16G16B16A16_SINT},
	{ ("R5G6B5_UNORM"),     1,          1,          1,          2,          3,              0,              1,              PF_R5G6B5_UNORM		},
	{ ("R8G8B8A8"),			1,			1,			1,			4,			4,				0,				1,				PF_R8G8B8A8			},
	{ ("A8R8G8B8"),			1,			1,			1,			4,			4,				0,				1,				PF_A8R8G8B8			},
	{ ("BC4"),				4,			4,			1,			8,			1,				0,				1,				PF_BC4				},
	{ ("R8G8"),				1,			1,			1,			2,			2,				0,				1,				PF_R8G8				},

	{ ("ATC_RGB"),			4,			4,			1,			8,			3,				0,				0,				PF_ATC_RGB			},
	{ ("ATC_RGBA_E"),		4,			4,			1,			16,			4,				0,				0,				PF_ATC_RGBA_E		},
	{ ("ATC_RGBA_I"),		4,			4,			1,			16,			4,				0,				0,				PF_ATC_RGBA_I		},
	{ ("X24_G8"),			1,			1,			1,			1,			1,				0,				0,				PF_X24_G8			},
	{ ("ETC1"),				4,			4,			1,			8,			3,				0,				0,				PF_ETC1				},
	{ ("ETC2_RGB"),			4,			4,			1,			8,			3,				0,				0,				PF_ETC2_RGB			},
	{ ("ETC2_RGBA"),		4,			4,			1,			16,			4,				0,				0,				PF_ETC2_RGBA		},
	{ ("PF_R32G32B32A32_UINT"),1,		1,			1,			16,			4,				0,				1,				PF_R32G32B32A32_UINT},
	{ ("PF_R16G16_UINT"),	1,			1,			1,			4,			4,				0,				1,				PF_R16G16_UINT},

	// ASTC support
	{ ("ASTC_4x4"),			4,			4,			1,			16,			4,				0,				0,				PF_ASTC_4x4			},
	{ ("ASTC_6x6"),			6,			6,			1,			16,			4,				0,				0,				PF_ASTC_6x6			},
	{ ("ASTC_8x8"),			8,			8,			1,			16,			4,				0,				0,				PF_ASTC_8x8			},
	{ ("ASTC_10x10"),		10,			10,			1,			16,			4,				0,				0,				PF_ASTC_10x10		},
	{ ("ASTC_12x12"),		12,			12,			1,			16,			4,				0,				0,				PF_ASTC_12x12		},

	{ ("BC6H"),				4,			4,			1,			16,			3,				0,				1,				PF_BC6H				},
	{ ("BC7"),				4,			4,			1,			16,			4,				0,				1,				PF_BC7				},
	{ ("R8_UINT"),			1,			1,			1,			1,			1,				0,				1,				PF_R8_UINT			},
	{ ("L8"),				1,			1,			1,			1,			1,				0,				0,				PF_L8				},
	{ ("XGXR8"),			1,			1,			1,			4,			4,				0,				1,				PF_XGXR8 			},
	{ ("R8G8B8A8_UINT"),	1,			1,			1,			4,			4,				0,				1,				PF_R8G8B8A8_UINT	},
	{ ("R8G8B8A8_SNORM"),	1,			1,			1,			4,			4,				0,				1,				PF_R8G8B8A8_SNORM	},

	{ ("R16G16B16A16_UINT"),1,			1,			1,			8,			4,				0,				1,				PF_R16G16B16A16_UNORM },
	{ ("R16G16B16A16_SINT"),1,			1,			1,			8,			4,				0,				1,				PF_R16G16B16A16_SNORM },
};

void CalcMipMapExtent3D(uint32 TextureSizeX, uint32 TextureSizeY, uint32 TextureSizeZ, EPixelFormat Format, uint32 MipIndex, uint32& OutXExtent, uint32& OutYExtent, uint32& OutZExtent)
{
	OutXExtent = YMath::Max<uint32>(TextureSizeX >> MipIndex, GPixelFormats[Format].BlockSizeX);
	OutYExtent = YMath::Max<uint32>(TextureSizeY >> MipIndex, GPixelFormats[Format].BlockSizeY);
	OutZExtent = YMath::Max<uint32>(TextureSizeZ >> MipIndex, GPixelFormats[Format].BlockSizeZ);
}

SIZE_T CalcTextureMipMapSize3D(uint32 TextureSizeX, uint32 TextureSizeY, uint32 TextureSizeZ, EPixelFormat Format, uint32 MipIndex)
{
	uint32 XExtent;
	uint32 YExtent;
	uint32 ZExtent;
	CalcMipMapExtent3D(TextureSizeX, TextureSizeY, TextureSizeZ, Format, MipIndex, XExtent, YExtent, ZExtent);

	// Offset MipExtent to round up result
	XExtent += GPixelFormats[Format].BlockSizeX - 1;
	YExtent += GPixelFormats[Format].BlockSizeY - 1;
	ZExtent += GPixelFormats[Format].BlockSizeZ - 1;

	const uint32 XPitch = (XExtent / GPixelFormats[Format].BlockSizeX) * GPixelFormats[Format].BlockBytes;
	const uint32 NumRows = YExtent / GPixelFormats[Format].BlockSizeY;
	const uint32 NumLayers = ZExtent / GPixelFormats[Format].BlockSizeZ;

	return NumLayers * NumRows * XPitch;
}

SIZE_T CalcTextureSize3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, EPixelFormat Format, uint32 MipCount)
{
	SIZE_T Size = 0;
	for (uint32 MipIndex = 0; MipIndex < MipCount; ++MipIndex)
	{
		Size += CalcTextureMipMapSize3D(SizeX, SizeY, SizeZ, Format, MipIndex);
	}
	return Size;
}

FIntPoint CalcMipMapExtent(uint32 TextureSizeX, uint32 TextureSizeY, EPixelFormat Format, uint32 MipIndex)
{
	return FIntPoint(YMath::Max<uint32>(TextureSizeX >> MipIndex, GPixelFormats[Format].BlockSizeX), YMath::Max<uint32>(TextureSizeY >> MipIndex, GPixelFormats[Format].BlockSizeY));
}

SIZE_T CalcTextureSize(uint32 SizeX, uint32 SizeY, EPixelFormat Format, uint32 MipCount)
{
	SIZE_T Size = 0;
	for (uint32 MipIndex = 0; MipIndex < MipCount; ++MipIndex)
	{
		Size += CalcTextureMipMapSize(SizeX, SizeY, Format, MipIndex);
	}
	return Size;
}

SIZE_T CalcTextureMipMapSize(uint32 TextureSizeX, uint32 TextureSizeY, EPixelFormat Format, uint32 MipIndex)
{
	const uint32 WidthInBlocks = CalcTextureMipWidthInBlocks(TextureSizeX, Format, MipIndex);
	const uint32 HeightInBlocks = CalcTextureMipHeightInBlocks(TextureSizeY, Format, MipIndex);
	return WidthInBlocks * HeightInBlocks * GPixelFormats[Format].BlockBytes;
}

SIZE_T CalcTextureMipWidthInBlocks(uint32 TextureSizeX, EPixelFormat Format, uint32 MipIndex)
{
	const uint32 BlockSizeX = GPixelFormats[Format].BlockSizeX;
	const uint32 WidthInTexels = YMath::Max<uint32>(TextureSizeX >> MipIndex, 1);
	const uint32 WidthInBlocks = (WidthInTexels + BlockSizeX - 1) / BlockSizeX;
	return WidthInBlocks;
}

SIZE_T CalcTextureMipHeightInBlocks(uint32 TextureSizeY, EPixelFormat Format, uint32 MipIndex)
{
	const uint32 BlockSizeY = GPixelFormats[Format].BlockSizeY;
	const uint32 HeightInTexels = YMath::Max<uint32>(TextureSizeY >> MipIndex, 1);
	const uint32 HeightInBlocks = (HeightInTexels + BlockSizeY - 1) / BlockSizeY;
	return HeightInBlocks;
}

