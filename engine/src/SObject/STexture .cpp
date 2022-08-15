#include "SObject/STexture.h"
#include "Utility/YPath.h"
#include "Engine/YFile.h"
#include <algorithm>
#include <set>
#include "FreeImage.h"
#include "Utilities.h"
#include "Math/YMath.h"

struct FreeImageHelper
{
	static int GetBytePerChannel(FIBITMAP* bitmap)
	{
		FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(bitmap);
		int byte_per_channel = 0;
		if (image_type == FIT_BITMAP)
		{
			byte_per_channel = 1;
		}
		else if (image_type == FIT_UINT16 || image_type == FIT_RGB16 || image_type == FIT_RGBA16)
		{
			byte_per_channel = 2;
		}
		else if (image_type == FIT_FLOAT || image_type == FIT_RGBF || image_type == FIT_RGBAF)
		{
			byte_per_channel = 4;
		}
		return byte_per_channel;
	}

	static int GetChannelCount(FIBITMAP* bitmap)
	{
		const uint32_t bits_per_pixel = FreeImage_GetBPP(bitmap);
		const uint32_t bits_per_channel = GetBytePerChannel(bitmap) * 8;
		const uint32_t channel_count = bits_per_pixel / bits_per_channel;
		assert(channel_count != 0);
		return (int)channel_count;
	}

	static EPixelFormat get_rhi_format(const int byte_per_channel, const int channel_count)
	{
		assert(byte_per_channel != 0);
		assert(channel_count != 0);

		EPixelFormat format = EPixelFormat::PF_Unknown;

		if (channel_count == 1)
		{
			if (byte_per_channel == 1)
			{
				format = EPixelFormat::PF_G8;
			}
			else if (byte_per_channel == 2)
			{
				format = PF_G16;
			}
		}
		else if (channel_count == 2)
		{
			if (byte_per_channel == 2)
			{
				format = PF_R8G8;
			}
		}
		else if (channel_count == 3)
		{
			if (byte_per_channel == 4)
			{
				format = PF_Unknown;
			}
		}
		else if (channel_count == 4)
		{
			if (byte_per_channel == 1)
			{
				format = PF_B8G8R8A8;
			}
			else if (byte_per_channel == 2)
			{
				format = PF_R16G16B16A16_UNORM;
			}
			else if (byte_per_channel == 4)
			{
				format = PF_A32B32G32R32F;
			}
		}

		assert(format != EPixelFormat::PF_Unknown);

		return format;
	}
	static FIBITMAP* ConvertTo32Bits(FIBITMAP* bitmap)
	{
		assert(bitmap != nullptr);

		FIBITMAP* previous_bitmap = bitmap;
		bitmap = FreeImage_ConvertTo32Bits(previous_bitmap);
		FreeImage_Unload(previous_bitmap);

		assert(bitmap != nullptr);

		return bitmap;
	}
	static FIBITMAP* ApplyBitMapCorrection(FIBITMAP* bitmap)
	{
		assert(bitmap);
		// Convert to a standard bitmap. FIT_UINT16 and FIT_RGBA16 are processed without errors
				// but show up empty in the editor. For now, we convert everything to a standard bitmap.
		const FREE_IMAGE_TYPE type = FreeImage_GetImageType(bitmap);
		if (type != FIT_BITMAP)
		{
			// FreeImage can't convert FIT_RGBF
			if (type != FIT_RGBF)
			{
				FIBITMAP* previous_bitmap = bitmap;
				bitmap = FreeImage_ConvertToType(bitmap, FIT_BITMAP);
				FreeImage_Unload(previous_bitmap);
			}
		}

		// Textures with few colors(typically less than 8 bits) and /or a palette color type, get converted to an R8G8B8A8.
		// This is because get_channel_count() returns a single channel, and from there many issues start to occur.
		if (FreeImage_GetColorsUsed(bitmap) <= 256 && FreeImage_GetColorType(bitmap) != FIC_RGB)
		{
			bitmap = ConvertTo32Bits(bitmap);
		}

		// Textures with 3 channels and 8 bit per channel get converted to an R8G8B8A8 format.
		// This is because there is no such RHI_FORMAT format.
		if (GetChannelCount(bitmap) == 3 && GetBytePerChannel(bitmap) == 1)
		{
			bitmap = ConvertTo32Bits(bitmap);
		}

		// Most GPUs can't use a 32 bit RGB texture as a color attachment.
		// Vulkan tells you, your GPU doesn't support it.
		// D3D11 seems to be doing some sort of emulation under the hood while throwing some warnings regarding sampling it.
		// So to prevent that, we maintain the 32 bits and convert to an RGBA format.
		if (GetChannelCount(bitmap) == 3 && GetBytePerChannel(bitmap) == 4)
		{
			FIBITMAP* previous_bitmap = bitmap;
			bitmap = FreeImage_ConvertToRGBAF(bitmap);
			FreeImage_Unload(previous_bitmap);
		}
		// Convert BGR to RGB (if needed)
		if (FreeImage_GetBPP(bitmap) == 32)
		{
			if (FreeImage_GetRedMask(bitmap) == 0xff0000 && GetChannelCount(bitmap) >= 2)
			{
				//if (!SwapRedBlue32(bitmap))
				{
					//ERROR_INFO("Failed to swap red with blue channel");
				}
			}
		}
		// Flip it vertically
		// free image first line in bottom
		FreeImage_FlipVertical(bitmap);
		return bitmap;

	}

