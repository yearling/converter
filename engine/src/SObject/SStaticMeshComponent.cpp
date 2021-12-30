#include "SObject/SStaticMeshComponent.h"
#include "Engine/YRenderScene.h"

SStaticMeshComponent::SStaticMeshComponent():
	SRenderComponent(EComponentType::StaticMeshComponent)
{

}

SStaticMeshComponent::~SStaticMeshComponent()
{

}

bool SStaticMeshComponent::LoadFromJson(const Json::Value& RootJson)
{
	SRenderComponent::LoadFromJson(RootJson);
	if (RootJson.isMember("model"))
	{
		std::string model_path = RootJson["model"].asString();
		static_mesh_ = std::make_unique<YStaticMesh>();
		if (static_mesh_->LoadV0(model_path))
		{
			LOG_INFO("Static mesh load success! ",model_path);
			return true;
		}
	}
	static_mesh_ = nullptr;
	LOG_INFO("Static mesh load failed! ");
	return false;
}

bool SStaticMeshComponent::PostLoadOp()
{
	SRenderComponent::PostLoadOp();
	if (static_mesh_)
	{
		if (static_mesh_->AllocGpuResource())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return true;
}

void SStaticMeshComponent::Update(double deta_time)
{
	SRenderComponent::Update(deta_time);
}

void SStaticMeshComponent::RegisterToScene(class YScene* scene)
{
	SRenderComponent::RegisterToScene(scene);
	scene->static_meshes_components_.insert(this);
}

YStaticMesh* SStaticMeshComponent::GetMesh()
{
	return static_mesh_.get();
}

