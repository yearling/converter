#include "RHI/DirectX11/D3D11Device.h"
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <cassert>
#include "Engine/YLog.h"
#include "RHI/DirectX11/D3D11Texture.h"

D3D11Device* g_device = nullptr;
D3D11Device::D3D11Device() {}
D3D11Device::~D3D11Device() {}
std::unique_ptr<D3D11Device>D3D11Device::CreateD3D11Device() {
	std::unique_ptr<D3D11Device> device = std::make_unique<D3D11Device>();

	D3D_FEATURE_LEVEL d3d_feature_level;
	UINT create_device_flags = 0;
#    if defined(DEBUG) || defined(_DEBUG)
	create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#    endif
	ID3D11Device* d3d_device = nullptr;
	ID3D11DeviceContext* d3d_dc = nullptr;
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, create_device_flags, nullptr, 0, D3D11_SDK_VERSION, &d3d_device,
		&d3d_feature_level, &d3d_dc);
	if (FAILED(hr)) {
		ERROR_INFO("Create D3D11 device failed!");
		return nullptr;
	}
	device->d3d_device_.Attach(d3d_device);
	device->d3d_dc_.Attach(d3d_dc);

	device->sample_state_mamager_ = std::make_unique<D3DTextureSamplerManager>(device.get());
	g_device = device.get();
	return std::move(device);
}

bool D3D11Device::CreateSwapChain(void* wnd) {
	assert(d3d_device_);
	assert(wnd);
	cur_hwnd_ = *((HWND*)wnd);
	if (!d3d_device_) {
		return false;
	}

	DXGI_SAMPLE_DESC sample_desc = {};
	sample_desc.Count = 1;
	sample_desc.Quality = 0;
	DXGI_SWAP_CHAIN_DESC1 chain_des;
	chain_des.Width = width_;
	chain_des.Height = height_;
	chain_des.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	chain_des.Stereo = false;
	chain_des.SampleDesc.Count = 1;
	chain_des.SampleDesc.Quality = 0;
	chain_des.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	chain_des.BufferCount = 2;
	chain_des.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	chain_des.Scaling = DXGI_SCALING_NONE;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC full_desc = {};
	full_desc.Windowed = true;
	chain_des.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	chain_des.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	if (disable_Vsync)
	{
		chain_des.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}
	//chain_des.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	TComPtr<IDXGIDevice1> dxgi_device;
	TComPtr<IDXGIAdapter> dxgi_adapter;
	HRESULT hr = S_OK;
	if (FAILED(hr = d3d_device_->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgi_device))) {
		return false;
	}
	if (FAILED(hr = dxgi_device->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgi_adapter))) {
		return false;
	}
	if (FAILED(hr = dxgi_adapter->GetParent(__uuidof(IDXGIFactory5), (void**)&dxgi_factory))) {
		return false;
	}
	/*	BOOL allowTearing = FALSE;
	hr = dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(BOOL));
	if (FAILED(hr))
		allowTearing = FALSE;
	*/
	if (FAILED(hr = dxgi_factory->CreateSwapChainForHwnd(d3d_device_, *(reinterpret_cast<HWND*>(wnd)), &chain_des, &full_desc, nullptr, &d3d_swap_chain_))) {
		return false;
	}
#
	// get back buffer
	TComPtr<ID3D11Texture2D> back_buffer;
	d3d_swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
	hr = d3d_device_->CreateRenderTargetView(back_buffer, nullptr, &main_RTVs_);
	if (FAILED(hr)) {
		ERROR_INFO("Create render target view failed!");
		return false;
	}

	// get back buffer desc
	D3D11_TEXTURE2D_DESC back_buffer_desc;
	back_buffer->GetDesc(&back_buffer_desc);
	width_ = back_buffer_desc.Width;
	height_ = back_buffer_desc.Height;

	if (!CreateDepth2DTexture(width_, height_, DXGI_FORMAT_D24_UNORM_S8_UINT, main_depth_buffer_, "m_DepthStencilTexture"))
	{
		ERROR_INFO("Create Depth texture failed!");
		return false;
	}
	if (!CreateDSVForTexture2D(DXGI_FORMAT_D24_UNORM_S8_UINT, main_depth_buffer_, main_DSV_, "depth_stencil_dsv"))
	{
		ERROR_INFO("Create Depth texture's DSV failed!");
		return false;
	}

	return true;
}


