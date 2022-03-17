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
	const float move_speed = 20.0f;
	YVector move_dir = YVector(1.0f, 0.0f, 0.0f).GetSafeNormal();
	YVector delta_dir = move_dir * deta_time * move_speed;
	YVector local_trans = GetLocalTranslation();
	if(local_trans.x > 30.f )
	{ 
		move_dir_neg = -1.0f;
		local_trans.x = 30.0f;
	}
	else if (local_trans.x < -30.f)
	{
		move_dir_neg = 1.0f;
		local_trans.x = -30.0f;
	}

	local_trans = local_trans + (move_dir_neg * delta_dir);

	SetLocalTranslation(local_trans);

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

