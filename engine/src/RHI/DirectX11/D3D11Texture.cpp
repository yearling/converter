#include <d3d11.h>
#include <cassert>
#include "RHI/DirectX11/D3D11Texture.h"
#include "Engine/YLog.h"
#include "RHI/D3D11RHI/D3D11RHI.h"

static DXGI_FORMAT PixelFormatToDXPixelFormat(EPixelFormat pixel_format) {
	static const DXGI_FORMAT kDXPixelFormatTable[] = {
		DXGI_FORMAT_UNKNOWN,  // Unknown

		DXGI_FORMAT_UNKNOWN,         // R8G8B8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM,  // R8G8B8A8_UNORM,
		DXGI_FORMAT_B8G8R8A8_UNORM,  // B8G8R8A8_UNORM,

		DXGI_FORMAT_R16_FLOAT,              // R16_FLOAT
		DXGI_FORMAT_R16G16_FLOAT,           // R16G16_FLOAT
		DXGI_FORMAT_UNKNOWN,                // R16G16B16_FLOAT
		DXGI_FORMAT_R16G16B16A16_FLOAT,     // R16G16B16A16_FLOAT
		DXGI_FORMAT_R32G32B32_FLOAT,        // R32G32B32_FLOAT
		DXGI_FORMAT_R32G32B32A32_FLOAT,     // R32G32B32A32_FLOAT
		DXGI_FORMAT_R32_FLOAT,              // R32_FLOAT
		DXGI_FORMAT_R32_SINT,               // R32_INT
		DXGI_FORMAT_R8_UNORM,               // R8_UNORM
		DXGI_FORMAT_D16_UNORM,              // D16_UNORM,
		DXGI_FORMAT_UNKNOWN,                // S8_UINT,
		DXGI_FORMAT_D24_UNORM_S8_UINT,      // D16_UNORM_S8_UINT,
		DXGI_FORMAT_D24_UNORM_S8_UINT,      // D24_UNORM_S8_UINT,
		DXGI_FORMAT_D32_FLOAT,              // D32_SFLOAT,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,  // D24_UNORM_X8_PACKED
		DXGI_FORMAT_D32_FLOAT_S8X24_UINT,   // D32_SFLOAT_S8_UINT,

		DXGI_FORMAT_UNKNOWN,  // ETC2_R8G8B8,
		DXGI_FORMAT_UNKNOWN,  // ETC2_R8G8B8_SRGB,
		DXGI_FORMAT_UNKNOWN,  // ETC2_R8G8B8A1,
		DXGI_FORMAT_UNKNOWN,  // ETC2_R8G8B8A1_SRGB,
		DXGI_FORMAT_UNKNOWN,  // ETC2_R8G8B8A8,
		DXGI_FORMAT_UNKNOWN,  // ETC2_R8G8B8A8_SRGB,

		DXGI_FORMAT_UNKNOWN,  // PVRTC1_RGB_2BPP,
		DXGI_FORMAT_UNKNOWN,  // PVRTC1_RGBA_2BPP,
		DXGI_FORMAT_UNKNOWN,  // PVRTC1_RGB_4BPP,
		DXGI_FORMAT_UNKNOWN,  // PVRTC1_RGBA_4BPP,

		DXGI_FORMAT_UNKNOWN,  // ASTC_RGBA_4x4,
		DXGI_FORMAT_UNKNOWN,  // ASTC_SRGB8_ALPHA8_4x4,
		DXGI_FORMAT_UNKNOWN,  // ASTC_RGBA_6x6,
		DXGI_FORMAT_UNKNOWN   // ASTC_SRGB8_ALPHA8_6x6,
	};

	static const size_t kArrayLength = sizeof(kDXPixelFormatTable) / sizeof(DXGI_FORMAT);

	if (static_cast<size_t>(pixel_format) < kArrayLength) {
		return kDXPixelFormatTable[static_cast<size_t>(pixel_format)];
	}
	else {
		return DXGI_FORMAT_UNKNOWN;
	}
}

static int GetPixelSize(DXGI_FORMAT format) {
	if (format == DXGI_FORMAT_R8G8B8A8_UNORM) {
		return 4;
	}
	else
	{
		assert(0);
	}
	return 0;
}
D3DTexture2D::D3DTexture2D(TextureUsage usage) {}

D3DTexture2D::~D3DTexture2D() {}

bool D3DTexture2D::Create(EPixelFormat pixel_format, int width, int height, int mip_count, int sample_count,
	TextureUsage texture_usage, const unsigned char* data) {
	DXGI_FORMAT dxgi_pixel_format = PixelFormatToDXPixelFormat(pixel_format);
	if (dxgi_pixel_format == DXGI_FORMAT_UNKNOWN) {
		return false;
	}
	int row_bytes = (width * GetPixelSize(dxgi_pixel_format) * 8 + 7) / 8;
	int total_byte = row_bytes * height;

	D3D11_SUBRESOURCE_DATA sub_data;
	sub_data.pSysMem = (void*)data;
	sub_data.SysMemPitch = row_bytes;
	sub_data.SysMemSlicePitch = total_byte;
	bool is_rtv = static_cast<bool>(texture_usage & TextureUsage::TU_RenderTarget);
	bool is_dsv = static_cast<bool>(texture_usage & TextureUsage::TU_DepthStencil);
	bool is_srv = static_cast<bool>(texture_usage & TextureUsage::TU_ShaderResource);

	if (is_rtv && is_srv) {
		// rt and srv
	}
	else if (is_rtv) {
		// only rt
	}
	else if (is_dsv && is_srv) {
		// dsv and srv
	}
	else if (is_dsv) {
		// only dsv
	}
	else if (is_srv) {
		// only srv
		if (!g_device->Create2DTextureWithSRV(width, height, dxgi_pixel_format, mip_count, sample_count, &sub_data, d3d_texture2d_,
			srv_)) {
			ERROR_INFO("D3D11 Create2DTextureWithSRV Texture2D failed!");
			return false;
		}
	}
	else {
		ERROR_INFO("create texture failed, the usage unsupported");
		return false;
	}
	return true;
}


