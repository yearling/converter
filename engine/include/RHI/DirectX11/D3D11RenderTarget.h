#pragma once
#include "Render/YPixelFormat.h"
#include "ComPtr.h"
#include "RHI/DirectX11/D3D11Device.h"
#include "Math/YVector.h"

class IRenderTarget
{
public:
	IRenderTarget();
	virtual ~IRenderTarget();
	virtual void BindRenderTargets()=0;
	virtual void UnBindRenderTargets()=0;
	virtual bool OnResize(int width, int height) = 0;
	virtual void ClearColor(YVector4 color)=0;
	virtual void ClearDepthStencil(float depth, uint8_t stencil)=0;
	int GetWidth() const;
	int GetHeight() const;
	PixelFormat GetColorFormat() const;
	PixelFormat GetDepthFormat() const;
protected:
	
	int width_ = 0;
	int height_ = 0;
	PixelFormat color_format_= PF_INVALID;
	PixelFormat depth_format_ = PF_INVALID;
};

class D3D11RenderTarget :public IRenderTarget
{
public:
	D3D11RenderTarget();
	~D3D11RenderTarget() override;
	void BindRenderTargets() override;
	void UnBindRenderTargets() override;
	bool CreateRenderTarget(PixelFormat color_format, PixelFormat depth_format, int width, int height);
	bool OnResize(int width, int height) override;
	ID3D11Texture2D* GetColorBuffer();
	ID3D11Texture2D* GetDepthBuffer();
	ID3D11ShaderResourceView* GetColorBufferRSV();
	ID3D11ShaderResourceView* GetDepthBufferRSV();
	ID3D11DepthStencilView* GetDSV();
	ID3D11RenderTargetView* GetRTV();

	virtual void ClearColor(YVector4 color) override;
	virtual void ClearDepthStencil(float depth, uint8_t stencil) override;

protected:
	TComPtr<ID3D11RenderTargetView> rtv_;
	TComPtr<ID3D11DepthStencilView> dsv_;
	TComPtr<ID3D11Texture2D> color_buffer_;
	TComPtr<ID3D11Texture2D> depth_buffer_;
	TComPtr<ID3D11ShaderResourceView> color_buffer_srv_;
	TComPtr<ID3D11ShaderResourceView> depth_buffer_srv_;

};