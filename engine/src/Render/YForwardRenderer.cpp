#include "Render/YForwardRenderer.h"
#include "Engine/YCanvas.h"
#include "RHI/DirectX11/D3D11Device.h"
#include "Render/YRenderInterface.h"
#include "Engine/YCanvasUtility.h"


bool YForwardRenderer::Init()
{
	return true;
}

bool YForwardRenderer::Render(std::unique_ptr<YRenderScene> render_scene)
{
	render_scene_ = std::move(render_scene);
	const int scene_width = render_scene_->view_port_.GetWidth();
	const int scene_height = render_scene_->view_port_.GetHeight();
	if (!rts_)
	{
		rts_ = std::make_unique<D3D11RenderTarget>();
		if (!rts_->CreateRenderTarget(PF_R8G8B8A8, PF_D24S8, scene_width, scene_height))
		{
			rts_ = nullptr;
			return false;
		}
	}

	if (!rts_->OnResize(scene_width, scene_height))
	{
		ERROR_INFO("Forward renderer create RT failed!");
	}

	g_device->SetViewPort(render_scene_->view_port_.left_top_x, render_scene_->view_port_.left_top_y, scene_width, scene_height);

	rts_->BindRenderTargets();
	rts_->ClearColor(YVector4(0.0f, 0.0f, 0.0f, 1.0f));
	rts_->ClearDepthStencil(1.0, 0);


	//
	//ID3D11RenderTargetView* main_rtv = g_device->GetMainRTV();
	//ID3D11DepthStencilView* main_dsv = g_device->GetMainDSV();
	//g_device->SetRenderTarget(main_rtv, main_dsv);
	//ID3D11Device* raw_device = g_device->GetDevice();
	//ID3D11DeviceContext* raw_dc = g_device->GetDC();
	//float color[4] = { 0.f, 0.f, 0.f, 1.0f };
	//raw_dc->ClearRenderTargetView(main_rtv, reinterpret_cast<float*>(color));
	//raw_dc->ClearDepthStencilView(main_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);


	RenderParam render_param;
	render_param.camera_proxy = render_scene_->camera_element.get();
	render_param.dir_lights_proxy = &render_scene_->dir_light_elements_;
	render_param.delta_time = (float) render_scene_->deta_time;
	render_param.game_time = (float) render_scene_->game_time;
	
	for (PrimitiveElementProxy& ele : render_scene_->primitive_elements_)
	{
		render_param.local_to_world_ = ele.local_to_world_;
		ele.mesh_->Render(&render_param);
	}


	g_Canvas->Update();
	g_Canvas->Render(&render_param);


	// copy back to main rtv
	//TComPtr<ID3D11Texture2D> swap_chain_buffer;
	//swap_chain_buffer.Attach(g_device->GetSwapChainColorBuffer());
	//g_device->GetDC()->CopyResource( swap_chain_buffer,rts_->GetColorBuffer());
	
	ID3D11RenderTargetView* main_rtv = g_device->GetMainRTV();
	ID3D11DepthStencilView* main_dsv = g_device->GetMainDSV();
	g_device->SetRenderTarget(main_rtv, main_dsv);
	YVector4 clear_color = YVector4(0.0f, 0.0f, 0.0f, 1.0f);
	g_device->GetDC()->ClearRenderTargetView(main_rtv, &clear_color.x);
	g_device->GetDC()->ClearDepthStencilView(main_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
	return true;
}

bool YForwardRenderer::Clearup()
{
	return true;
}

YForwardRenderer::~YForwardRenderer()
{
	Clearup();
}

D3D11RenderTarget* YForwardRenderer::GetRTs() const
{
	return rts_.get();
}

