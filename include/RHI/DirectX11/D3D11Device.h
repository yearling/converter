#pragma once
#include <initguid.h>
#include <dxgi1_2.h>
#include <windows.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <memory>
#include <string>
#include "RHI/DirectX11/ComPtr.h"
//#include <guiddef.h>
//#include <dxgi1_2.h>
class D3DTextureSampler;
class D3DTextureSamplerManager;

enum SampleFilterType { SF_MIN_MAG_MIP_LINEAR = 0, SF_MIN_MAG_MIP_POINT, SF_ANISOTROPIC, SF_NUM };

enum TextureAddressMode { AM_WRAP, AM_MIRROR, AM_CLAMP, AM_BORDER, AM_NUM };
class D3D11Device {
private:
	D3D11Device(const D3D11Device&) = delete;
	D3D11Device(D3D11Device&&) = delete;
	D3D11Device& operator=(const D3D11Device&) = delete;
	D3D11Device& operator=(D3D11Device&&) = delete;

public:
	D3D11Device();
	~D3D11Device();
	static std::unique_ptr<D3D11Device> CreateD3D11Device();
	bool CreateSwapChain(void* wnd);
	bool OnResize(int width, int height);
	//for debug
	ID3D11Device* GetDevice() const { return d3d_device_; }
	ID3D11DeviceContext* GetDC() const { return d3d_dc_; }
	ID3D11RenderTargetView* GetMainRTV() const;
	ID3D11DepthStencilView* GetMainDSV() const { return main_DSV_; }
	int GetDeviceWidth() const;
	int GetDeviceHeight() const;
	void PreRender();

public:
	// shader
	bool ComplieShaderFromFile(const std::string& file_name, const std::string& entry_point, const std::string& shader_model,
		TComPtr<ID3DBlob>& blob, bool ColomMajor = true);
	bool CreateComputerShader(const std::string& file_name, const std::string& MainPoint, TComPtr<ID3D11ComputeShader>& cs);
	// vs
	bool CreateVertexShader(const std::string& FileName, const std::string& MainPoint, TComPtr<ID3D11VertexShader>& vs,
		const std::string& alias /*= ""*/);
	// ps
	bool CreatePixelShader(const std::string& FileName, const std::string& MainPoint, TComPtr<ID3D11PixelShader>& ps,
		const std::string& alias /*= ""*/);
	// IL
	bool CreateInputLayout(const std::string& FileName, const std::string& VSMainPoint, const D3D11_INPUT_ELEMENT_DESC* pDesc, int number,
		TComPtr<ID3D11InputLayout>& InputLayout, const std::string& alias /*= ""*/);
	// cb
	bool CreateConstantBufferCPUWrite(int size, TComPtr<ID3D11Buffer>& buffer, const std::string& alias /*= ""*/);
	bool CreateConstantBufferDefault(int size, TComPtr<ID3D11Buffer>& buffer, const std::string& alias /*= ""*/);

	// Texture
	bool Create2DTextureSRV(UINT width, UINT height, DXGI_FORMAT format, int mip_level, int sample_count, D3D11_SUBRESOURCE_DATA* data,
		TComPtr<ID3D11Texture2D>& tex2D);
	bool Create2DTextureWithSRV(UINT width, UINT height, DXGI_FORMAT format, int mip_level, int sample_count, D3D11_SUBRESOURCE_DATA* data,
		TComPtr<ID3D11Texture2D>& tex2D, TComPtr<ID3D11ShaderResourceView>& srv);
	// SRV,RTV
	bool Create2DTextureRTV_SRV(UINT width, UINT height, DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& tex2D,
		const std::string& alias /*=""*/);
	// SRV
	bool CreateSRVForTexture2D(DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& tex2D, TComPtr<ID3D11ShaderResourceView>& srv);
	// VB
	bool CreateVertexBufferStatic(UINT ByteWidth, const void* pData, TComPtr<ID3D11Buffer>& buffer);

	bool CreateVertexBufferDynamic(UINT ByteWidth, const void* pData, TComPtr<ID3D11Buffer>& buffer);
	// IB
	bool CreateIndexBuffer(UINT ByteWidth, const void* pData, TComPtr<ID3D11Buffer>& buffer);

	// Sampler
	D3DTextureSampler* GetSamplerState(SampleFilterType filter_type, TextureAddressMode address_mode);
	bool CreateSamplerLinearWrap(TComPtr<ID3D11SamplerState>& sample, const std::string& alias /*=""*/);
	bool CreateSamplerPointWrap(TComPtr<ID3D11SamplerState>& sample, const std::string& alias /*= ""*/);
	bool CreateSamplerLinearClamp(TComPtr<ID3D11SamplerState>& sample, const std::string& alias /*=""*/);
	bool CreateRasterState(TComPtr<ID3D11RasterizerState>& RasterState);
	bool CreateRasterStateNonCull(TComPtr<ID3D11RasterizerState>& RasterState);
	bool CreateBlendState(TComPtr<ID3D11BlendState>& BlendState, bool Opaque /*= true*/);
	bool CreateDepthStencileState(TComPtr<ID3D11DepthStencilState>& ds, bool Write /*= true*/);
	bool CreateDepthStencileStateNoWriteNoTest(TComPtr<ID3D11DepthStencilState>& ds);
	bool CreateRenderTargetView(DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& texture, TComPtr<ID3D11RenderTargetView>& rtv,
		const std::string& alias /*= ""*/);
	bool CreateBufferSRV_UAV(int ByteWidth, TComPtr<ID3D11Buffer>& buffer, const std::string& alias /*= ""*/);

	bool CreateSRVForBuffer(DXGI_FORMAT format, UINT number, TComPtr<ID3D11Buffer> buffer, TComPtr<ID3D11ShaderResourceView>& srv,
		const std::string& alias /*= ""*/);

	bool Create2DTextureDSV_SRV(UINT width, UINT height, DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& tex2D,
		const std::string& alias /*= ""*/);
	bool CreateDSVForTexture2D(DXGI_FORMAT format, TComPtr<ID3D11Texture2D>& tex2D, TComPtr<ID3D11DepthStencilView>& dsv,
		const std::string& alias /*= ""*/);

	bool Create2DTextureArrayDSV_SRV(UINT width, UINT height, DXGI_FORMAT format, UINT ArraySize, TComPtr<ID3D11Texture2D>& tex2D,
		const std::string& alias /*= ""*/);
	// SetRT
	bool SetRenderTarget(ID3D11RenderTargetView *rtv, TComPtr<ID3D11DepthStencilView>& dsv);

	// SetViewPort
	bool SetViewPort(int top_left_x, int top_left_y, int width, int height);

	// present
	bool Present();
	//update
	bool UpdateVBDynaimc(ID3D11Buffer* buffer, int start, void* p_data, int bytes_num);
private:
	TComPtr<ID3D11Device> d3d_device_;
	TComPtr<ID3D11DeviceContext> d3d_dc_;
	TComPtr<IDXGISwapChain1> d3d_swap_chain_;
	unsigned long swap_index_ = 0;
	int width_ = 0;
	int height_ = 0;
	TComPtr<ID3D11RenderTargetView> main_RTVs_;
	TComPtr<ID3D11DepthStencilView> main_DSV_;
	TComPtr<ID3D11Texture2D> main_depth_buffer_;
	std::unique_ptr <D3DTextureSamplerManager> sample_state_mamager_;
};

extern D3D11Device* g_device;