	static bool IsSRGB(FIBITMAP* bitmap)
	{
		if (FIICCPROFILE* icc_profile = FreeImage_GetICCProfile(bitmap))
		{
			int i, tag_count, tag_ofs, tag_size;
			unsigned char* icc, * tag, * icc_end;
			char tag_data[256];

			if (!icc_profile->data)
				return false;

			icc = static_cast<unsigned char*>(icc_profile->data);
			if (icc[36] != 'a' || icc[37] != 'c' || icc[38] != 's' || icc[39] != 'p')
				return false; // not an ICC file

			icc_end = icc + icc_profile->size;
			tag_count = icc[128 + 0] * 0x1000000 + icc[128 + 1] * 0x10000 + icc[128 + 2] * 0x100 + icc[128 + 3];

			// search for 'desc' tag
			for (i = 0; i < tag_count; i++)
			{
				tag = icc + 128 + 4 + i * 12;
				if (tag > icc_end)
					return false; // invalid ICC file

				// check for a desc flag
				if (memcmp(tag, "desc", 4) == 0)
				{
					tag_ofs = tag[4] * 0x1000000 + tag[5] * 0x10000 + tag[6] * 0x100 + tag[7];
					tag_size = tag[8] * 0x1000000 + tag[9] * 0x10000 + tag[10] * 0x100 + tag[11];

					if (static_cast<uint32_t>(tag_ofs + tag_size) > icc_profile->size)
						return false; // invalid ICC file

					strncpy_s(tag_data, (char*)(icc + tag_ofs + 12), min(255, tag_size - 12));
					if (strcmp(tag_data, "sRGB IEC61966-2.1") == 0 || strcmp(tag_data, "sRGB IEC61966-2-1") == 0 || strcmp(tag_data, "sRGB IEC61966") == 0 || strcmp(tag_data, "* wsRGB") == 0)
						return true;

					return false;
				}
			}

			return false;
		}

		return false;
	}
};

STexture::STexture()
{

}

STexture::STexture(SObject* parent):SObject(parent)
{

}

STexture::~STexture()
{

}

void STexture::SaveToPackage(const std::string& Path)
{
}

bool STexture::PostLoadOp()
{
	return true;
}

void STexture::Update(double deta_time)
{
}

