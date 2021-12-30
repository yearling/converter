#pragma once
#include "Engine/YRenderScene.h"
#include <memory>

class RenderParam
{
public:
	CameraElementProxy* camera_proxy;
	//todo 
	std::vector<DirectLightElementProxy>* dir_lights_proxy;
	float delta_time;
	float game_time;
	YMatrix local_to_world_;
};
class IRenderInterface
{
public:
	IRenderInterface();
	virtual ~IRenderInterface();
	virtual bool Init()=0;
	virtual bool Render(std::unique_ptr<YRenderScene> render_scene)=0;
	virtual bool Clearup()=0;
};