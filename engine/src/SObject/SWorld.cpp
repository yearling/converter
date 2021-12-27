#include "SObject/SWorld.h"
#include "SObject/SObjectManager.h"


SWorld::SWorld()
{

}

SWorld::~SWorld()
{

}

bool SWorld::LoadFromPackage(const std::string& Path)
{
	return true;
}

bool SWorld::PostLoadOp()
{
	bool bSuccess = true;
	for (TRefCountPtr<SActor>& Actor : Actors)
	{
		bSuccess &= Actor->PostLoadOp();
	}
	return bSuccess;
}

void SWorld::UpdateToScene()
{
	//TArray<SStaticMeshComponent*> StaticMeshComponents;
	//for (TRefCountPtr<SActor> &Actor : Actors)
	//{
	//	Actor->RecurisveGetTypeComponent(SComponent::StaticMeshComponent, StaticMeshComponents);
	//}
	//for (SStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	//{
	//	CurrentScene->RegisterToScene(StaticMeshComponent);
	//}
}
