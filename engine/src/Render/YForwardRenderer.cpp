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
	ID3D11RenderTargetView* main_rtv = g_device->GetMainRTV();
	ID3D11DepthStencilView* main_dsv = g_device->GetMainDSV();
	g_device->SetRenderTarget(main_rtv, main_dsv);
	g_device->SetViewPort(0, 0, g_device->GetDeviceWidth(), g_device->GetDeviceHeight());
	ID3D11Device* raw_device = g_device->GetDevice();
	ID3D11DeviceContext* raw_dc = g_device->GetDC();
	float color[4] = { 0.f, 0.f, 0.f, 1.0f };
	raw_dc->ClearRenderTargetView(main_rtv, reinterpret_cast<float*>(color));
	raw_dc->ClearDepthStencilView(main_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

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
	return true;
}

bool YForwardRenderer::Clearup()
{
	return true;
}

