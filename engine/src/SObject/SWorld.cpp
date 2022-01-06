#include "SObject/SWorld.h"
#include "SObject/SObjectManager.h"
#include "json.h"


SWorld::SWorld()
{

}

SWorld::~SWorld()
{

}

bool SWorld::LoadFromJson(const Json::Value& RootJson)
{
	//actors
	const Json::Value& actors = RootJson["actors"];
	if (actors.isArray())
	{
		for (int actor_index = 0; actor_index < (int)actors.size(); ++actor_index)
		{
			const Json::Value actor_json = actors[actor_index];
			TRefCountPtr<SActor> actor_ins = SObjectManager::ConstructInstance<SActor>();
			if (!actor_ins->LoadFromJson(actor_json))
			{
				ERROR_INFO("load SActor failed! json file \n", actor_json.toStyledString());
			}
			else
			{
				actors_.push_back(actor_ins);
			}
		}
		return true;
	}
	return true;
}

bool SWorld::PostLoadOp()
{
	bool bSuccess = true;
	for (TRefCountPtr<SActor>& Actor : actors_)
	{
		bSuccess &= Actor->PostLoadOp();
	}
	scene_ = std::make_unique<YScene>();
	for (TRefCountPtr<SActor>& actor : actors_)
	{
		actor->RegisterToScene(scene_.get());
	}
	return bSuccess;
}

std::unique_ptr<YRenderScene> SWorld::GenerateRenderScene()
{
	return scene_->GenerateOneFrame();
}


void SWorld::Update(double deta_time)
{
	// animation system update

	// physical system update

	// physical system update apply to animation
	for (TRefCountPtr<SActor>& Actor : actors_)
	{
		Actor->Update(deta_time);
	}


}
TRefCountPtr<SWorld> g_world;
SWorld* SWorld::GetWorld()
{
	return g_world;
}

void SWorld::SetWorld(TRefCountPtr<SWorld>& world)
{
	g_world = world;
}

std::vector<TRefCountPtr<SActor>> SWorld::GetAllActorsWithComponent(const std::vector<SComponent::EComponentType> component_types) const
{
	std::vector<TRefCountPtr<SActor>> tmp;
	std::vector<SComponent*> tmp_component;
	for (const TRefCountPtr<SActor>& actor : actors_)
	{
		bool has_component = true;
		for (SComponent::EComponentType component_type : component_types)
		{
			tmp_component.clear();
			actor->RecursiveGetComponent(component_type, tmp_component);
			if (tmp_component.empty())
			{
				has_component = false;
				break;
			}
		}
		if (has_component)
		{
			tmp.push_back(actor);
		}
	}
	return tmp;
}