bool D3D11Device::OnResize(int width, int height) {

	if (d3d_dc_)
	{
		d3d_dc_->ClearState();
		d3d_dc_->Flush();
	}
	assert(d3d_swap_chain_);
	if (!d3d_swap_chain_) {
		return false;
	}
	if (width == width_ && height == height_) {
		return true;
	}

	main_RTVs_.Reset();
	main_DSV_.Reset();
	main_depth_buffer_.Reset();
	width_ = width;
	height_ = height;
	HRESULT hr = S_OK;
	if (disable_Vsync)
	{
		d3d_swap_chain_.Reset();
		DXGI_SAMPLE_DESC sample_desc = {};
		sample_desc.Count = 1;
		sample_desc.Quality = 0;
		DXGI_SWAP_CHAIN_DESC1 chain_des;
		chain_des.Width = width_;
		chain_des.Height = height_;
		chain_des.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		chain_des.Stereo = false;
		chain_des.SampleDesc.Count = 1;
		chain_des.SampleDesc.Quality = 0;
		chain_des.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		chain_des.BufferCount = 2;
		chain_des.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		chain_des.Scaling = DXGI_SCALING_NONE;
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC full_desc = {};
		full_desc.Windowed = true;
		chain_des.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		chain_des.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		if (disable_Vsync)
		{
			chain_des.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		}
		if (FAILED(hr = dxgi_factory->CreateSwapChainForHwnd(d3d_device_, cur_hwnd_ , &chain_des, &full_desc, nullptr, &d3d_swap_chain_))) {
			return false;
		}
	}
	else
	{
		if (FAILED(hr = d3d_swap_chain_->ResizeBuffers(2, width_, height_, DXGI_FORMAT_R8G8B8A8_UNORM, 0))) {
			ERROR_INFO("d3d swap chain resize buffer failed!");
		}
	
	}

	TComPtr<ID3D11Texture2D> back_buffer;
	if (FAILED(hr = d3d_swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer)))) {
		ERROR_INFO("d3d swap chain GetBuffer failed!");
	}
	if (FAILED(hr = d3d_device_->CreateRenderTargetView(back_buffer, 0, &main_RTVs_))) {
		ERROR_INFO("d3d device  create render target view failed!");
	}

	CreateDepth2DTexture(width_, height_, DXGI_FORMAT_D24_UNORM_S8_UINT, main_depth_buffer_, "m_DepthStencilTexture");
	CreateDSVForTexture2D(DXGI_FORMAT_D24_UNORM_S8_UINT, main_depth_buffer_, main_DSV_, "depth_stencil_dsv");
	SetRenderTarget(GetMainRTV(), main_DSV_);
	SetViewPort(0, 0, width_, height_);
	return true;
}

ID3D11RenderTargetView* D3D11Device::GetMainRTV() const { return main_RTVs_; }

int D3D11Device::GetDeviceWidth() const { return width_; }

int D3D11Device::GetDeviceHeight() const { return height_; }

void D3D11Device::PreRender() {
	if (!d3d_swap_chain_) {
		return;
	}
	TComPtr<ID3D11Texture2D> back_buffer;
	HRESULT hr = S_OK;
	if (FAILED(hr = d3d_swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer)))) {
		ERROR_INFO("d3d swap chain GetBuffer failed!");
	}
	if (FAILED(hr = d3d_device_->CreateRenderTargetView(back_buffer, 0, &main_RTVs_))) {
		ERROR_INFO("d3d device  create render target view failed!");
	}
}

bool D3D11Device::ComplieShaderFromFile(const std::string& file_name, const std::string& entry_point, const std::string& shader_model,
	TComPtr<ID3DBlob>& blob, bool ColomMajor /*= true*/) {
	HRESULT hr = S_OK;
	DWORD shader_flags = D3DCOMPILE_ENABLE_STRICTNESS;
	if (!ColomMajor) shader_flags |= D3D10_SHADER_PACK_MATRIX_ROW_MAJOR;
#    if defined(DEBUG) || defined(_DEBUG)
	shader_flags |= D3DCOMPILE_DEBUG;
	shader_flags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#    endif
	TComPtr<ID3DBlob> err_bob;
	/* if (FAILED(hr = D3DCompileFromFile(file_name.c_str(), NULL, NULL, entry_point.c_str(), shader_model.c_str(), shader_flags, 0,
									   &blob, &err_bob))) {
		if (err_bob) {
			ERROR_INFO("d3d swap chain resize buffer failed!"<<(char*)err_bob->GetBufferPointer());
		}
		return false;
	}
	*/
	return true;
}
bool D3D11Device::CreateComputerShader(const std::string& file_name, const std::string& MainPoint, TComPtr<ID3D11ComputeShader>& cs) {
	HRESULT hr = S_OK;
	TComPtr<ID3DBlob> vs_blob;
	ComplieShaderFromFile(file_name, MainPoint, "cs_5_0", vs_blob);
	if (FAILED(hr = d3d_device_->CreateComputeShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &cs))) {
		return false;
	}
	return true;
}