bool STexture::LoadFromMemoryFile(std::unique_ptr<MemoryFile> mem_file)
{
	FIMEMORY* memory = FreeImage_OpenMemory(mem_file->GetData(), mem_file->GetSize());
	// get the file type
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(memory, 0);

	// load an image from the memory stream
	FIBITMAP* bitmap = FreeImage_LoadFromMemory(fif, memory, 0);
	if (!bitmap)
	{
		WARNING_INFO("load picture ",name_, " failed");
		return false;
	}
    FreeImage_FlipVertical(bitmap);
	has_alpha_ = FreeImage_IsTransparent(bitmap);
	is_greyscale_ = FreeImage_GetColorType(bitmap) == FREE_IMAGE_COLOR_TYPE::FIC_MINISBLACK;
	sRGB_ = FreeImageHelper::IsSRGB(bitmap);
	bitmap = FreeImageHelper::ApplyBitMapCorrection(bitmap);

	if (!bitmap)
	{
		ERROR_INFO(name_, " Failed to apply bitmap corrections");
		return false;
	}

	byte_per_channel_ = FreeImageHelper::GetBytePerChannel(bitmap);
	channel_count_ = FreeImageHelper::GetChannelCount(bitmap);
	pixel_format_ = FreeImageHelper::get_rhi_format(byte_per_channel_, channel_count_);
	width_ = FreeImage_GetWidth(bitmap);
	height_ = FreeImage_GetHeight(bitmap);

	scane_width_ = FreeImage_GetPitch(bitmap);

	memory_size_ = height_* scane_width_;
	int api_size = FreeImage_GetMemorySize(bitmap);
	int dbi_size = FreeImage_GetDIBSize(bitmap);
	//assert(dbi_size == size_bytes);
	data_.clear();
	data_.resize(memory_size_);
	//bugs;
	// FreeImage_GetBits() function will return a pointer to the data stored in a FreeImage-internal format.
	// Where each scanline is usually aligned to a 4-bytes boundary. 
	// This will work fine while you have power-of-two textures. 
	// However, for NPOT textures you will need to specify the correct unpack alignment for OpenGL.
	// There is another function in FreeImage which will allow you to get unaligned tightly packed array of pixels : FreeImage_ConvertToRawBits()
	//BYTE* bytes = FreeImage_GetBits(bitmap);
	FreeImage_ConvertToRawBits(&data_[0], bitmap, scane_width_, 32,
		FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
	FreeImage_Unload(bitmap);
	FreeImage_CloseMemory(memory);
	return true;
}

bool STexture::LoadFromPackage(const std::string& path)
{
	name_ = path;
	//todo pose convert texture, now use full name
	std::string extension = YPath::GetFileExtension(path);
	std::set<std::string> img_ext = { "png","jpeg","jpg", "bmp","dds","psd","tga","exr","hdr","tiff","webp" };
	std::for_each(extension.begin(), extension.end(), [](char& c) {c = ::tolower(c); });
	if (!img_ext.count(extension))
	{
		WARNING_INFO("texture ", path, "'s format not support");
		return false;
	}
	std::string asset_binary_path = path.substr(1);
	bool asset_binary_exist = YPath::FileExists(asset_binary_path);
	if (asset_binary_exist)
	{
		YFile asset_file(asset_binary_path, YFile::FileType(YFile::FT_BINARY | YFile::FT_Read));
		std::unique_ptr<MemoryFile> mem_file = asset_file.ReadFile();
		if (!mem_file)
		{
			ERROR_INFO("load binary package ", path, "failed!, read file error");
			return false;
		}
		if (!LoadFromMemoryFile(std::move(mem_file)))
		{
			ERROR_INFO("load binary package ", path, "failed!, read file error");
			return false;
		}
	}
	else {
		ERROR_INFO("load package ", path, "failed!, file not exist");
		return false;
	}

	return true;
}

TextureType STexture::GetTextureType() const
{
	return texture_type_;
}

EPixelFormat STexture::GetPixelFormat() const
{
	return pixel_format_;
}

int STexture::GetMipMapLevel() const
{
	return mipmap_level_ ;
}

bool STexture::HasMimMap() const
{
	return has_mipmap_;
}

bool STexture::HasAlpha() const
{
	return has_alpha_;
}

bool STexture::IsSRGB() const
{
	return sRGB_;
}

bool STexture::IsGreyScale() const
{
	return is_greyscale_;
}

bool STexture::IsCube() const
{
	return is_cube_;
}

int STexture::GetWidth() const
{
	return width_;
}

int STexture::GetHeight() const
{
	return height_;
}

int STexture::GetBytePerChannel() const
{
	return byte_per_channel_;
}

int STexture::GetScanWidth() const
{
	return scane_width_;
}

int STexture::GetMemorySize() const
{
	return memory_size_;
}

bool STexture::UploadGPUBuffer()
{
	bool generate_mipmap = (width_ == height_) && YMath::IsPowerOfTwo(width_);
	assert(memory_size_);
	if (!memory_size_)
	{
		WARNING_INFO("texture ", name_, " has zero size");
		return false;
	}
	
	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &data_[0];
	data.SysMemPitch = scane_width_;
	data.SysMemSlicePitch = memory_size_;
	texture_2d_ = std::make_unique<D3DTexture2D>(TextureUsage::TU_ShaderResource);
	if (!g_device->Create2DTextureWithSRV(width_, height_, DXGI_FORMAT_B8G8R8A8_UNORM, generate_mipmap, 1, &data, texture_2d_->d3d_texture2d_, texture_2d_->srv_))
	{
		WARNING_INFO(name_, " Create Texture2d failed!");
		return false;
	}
	
	return true;
}

