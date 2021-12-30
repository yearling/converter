#include "Engine/YRenderScene.h"
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
	
	if (one_frame->dir_light_elements_.size() == 0)
	{
		WARNING_INFO("scene has no dir light");
	}
	assert(camera_);
	
	one_frame->camera_element = std::move(camera_->GetProxy());


	return one_frame;
}
