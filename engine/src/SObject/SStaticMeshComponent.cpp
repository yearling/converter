#include "SObject/SStaticMeshComponent.h"
#include "Engine/YRenderScene.h"
#include "SObject/SObjectManager.h"

SStaticMeshComponent::SStaticMeshComponent() :
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
		TRefCountPtr<SStaticMesh> static_mesh = SObjectManager::ConstructFromPackage<SStaticMesh>(model_path, this);
		if (!static_mesh)
		{
			ERROR_INFO("load static mesh ", model_path, "failed!");
			return false;
		}
		static_mesh_ = static_mesh;
	}
	else
	{
		ERROR_INFO("SstaticMeshComponent do not have a model");
		return false;
	}
	return true;
}

bool SStaticMeshComponent::PostLoadOp()
{
	SRenderComponent::PostLoadOp();
	if (static_mesh_)
	{
		static_mesh_->PostLoadOp();
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
	return static_mesh_->GetStaticMesh();
}

void SStaticMeshComponent::UpdateBound()
{
	if (static_mesh_)
	{
		bounds_ = static_mesh_->GetStaticMesh()->raw_meshes[0].aabb.TransformBy(component_to_world_);
	}
}