bool D3D11Device::CreateVertexShader(const std::string& FileName, const std::string& MainPoint, TComPtr<ID3D11VertexShader>& vs,
	const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	TComPtr<ID3DBlob> vs_blob;
	ComplieShaderFromFile(FileName, MainPoint, "vs_5_0", vs_blob);
	if (FAILED(hr = d3d_device_->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &vs))) {
		return false;
	}
	return true;
}


bool D3D11Device::CreatePixelShader(const std::string& FileName, const std::string& MainPoint, TComPtr<ID3D11PixelShader>& ps,
	const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	TComPtr<ID3DBlob> vs_blob;
	ComplieShaderFromFile(FileName, MainPoint, "ps_5_0", vs_blob);
	if (FAILED(hr = d3d_device_->CreatePixelShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &ps))) {
		return false;
	}
	return true;
}
bool D3D11Device::CreateInputLayout(const std::string& FileName, const std::string& VSMainPoint, const D3D11_INPUT_ELEMENT_DESC* pDesc,
	int number, TComPtr<ID3D11InputLayout>& InputLayout, const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	TComPtr<ID3DBlob> vs_blob;
	ComplieShaderFromFile(FileName, VSMainPoint, "vs_5_0", vs_blob);
	if (FAILED(hr = d3d_device_->CreateInputLayout(pDesc, number, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &InputLayout))) {
		return false;
	}
	return true;
}


