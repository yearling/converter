#pragma once
#include "YRenderInterface.h"

class YForwardRenderer :public IRenderInterface
{
public:


	bool Init() override;


	bool Render(std::unique_ptr<YRenderScene> render_scene) override;


	bool Clearup() override;

protected:
	std::unique_ptr<YRenderScene> render_scene_;
};