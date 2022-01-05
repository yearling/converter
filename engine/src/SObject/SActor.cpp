#include "SObject/SActor.h"
#include "SObject/SComponent.h"
#include "SObject/SObjectManager.h"
#include "Engine/YRenderScene.h"

SActor::SActor()
{
}

SActor::~SActor()
{

}


bool SActor::LoadFromJson(const Json::Value& root_json)
{
	if (root_json.isMember("root_component"))
	{
		//actors
		Json::Value root_component_value = root_json["root_component"];
		{
			TRefCountPtr new_root_component = SComponent::ComponentFactory(root_component_value);
			if (new_root_component)
			{
				root_component_ = new_root_component;
			}
		}
		if (root_json.isMember("id"))
		{
			id_ = root_json["id"].asInt();
		}
		
		if (root_json.isMember("name"))
		{
			name_ = root_json["name"].asString();
		}
		LOG_INFO("Actor ", name_, " load success");
		return true;
	}
	else
	{
		LOG_INFO("Actor load failed!");
		return false;
	}
}

bool SActor::PostLoadOp()
{
	if (root_component_)
	{
		return	root_component_->PostLoadOp();
	}
	return true;
}

void SActor::Update(double deta_time)
{
	if (root_component_)
	{
		root_component_->Update(deta_time);
	}
}

void SActor::RecursiveGetComponent(SComponent::EComponentType ComponentType, std::vector<SComponent*>& select_component)
{
	RecurisveGetTypeComponent<SComponent>(ComponentType, select_component);
}


void SActor::RegisterToScene(YScene* scene)
{
	if (root_component_)
	{
		root_component_->RegisterToScene(scene);
	}
}