bool D3D11Device::CreateConstantBufferCPUWrite(int size, TComPtr<ID3D11Buffer>& buffer, const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.ByteWidth = size;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(hr = d3d_device_->CreateBuffer(&desc, NULL, &buffer))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateConstantBufferDefault(int size, TComPtr<ID3D11Buffer>& buffer, const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.ByteWidth = size;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	if (FAILED(hr = d3d_device_->CreateBuffer(&desc, NULL, &buffer))) {
		return false;
	}
	return true;
}

bool D3D11Device::Create2DTextureSRV(UINT width, UINT height, DXGI_FORMAT format, int mip_level,
	int sample_count, D3D11_SUBRESOURCE_DATA* data, TComPtr<ID3D11Texture2D>& tex2D) {
	HRESULT hr = S_OK;
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.Usage = D3D11_USAGE_DEFAULT;
	// to auto generate mip map, sould use shader resource and render target flag 
	// https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_resource_misc_flag
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MipLevels = 0;
	desc.SampleDesc.Count = sample_count;
	desc.SampleDesc.Quality = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	if (FAILED(hr = d3d_device_->CreateTexture2D(&desc, nullptr, &tex2D))) {
		ERROR_INFO("create texture2d failed!!");
		return false;
	}

	return true;
}


bool D3D11Device::Create2DTextureWithSRV(UINT width, UINT height, DXGI_FORMAT format, int mip_level, int sample_count,
	D3D11_SUBRESOURCE_DATA* data, TComPtr<ID3D11Texture2D>& tex2D,
	TComPtr<ID3D11ShaderResourceView>& srv) {
	if (!Create2DTextureSRV(width, height, format, mip_level, sample_count, data, tex2D)) {
		return false;
	}
	if (!CreateSRVForTexture2D(format, tex2D, srv)) {
		return false;
	}
	d3d_dc_->UpdateSubresource(tex2D, 0, nullptr, data->pSysMem, data->SysMemPitch, data->SysMemSlicePitch);
	d3d_dc_->GenerateMips(srv);
	return true;
}


bool D3D11Device::Create2DTextureRTV_SRV(UINT width, UINT height, DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& tex2D,
	const std::string& alias /*=""*/) {
	HRESULT hr = S_OK;
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	if (FAILED(hr = d3d_device_->CreateTexture2D(&desc, NULL, &tex2D))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateSRVForTexture2D(DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& tex2D, TComPtr<ID3D11ShaderResourceView>& srv) {
	HRESULT hr = S_OK;
	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.Format = format;
	desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = -1; //for all mips
	desc.Texture2D.MostDetailedMip = 0;
	if (FAILED(hr = d3d_device_->CreateShaderResourceView(tex2D, &desc, &srv))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateVertexBufferStatic(UINT ByteWidth, const void* pData, TComPtr<ID3D11Buffer>& buffer) {
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = ByteWidth;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA sub;
	memset(&sub, 0, sizeof(sub));
	sub.pSysMem = pData;
	if (FAILED(hr = d3d_device_->CreateBuffer(&desc, &sub, &buffer))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateVertexBufferDynamic(UINT ByteWidth, const void* pData, TComPtr<ID3D11Buffer>& buffer) {
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = ByteWidth;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	if (pData == NULL) {
		if (FAILED(hr = d3d_device_->CreateBuffer(&desc, NULL, &buffer))) {
			return false;
		}
	}
	else {
		D3D11_SUBRESOURCE_DATA sub;
		memset(&sub, 0, sizeof(sub));
		sub.pSysMem = pData;
		if (FAILED(hr = d3d_device_->CreateBuffer(&desc, &sub, &buffer))) {
			return false;
		}
	}
	return true;
}

bool D3D11Device::CreateIndexBuffer(UINT ByteWidth, const void* pData, TComPtr<ID3D11Buffer>& buffer) {
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = ByteWidth;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA sub;
	memset(&sub, 0, sizeof(sub));
	sub.pSysMem = pData;
	if (FAILED(hr = d3d_device_->CreateBuffer(&desc, &sub, &buffer))) {
		return false;
	}
	return true;
}

D3DTextureSampler* D3D11Device::GetSamplerState(SampleFilterType filter_type, TextureAddressMode address_mode) {
	//return sample_state_mamager_->GetTextureSampler(filter_type, address_mode);
	return nullptr;
}

bool D3D11Device::CreateSamplerLinearWrap(TComPtr<ID3D11SamplerState>& sample, const std::string& alias /*=""*/) {
	HRESULT hr = S_OK;
	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	if (FAILED(hr = d3d_device_->CreateSamplerState(&samDesc, &sample))) {
		return false;
	}
	return true;
}


bool D3D11Device::CreateSamplerPointWrap(TComPtr<ID3D11SamplerState>& sample, const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 8;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	if (FAILED(hr = d3d_device_->CreateSamplerState(&samDesc, &sample))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateSamplerLinearClamp(TComPtr<ID3D11SamplerState>& sample, const std::string& alias /*=""*/) {
	HRESULT hr = S_OK;
	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	if (FAILED(hr = d3d_device_->CreateSamplerState(&samDesc, &sample))) {
		return false;
	}
	return true;
}


bool D3D11Device::CreateRasterState(TComPtr<ID3D11RasterizerState>& RasterState) {
	HRESULT hr = S_OK;
	D3D11_RASTERIZER_DESC desc;
	desc.FillMode = D3D11_FILL_SOLID;

	desc.CullMode = D3D11_CULL_BACK;
	desc.FrontCounterClockwise = TRUE;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0.0f;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthClipEnable = TRUE;
	desc.ScissorEnable = FALSE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	if (FAILED(hr = d3d_device_->CreateRasterizerState(&desc, &RasterState))) {
		return false;
	}
	return true;
}


bool D3D11Device::CreateRasterStateNonCull(TComPtr<ID3D11RasterizerState>& RasterState) {
	HRESULT hr = S_OK;
	D3D11_RASTERIZER_DESC desc;
	desc.FillMode = D3D11_FILL_SOLID;

	desc.CullMode = D3D11_CULL_NONE;
	desc.FrontCounterClockwise = TRUE;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0.0f;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthClipEnable = TRUE;
	desc.ScissorEnable = FALSE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	if (FAILED(hr = d3d_device_->CreateRasterizerState(&desc, &RasterState))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateBlendState(TComPtr<ID3D11BlendState>& BlendState, bool Opaque /*= true*/) {
	HRESULT hr = S_OK;
	D3D11_BLEND_DESC desc;
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;
	desc.RenderTarget[0].BlendEnable = (Opaque) ? FALSE : TRUE;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	if (FAILED(hr = d3d_device_->CreateBlendState(&desc, &BlendState))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateDepthStencileState(TComPtr<ID3D11DepthStencilState>& ds, bool Write /*= true*/) {
	HRESULT hr = S_OK;
	D3D11_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = TRUE;
	desc.DepthWriteMask = Write ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	desc.StencilEnable = FALSE;
	desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	if (FAILED(hr = d3d_device_->CreateDepthStencilState(&desc, &ds))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateDepthStencileStateNoWriteNoTest(TComPtr<ID3D11DepthStencilState>& ds) {
	HRESULT hr = S_OK;
	D3D11_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_LESS;
	desc.StencilEnable = FALSE;
	desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	if (FAILED(hr = d3d_device_->CreateDepthStencilState(&desc, &ds))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateRenderTargetView(DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& texture, TComPtr<ID3D11RenderTargetView>& rtv,
	const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	D3D11_RENDER_TARGET_VIEW_DESC desc;
	desc.Format = format;
	desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;
	if (FAILED(hr = d3d_device_->CreateRenderTargetView(texture, &desc, &rtv))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateBufferSRV_UAV(int ByteWidth, TComPtr<ID3D11Buffer>& buffer, const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.ByteWidth = ByteWidth;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	if (FAILED(hr = d3d_device_->CreateBuffer(&desc, NULL, &buffer))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateSRVForBuffer(DXGI_FORMAT format, UINT number, TComPtr<ID3D11Buffer> buffer,
	TComPtr<ID3D11ShaderResourceView>& srv, const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.Format = format;
	desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	desc.Buffer.ElementOffset = 0;
	desc.Buffer.ElementWidth = number;
	if (FAILED(hr = d3d_device_->CreateShaderResourceView(buffer, &desc, &srv))) {
		return false;
	}
	return true;
}

bool D3D11Device::Create2DTextureDSV_SRV(UINT width, UINT height, DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& tex2D,
	const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	if (FAILED(hr = d3d_device_->CreateTexture2D(&desc, NULL, &tex2D))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateDepth2DTexture(UINT width, UINT height, DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& tex2D, const std::string& alias /*= ""*/)
{
	HRESULT hr = S_OK;
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags =  D3D11_BIND_DEPTH_STENCIL;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	if (FAILED(hr = d3d_device_->CreateTexture2D(&desc, NULL, &tex2D))) {
		return false;
	}
	return true;
}

bool D3D11Device::CreateDSVForTexture2D(DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& tex2D, TComPtr<ID3D11DepthStencilView>& dsv,
	const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	D3D11_DEPTH_STENCIL_VIEW_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.Format = format;
	desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;
	desc.Flags = 0;
	if (FAILED(hr = d3d_device_->CreateDepthStencilView(tex2D, &desc, &dsv))) {
		return false;
	}
	return true;
}

bool D3D11Device::Create2DTextureArrayDSV_SRV(UINT width, UINT height, DXGI_FORMAT format, UINT ArraySize,
	TComPtr<ID3D11Texture2D>& tex2D, const std::string& alias /*= ""*/) {
	HRESULT hr = S_OK;
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = ArraySize;
	desc.Format = format;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	if (FAILED(hr = d3d_device_->CreateTexture2D(&desc, NULL, &tex2D))) {
		return false;
	}
	return true;
}

bool D3D11Device::SetRenderTarget(ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv) {
	assert(d3d_dc_);
	d3d_dc_->OMSetRenderTargets(1, &rtv, dsv);
	return true;
}

bool D3D11Device::SetViewPort(int top_left_x, int top_left_y, int width, int height) {
	D3D11_VIEWPORT view_port;
	view_port.TopLeftX = static_cast<float>(top_left_x);
	view_port.TopLeftY = static_cast<float>(top_left_y);
	view_port.Width = static_cast<float>(width);
	view_port.Height = static_cast<float>(height);
	view_port.MinDepth = 0.0;
	view_port.MaxDepth = 1.0;
	assert(d3d_dc_);
	d3d_dc_->RSSetViewports(1, &view_port);
	return true;
}

bool D3D11Device::Present() {
	if (d3d_swap_chain_) {
		//swap_index_++;
		HRESULT hr = S_OK;
		DXGI_PRESENT_PARAMETERS param;
		param.DirtyRectsCount = 0;
		param.pDirtyRects = nullptr;
		param.pScrollOffset = 0;
		param.pScrollRect = nullptr;
		hr = d3d_swap_chain_->Present1(0, disable_Vsync ? DXGI_PRESENT_ALLOW_TEARING : 0, &param);
		if (hr != S_OK) {
			ERROR_INFO("dx11 present error, error code is :", hr);
		}
	}
	return true;
}

bool D3D11Device::UpdateVBDynaimc(ID3D11Buffer* buffer, int start, void* p_data, unsigned int bytes_num)
{
	D3D11_MAPPED_SUBRESOURCE MapResource;
	HRESULT hr = d3d_dc_->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MapResource);
	if (p_data)
	{
		memcpy(MapResource.pData, p_data, bytes_num);
	}
	d3d_dc_->Unmap(buffer, 0);
	return true;
}

