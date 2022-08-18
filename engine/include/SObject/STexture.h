#pragma once 
#include "SObject.h"
#include "RHI/DirectX11/D3D11Texture.h"
#include "Engine/YPixelFormat.h"

class STexture :public SObject
{

public:
	STexture();
	STexture(SObject* parent);
	~STexture() override;
	static constexpr bool IsInstance() { return false; };
	void SaveToPackage(const std::string& Path) override;
	bool PostLoadOp() override;
	void Update(double deta_time) override;
    bool LoadFromMemoryFile(MemoryFile* mem_file) override;
	bool LoadFromPackage(const std::string& path) override;
	TextureType GetTextureType()const;
	EPixelFormat GetPixelFormat()const;
	int GetMipMapLevel() const;
	bool HasMimMap() const;
	bool HasAlpha() const;
	bool IsSRGB() const;
	bool IsGreyScale() const;
	bool IsCube() const;
	int GetWidth() const;
	int GetHeight() const;
	int GetBytePerChannel() const;
	int GetScanWidth() const;
	int GetMemorySize() const;
	bool UploadGPUBuffer();
//protected:
public:
	TextureType texture_type_ = Texture_Unknown;
	EPixelFormat  pixel_format_ = PF_Unknown;
	int mipmap_level_ = 0;
	bool has_mipmap_ = false;
	bool sRGB_ = true;
	bool has_alpha_ = false;
	bool is_greyscale_ = false;
	bool is_cube_ = false;
	int width_ = 0;
	int height_ = 0;
	int byte_per_channel_=0;
	int channel_count_=0;
	int scane_width_ = 0;
	int memory_size_ = 0;
	std::vector<unsigned char> data_;
	std::unique_ptr<D3DTexture2D> texture_2d_;
};