D3DTextureCube::D3DTextureCube(TextureUsage usage) {}

D3DTextureCube::~D3DTextureCube() {}

bool D3DTextureCube::Create(EPixelFormat pixel_format, int width, int height, int mip_count, int sample_count, TextureUsage texture_usage) {
	return true;
}


D3DTexture::D3DTexture() {}

D3DTexture::~D3DTexture() {}


D3DTextureSampler::D3DTextureSampler() : fileter_type_(SF_NUM), address_mode_u_(SA_NUM),address_mode_v_(SA_NUM),address_mode_w_(SA_NUM){}

D3DTextureSampler::D3DTextureSampler(SamplerFilterType filter_type, SamplerAddressMode adress_mode)
	: fileter_type_(filter_type), address_mode_u_(adress_mode),address_mode_v_(adress_mode),address_mode_w_(adress_mode) {}

D3DTextureSampler::D3DTextureSampler(SamplerFilterType filter_type, SamplerAddressMode address_mode_u, SamplerAddressMode adress_mode_v, SamplerAddressMode adress_mode_w)
	: fileter_type_(filter_type), address_mode_u_(address_mode_u), address_mode_v_(adress_mode_v), address_mode_w_(adress_mode_w) {}

D3DTextureSampler::~D3DTextureSampler() {}

SamplerAddressMode D3DTextureSampler::GetSampleAddressModelU() const
{
	return address_mode_u_;
}

SamplerAddressMode D3DTextureSampler::GetSampleAddressModelV() const
{
	return address_mode_v_;
}

SamplerAddressMode D3DTextureSampler::GetSampleAddressModelW() const
{
	return address_mode_w_;
}

SamplerFilterType D3DTextureSampler::GetSamplerFilterType() const
{
	return fileter_type_;
}

ID3D11SamplerState* D3DTextureSampler::GetSampler() const { return sample_state_; }

D3DTextureSamplerManager::D3DTextureSamplerManager() {}

D3DTextureSamplerManager::~D3DTextureSamplerManager() {

}

D3DTextureSampler* D3DTextureSamplerManager::GetTextureSampler(SamplerFilterType filter_type, SamplerAddressMode address_model_u, SamplerAddressMode address_model_v, SamplerAddressMode address_model_w)
{
	if (filter_type == SF_NUM || address_model_u == SA_NUM || address_model_v == SA_NUM || address_model_w == SA_NUM)
	{
		WARNING_INFO("Create Sampler Invalid parameter");
		return nullptr;
	}

	if (address_model_u == SA_Board || address_model_v == SA_Board || address_model_w == SA_Board)
	{
		ERROR_INFO("Sampler state not support  texture filter type: board");
		return nullptr;
	}
	const unsigned int address_u = (unsigned int)address_model_u;
	const unsigned int address_v = (unsigned int)address_model_v;
	const unsigned int address_w = (unsigned int)address_model_w;
	const unsigned int filter = (unsigned int)filter_type;
	const unsigned int hash = address_u << 24 | address_v << 16 | address_w << 8 | filter;
	if (!samples_.count(hash))
	{
		std::unique_ptr<D3DTextureSampler> sampler = std::make_unique<D3DTextureSampler>(filter_type, address_model_u, address_model_v, address_model_w);

		D3D11_SAMPLER_DESC desc;
		memset(&desc, 0, sizeof(D3D11_SAMPLER_DESC));
		switch (filter_type) {
		case SF_BiLinear:
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		case SF_Nearest:
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			break;
		case SF_Tilinear:
			desc.Filter = D3D11_FILTER_ANISOTROPIC;
			break;
		default:
			assert(0);
			break;
		}

		switch (address_u) {
		case SA_Wrap:
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			break;
		case SA_Mirror:
			desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
			break;
		case SA_Clamp:
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			break;
		case SA_Board:
			assert(0);
			break;
		default:
			assert(0);
			break;
		}

		switch (address_v) {
		case SA_Wrap:
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			break;
		case SA_Mirror:
			desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
			break;
		case SA_Clamp:
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			break;
		case SA_Board:
			assert(0);
			break;
		default:
			assert(0);
			break;
		}

		switch (address_w) {
		case SA_Wrap:
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			break;
		case SA_Mirror:
			desc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
			break;
		case SA_Clamp:
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			break;
		case SA_Board:
			assert(0);
			break;
		default:
			assert(0);
			break;
		}

		desc.MipLODBias = 0;
		desc.MaxAnisotropy = 16;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		TComPtr<ID3D11SamplerState> sample_state_tmp;
		HRESULT hr = g_device->GetDevice()->CreateSamplerState(&desc, &sample_state_tmp);
		if (hr != S_OK) {
			ERROR_INFO("create sampler state failed! filter: ",filter," address_u: ", address_u," address_v: ",address_v," address_w: ",address_w);
			return nullptr;
		}
		sampler->sample_state_ = sample_state_tmp;
		samples_.insert_or_assign(hash, std::move(sampler));
	}
	return samples_[hash].get();
}
