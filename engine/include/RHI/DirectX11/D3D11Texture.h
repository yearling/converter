#pragma once
#include "RHI/DirectX11/D3D11Device.h"
#include "RHI/DirectX11/D3D11Shader.h"
#include "Engine/YPixelFormat.h"
#include <unordered_map>
#include <memory>

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
	TextureUsage usage_ = TextureUsage::TU_NONE;
	bool has_depth_ = false;
	bool has_stencil_ = false;
	bool valid_ = false;
	SamplerAddressMode u_;
	SamplerAddressMode v_;
	SamplerAddressMode w_;
	SamplerFilterType filter_;
};

class D3DTexture2D : public D3DTexture {
public:
	D3DTexture2D(TextureUsage usage);
	~D3DTexture2D();
	bool Create(EPixelFormat pixel_format, int width, int height, int mip_count, int sample_count, TextureUsage texture_usage,
		const unsigned char* data);
	friend ID3DShaderBind;
	friend D3DVertexShader;
	friend D3DPixelShader;
	friend class STexture;
public:
	TComPtr<ID3D11Texture2D> d3d_texture2d_;
	TComPtr<ID3D11ShaderResourceView> srv_;
	TComPtr<ID3D11RenderTargetView> rtv_;
	TComPtr<ID3D11DepthStencilView> dsv_;
};

class D3DTextureCube : public D3DTexture {
public:
	D3DTextureCube(TextureUsage usage);
	~D3DTextureCube();
	bool Create(EPixelFormat pixel_format, int width, int height, int mip_count, int sample_count, TextureUsage texture_usage);

protected:
	TComPtr<ID3D11Texture2D> d3d_texture2d_cube_;
};



class D3DTextureSamplerManager;
class D3DTextureSampler {
public:
	D3DTextureSampler();
	D3DTextureSampler(SamplerFilterType filter_type, SamplerAddressMode adress_mode);
	D3DTextureSampler(SamplerFilterType filter_type, SamplerAddressMode adress_mode_u, SamplerAddressMode adress_mode_v, SamplerAddressMode adress_mode_w);
	virtual ~D3DTextureSampler();
	friend D3DTextureSamplerManager;
	SamplerAddressMode GetSampleAddressModelU() const;
	SamplerAddressMode GetSampleAddressModelV() const;
	SamplerAddressMode GetSampleAddressModelW() const;
	SamplerFilterType GetSamplerFilterType() const;
	ID3D11SamplerState* GetSampler() const;
protected:
	SamplerFilterType fileter_type_;
	SamplerAddressMode address_mode_u_;
	SamplerAddressMode address_mode_v_;
	SamplerAddressMode address_mode_w_;
	float border_color_[4];
	TComPtr <ID3D11SamplerState> sample_state_;
};

class D3DTextureSamplerManager
{
public:
	D3DTextureSamplerManager();
	~D3DTextureSamplerManager();
	D3DTextureSampler* GetTextureSampler(SamplerFilterType filter_type, SamplerAddressMode address_model_u, SamplerAddressMode address_model_v, SamplerAddressMode address_model_w);

private:
	std::unordered_map<int,std::unique_ptr<D3DTextureSampler>> samples_;
};