#include "SObject/SComponent.h"
#include "SObject/SObjectManager.h"
#include <functional>
#include "Engine/YReferenceCount.h"
#include "SObject/SStaticMeshComponent.h"
#include "Engine/YLight.h"
#include "Utility/YJsonHelper.h"
#include "Engine/YRenderScene.h"

SSceneComponent::SSceneComponent(EComponentType type)
	:SComponent(type)
{

}

void SSceneComponent::UpdateComponentToWorld()
{
	UpdateComponentToWorldWithParentRecursive();
}

void SSceneComponent::SetComponentToWorld(const YTransform& NewComponentToWorld)
{
	is_component_to_world_update_ = false;
	component_to_world_ = NewComponentToWorld;
}

const YTransform& SSceneComponent::GetComponentTransform() const
{
	return component_to_world_;
}

void SSceneComponent::UpdateComponentToWorldWithParentRecursive()
{
	if (parent_component_ && !parent_component_->is_component_to_world_update_)
	{
		parent_component_->UpdateComponentToWorld();
	}

	if (is_component_to_world_update_)
	{
		return;
	}

	is_component_to_world_update_ = true;
	YTransform NewTransform;
	{
		YTransform RelativeTransform(local_translation_, local_rotation_.ToQuat(), local_scale_);
		if (parent_component_)
		{
			NewTransform = RelativeTransform * parent_component_->GetComponentTransform();
		}
		else
		{
			NewTransform = RelativeTransform;
		}
	}

	component_to_world_ = NewTransform;
	PropagateTransformUpdate();
	OnTransformChange();
}

void SSceneComponent::PropagateTransformUpdate()
{
	UpdateBound();
	UpdateChildTransforms();
}

void SSceneComponent::UpdateBound()
{
	//Bounds.BoxExtent = FVector(0, 0, 0);
	//Bounds.Origin = component_to_world.GetTranslation();
	//Bounds.SphereRadius = 0.0;
}

void SSceneComponent::UpdateChildTransforms()
{
	for (TRefCountPtr<SSceneComponent>& child : child_components_)
	{
		child->UpdateComponentToWorld();
	}
}


bool SSceneComponent::LoadFromJson(const Json::Value& root_json)
{
	if (root_json.isMember("local_translation"))
	{
		YJsonHelper::ConvertJsonToVector(root_json["local_translation"],local_translation_);
	}

	if (root_json.isMember("local_rotation"))
	{
	 	YJsonHelper::ConvertJsonToRotator(root_json["local_rotation"], local_rotation_);
	}

	if (root_json.isMember("local_scale"))
	{
		YJsonHelper::ConvertJsonToVector(root_json["local_scale"], local_scale_);
	}

	if (root_json.isMember("children"))
	{
		for (int i = 0; i <(int) root_json["children"].size(); ++i)
		{
			const Json::Value& value = root_json["children"][i];
			TRefCountPtr<SSceneComponent> new_component = SComponent::ComponentFactory(value);
			if (new_component)
			{
				child_components_.push_back(new_component);
				new_component->parent_component_ = this;
			}
		}
	}
	return true;
}

bool SSceneComponent::PostLoadOp()
{
	UpdateComponentToWorld();
	bool bChildSuccess = true;
	for (TRefCountPtr<SSceneComponent>& Child : child_components_)
	{
		bChildSuccess &= Child->PostLoadOp();
	}
	return bChildSuccess;
}

void SSceneComponent::Update(double deta_time)
{
	UpdateComponentToWorld();
}

void SSceneComponent::RegisterToScene(YScene* scene)
{

}

void SSceneComponent::OnTransformChange()
{

}

std::unordered_map<std::string, std::function<SSceneComponent*()> > register_component_map =
{
	{"StaticMesh",[]() { return new SStaticMeshComponent(); }},
	{"DirectionLight",[]() { return new SDirectionLightComponent(); }}
};
TRefCountPtr<SSceneComponent> SComponent::ComponentFactory(const Json::Value& RootJson)
{
	if (RootJson.isMember("type"))
	{
		std::string compoennt_type_name = RootJson["type"].asString();
		if (!register_component_map.count(compoennt_type_name))
		{
			return nullptr;
		}
		TRefCountPtr<SSceneComponent> new_component = register_component_map[compoennt_type_name]();
		if (!new_component->LoadFromJson(RootJson))
		{
			return nullptr;
		}
		return new_component;
	}
	else
	{
		return nullptr;
	}
}

SComponent::EComponentType SComponent::GetComponentType() const
{
	return component_type_;
}

void SComponent::RegisterToScene(YScene* scene)
{
	
}

SActor* SComponent::GetParentActor() const
{
	 return actor_parent_; 
}

bool SComponent::LoadChildFromJson(const Json::Value& root_json)
{

	return true;
}

SRenderComponent::SRenderComponent():SSceneComponent(EComponentType::RenderComponent)
{

}

SRenderComponent::SRenderComponent(EComponentType Type): SSceneComponent(Type)
{

}

void SRenderComponent::Update(double deta_time)
{
	SSceneComponent::Update(deta_time);
}

void SRenderComponent::RegisterToScene(YScene* scene)
{
	assert(scene);
	for (auto& child : child_components_)
	{
		child->RegisterToScene(scene);
	}
}

bool SRenderComponent::LoadFromJson(const Json::Value& RootJson)
{
	if (!SSceneComponent::LoadFromJson(RootJson))
	{
		return false;
	}
	return true;
}

SRenderComponent::~SRenderComponent()
{

}

SLightComponent::SLightComponent():SRenderComponent(EComponentType::LightComponenet)
{

}

SLightComponent::SLightComponent(EComponentType type):SRenderComponent(type)
{

}

void SLightComponent::Update(double deta_time)
{
	SRenderComponent::Update(deta_time);
}

bool SLightComponent::LoadFromJson(const Json::Value& RootJson)
{
	if (!SRenderComponent::LoadFromJson(RootJson))
	{
		return false;
	}
	return true;
}

void SLightComponent::RegisterToScene(YScene* scene)
{
	SRenderComponent::RegisterToScene(scene);
}

SLightComponent::~SLightComponent()
{

}

SDirectionLightComponent::SDirectionLightComponent():SLightComponent(EComponentType::DirectLightComponent)
{

}

SDirectionLightComponent::~SDirectionLightComponent()
{

}

bool SDirectionLightComponent::LoadFromJson(const Json::Value& RootJson)
{
	if (!SLightComponent::LoadFromJson(RootJson))
	{
		return false;
	}

	dir_light_ = std::make_unique<DirectLight>();
	if (RootJson.isMember("strength"))
	{
		float light_strenth = RootJson["strength"].asFloat();
		dir_light_->SetLightStrength(light_strenth);
	}

	if (RootJson.isMember("color"))
	{
		YVector4 color = YVector4::zero_vector;
		if (YJsonHelper::ConvertJsonToVector4(RootJson["color"], color))
		{
			dir_light_->SetLightColor(color);
		}
	}
	LOG_INFO("Dir light load success! ");

	return true;
}

void SDirectionLightComponent::RegisterToScene(YScene* scene)
{
	scene->direct_light_components_.insert(this);
}

bool SDirectionLightComponent::PostLoadOp()
{
	return true;
}

void SDirectionLightComponent::OnTransformChange()
{
	SSceneComponent::UpdateComponentToWorld();
	if (dir_light_)
	{
		dir_light_->SetLightDir(component_to_world_.ToMatrix().TransformVector(YVector::forward_vector).GetSafeNormal());
	}
}

void SDirectionLightComponent::Update(double deta_time)
{
	SLightComponent::Update(deta_time);
}
