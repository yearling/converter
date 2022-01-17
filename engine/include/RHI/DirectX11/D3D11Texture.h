#pragma once
#include "RHI/DirectX11/D3D11Device.h"
#include "RHI/DirectX11/D3D11Shader.h"
#include "Render/YPixelFormat.h"
enum TextureType
{
	Texture2D = 0,
	Textrue3D = 1,
	TextureCube = 2,
	TT_Num
};

enum TextureUsage
{
	RenderTarget = 0,
	DepthStencil = 1,
	ShaderResource = 2,
	TU_Num
};

class D3DTexture {
public:
	D3DTexture();
	virtual ~D3DTexture();
	TextureType GetTextureType() const { return tex_type_; }
	TextureUsage GetTextureUsage() const { return usage_; }

	int GetWidth() const { return width_; }
	int GetHeight() const { return height_; }
	int GetDepth() const { return depth_; }
	int GetMipCount() const { return mip_count_; }
	int GetSampleCount() const { return sample_count_; }
	bool HasDepth() const { return has_depth_; }
	bool HasStencil() const { return has_stencil_; }
	bool IsValid() const { return valid_; }
protected:
	int width_ = 0;
	int height_ = 0;
	int depth_ = 0;
	int mip_count_ = 0;
	int sample_count_ = 0;
	TextureType tex_type_ = TextureType::TT_Num;
	TextureUsage usage_ = TextureUsage::TU_Num;
	bool has_depth_ = false;
	bool has_stencil_ = false;
	bool valid_ = false;
};

class D3DTexture2D : public D3DTexture {
public:
	D3DTexture2D(TextureUsage usage);
	~D3DTexture2D();
	bool Create(D3D11Device* d3d_device, PixelFormat pixel_format, int width, int height, int mip_count, int sample_count, TextureUsage texture_usage,
		const unsigned char* data);
	friend ID3DShaderBind;
	friend D3DVertexShader;
	friend D3DPixelShader;
protected:
	TComPtr<ID3D11Texture2D> d3d_texture2d_;
	TComPtr<ID3D11ShaderResourceView> srv_;
	TComPtr<ID3D11RenderTargetView> rtv_;
	TComPtr<ID3D11DepthStencilView> dsv_;
};

class D3DTextureCube : public D3DTexture {
public:
	D3DTextureCube(TextureUsage usage);
	~D3DTextureCube();
	bool Create(PixelFormat pixel_format, int width, int height, int mip_count, int sample_count, TextureUsage texture_usage);

protected:
	TComPtr<ID3D11Texture2D> d3d_texture2d_cube_;
};



class D3DTextureSamplerManager;
class D3DTextureSampler {
public:
	D3DTextureSampler();
	D3DTextureSampler(SampleFilterType filter_type, TextureAddressMode adress_mode);
	virtual ~D3DTextureSampler();
	friend D3DTextureSamplerManager;
	SampleFilterType GetSampleFilterType() const;
	TextureAddressMode GetTextureAddressMode() const;
	ID3D11SamplerState* GetSampler() const;
protected:
	SampleFilterType fileter_type_;
	TextureAddressMode address_mode_;
	float border_color_[4];
	TComPtr < ID3D11SamplerState> sample_state_;
};

class D3DTextureSamplerManager
{
public:
	D3DTextureSamplerManager(D3D11Device* device);
	~D3DTextureSamplerManager();
	D3DTextureSampler* GetTextureSampler(SampleFilterType filter_type, TextureAddressMode address_model);

private:
	std::unique_ptr<D3DTextureSampler> samples_[SF_NUM][AM_NUM - 1];
	D3D11Device* device_;

};