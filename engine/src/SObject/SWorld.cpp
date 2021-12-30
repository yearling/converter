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
				Actors.push_back(actor_ins);
			}
		}
		return true;
	}
	return true;
}

bool SWorld::PostLoadOp()
{
	bool bSuccess = true;
	for (TRefCountPtr<SActor>& Actor : Actors)
	{
		bSuccess &= Actor->PostLoadOp();
	}
	scene_ = std::make_unique<YScene>();
	for (TRefCountPtr<SActor>& actor : Actors)
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
	for (TRefCountPtr<SActor>& Actor : Actors)
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

void SWorld::SetCamera(CameraBase* camera)
{
	//todo
	camera_ = camera;
	scene_->camera_ = camera_;
}
