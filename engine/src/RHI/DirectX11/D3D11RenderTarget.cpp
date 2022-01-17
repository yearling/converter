#include "RHI/DirectX11/D3D11RenderTarget.h"
#include "engine/YLog.h"
IRenderTarget::IRenderTarget()
{

}

IRenderTarget::~IRenderTarget()
{

}

int IRenderTarget::GetWidth() const
{
	return width_;
}

int IRenderTarget::GetHeight() const
{
	return height_;
}

PixelFormat IRenderTarget::GetColorFormat() const
{
	return color_format_;
}

PixelFormat IRenderTarget::GetDepthFormat() const
{
	return depth_format_;
}

D3D11RenderTarget::D3D11RenderTarget()
{

}

D3D11RenderTarget::~D3D11RenderTarget()
{

}

void D3D11RenderTarget::BindRenderTargets()
{
	ID3D11DeviceContext* dc = g_device->GetDC();
	if (rtv_ || dsv_)
	{
		g_device->SetRenderTarget(rtv_, dsv_);
	}
}

void D3D11RenderTarget::UnBindRenderTargets()
{
	g_device->SetRenderTarget(nullptr, nullptr);
}

bool D3D11RenderTarget::CreateRenderTarget(PixelFormat color_format, PixelFormat depth_format, int width, int height)
{
	assert(g_device);
	assert(width && height);
	width_ = width;
	height_ = height;
	color_format_ = color_format;
	depth_format_ = depth_format;
	if (color_buffer_)
	{
		color_buffer_ = nullptr;
	}
	if (!g_device->Create2DTextureRTV_SRV(width_, height_, DXGI_FORMAT_R8G8B8A8_UNORM, color_buffer_))
	{
		ERROR_INFO("Create texture2d for rt color failed, width ", width_, " height ", height_, "format: ", (int)DXGI_FORMAT_R8G8B8A8_UNORM);
		return false;
	}

	if (depth_buffer_)
	{
		depth_buffer_ = nullptr;
	}

	if (!g_device->Create2DTextureDSV_SRV(width_, height_, DXGI_FORMAT_R24G8_TYPELESS, depth_buffer_))
	{
		ERROR_INFO("Create texture2d for rt depth failed, width ", width_, " height ", height_, "format: ", (int)DXGI_FORMAT_R24G8_TYPELESS);
		return false;
	}

	if (rtv_)
	{
		rtv_ = nullptr;
	}

	if (!g_device->CreateRenderTargetView(DXGI_FORMAT_R8G8B8A8_UNORM, color_buffer_, rtv_))
	{
		ERROR_INFO("Create render target view failed !, width ", width_, " height ", height_, "format: ", (int)DXGI_FORMAT_R8G8B8A8_UNORM);
		return false;
	}

	if (dsv_)
	{
		dsv_ = nullptr;
	}

	if (!g_device->CreateDSVForTexture2D(DXGI_FORMAT_D24_UNORM_S8_UINT, depth_buffer_, dsv_))
	{
		ERROR_INFO("Create depth stencil view failed !, width ", width_, " height ", height_, "format: ", (int)DXGI_FORMAT_D24_UNORM_S8_UINT);
		return false;
	}

	if (color_buffer_srv_)
	{
		color_buffer_srv_ = nullptr;
	}

	if (!g_device->CreateSRVForTexture2D(DXGI_FORMAT_R8G8B8A8_UNORM, color_buffer_, color_buffer_srv_))
	{
		ERROR_INFO("Create shader resource view failed !, width ", width_, " height ", height_, "format: ", (int)DXGI_FORMAT_R8G8B8A8_UNORM);
		return false;
	}

	if (!g_device->CreateSRVForTexture2D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS, depth_buffer_, depth_buffer_srv_))
	{
		ERROR_INFO("Create shader resource view failed !, width ", width_, " height ", height_, "format: ", (int)DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
		return false;
	}

	return true;
}

bool D3D11RenderTarget::OnResize(int width, int height)
{
	if (width == width_ && height == height_)
	{
		return true;
	}
	return CreateRenderTarget(color_format_, depth_format_, width, height);
}

ID3D11Texture2D* D3D11RenderTarget::GetColorBuffer()
{
	return color_buffer_;
}

ID3D11Texture2D* D3D11RenderTarget::GetDepthBuffer()
{
	return depth_buffer_;
}

ID3D11ShaderResourceView* D3D11RenderTarget::GetColorBufferRSV()
{
	return color_buffer_srv_;
}

ID3D11ShaderResourceView* D3D11RenderTarget::GetDepthBufferRSV()
{
	return depth_buffer_srv_;
}

ID3D11DepthStencilView* D3D11RenderTarget::GetDSV()
{
	return dsv_;
}

ID3D11RenderTargetView* D3D11RenderTarget::GetRTV()
{
	return rtv_;
}

void D3D11RenderTarget::ClearColor(YVector4 color)
{
	g_device->GetDC()->ClearRenderTargetView(rtv_, &color.x);
}

void D3D11RenderTarget::ClearDepthStencil(float depth, uint8_t stencil)
{
	g_device->GetDC()->ClearDepthStencilView(dsv_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
}
