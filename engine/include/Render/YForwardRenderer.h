#pragma once
#include "YRenderInterface.h"
#include "RHI/DirectX11/D3D11RenderTarget.h"

class YForwardRenderer :public IRenderInterface
{
public:


	bool Init() override;


	bool Render(std::unique_ptr<YRenderScene> render_scene) override;


	bool Clearup() override;

protected:
	std::unique_ptr<YRenderScene> render_scene_;
	std::unique_ptr<D3D11RenderTarget> rts_;
};