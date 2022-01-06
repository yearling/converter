#include "Engine/YRenderScene.h"
#include "Engine/YWindowEventManger.h"
YRenderScene::YRenderScene()
{

}

YScene::YScene()
{

}

std::unique_ptr<YRenderScene> YScene::GenerateOneFrame() const
{
	std::unique_ptr<YRenderScene> one_frame = std::make_unique<YRenderScene>();
	//collect static mesh
	one_frame->primitive_elements_.reserve(static_meshes_components_.size());
	for (SStaticMeshComponent* mesh_component : static_meshes_components_)
	{
		PrimitiveElementProxy primitive_elem;
		primitive_elem.local_to_world_ = mesh_component->GetComponentTransform().ToMatrix();
		primitive_elem.mesh_ = mesh_component->GetMesh();
		one_frame->primitive_elements_.push_back(primitive_elem);
	}

	one_frame->dir_light_elements_.reserve(direct_light_components_.size());
	for (SDirectionLightComponent* dir_light_componet : direct_light_components_)
	{
		DirectLightElementProxy dir_light_elem;
		DirectLight* light =  dir_light_componet->dir_light_.get();
		dir_light_elem.light_color = light->GetLightColor();
		dir_light_elem.light_dir = light->GetLightdir();
		dir_light_elem.light_strength = light->GetLightStrength();
		one_frame->dir_light_elements_.push_back(dir_light_elem);
	}

	{
		one_frame->camera_element = std::move(perspective_camera_components_->camera_->GetProxy());
	}
	
	if (one_frame->dir_light_elements_.size() == 0)
	{
		WARNING_INFO("scene has no dir light");
	}

	if (!one_frame->camera_element)
	{
		ERROR_INFO("scene has no camera");
	}
	assert(one_frame->camera_element);
	


	return one_frame;
}

void YScene::RegisterEvents()
{
	g_windows_event_manager->windows_size_changed_funcs_.push_back([this](int x, int y) {
		this->OnViewPortChange(x, y);
	});
}

void YScene::OnViewPortChange(int width, int height)
{
	view_port_.width_ = width;
	view_port_.height_ = height;
	if (perspective_camera_components_)
	{
		perspective_camera_components_->OnViewPortChanged(view_port_);
	}
}
