#include "SObject/SComponent.h"
#include "SObject/SObjectManager.h"

void SSceneComponent::UpdateComponentToWorld()
{
	UpdateComponentToWorldWithParentRecursive();
}

void SSceneComponent::UpdateComponentToWorldWithParentRecursive()
{
	if (ParentCompnent && !ParentCompnent->is_component_to_world_update_)
	{
		ParentCompnent->UpdateComponentToWorld();
		if (is_component_to_world_update_)
		{
			return;
		}
	}
	is_component_to_world_update_ = true;
	YTransform NewTransform;
	{
		YTransform RelativeTransform(local_translation, local_rotation.ToQuat(), local_scale);
		if (ParentCompnent)
		{
			NewTransform = RelativeTransform * ParentCompnent->GetComponentTransform();
		}
		else
		{
			NewTransform = RelativeTransform;
		}
	}

	component_to_world = NewTransform;
	PropagateTransformUpdate();
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
	for (SSceneComponent* child : ChildrenComponents)
	{
		child->UpdateComponentToWorld();
	}
}


bool SSceneComponent::LoadFromJson(const Json::Value& RootJson)
{
	return true;
}

bool SSceneComponent::PostLoadOp()
{
	UpdateComponentToWorld();
	bool bChildSuccess = true;
	for (TRefCountPtr<SSceneComponent>& Child : ChildrenComponents)
	{
		bChildSuccess &= Child->PostLoadOp();
	}
	return bChildSuccess;
}

//Load Utilities


//TRefCountPtr<SComponent> SComponent::LoadFromNamedJson(const FString& ComponentName, const TSharedPtr<FJsonObject>& RootJson)
//{
//	if (ComponentName == TEXT("SceneComponent"))
//	{
//		TRefCountPtr<SSceneComponent>  SceneComponent = SObjectManager::ConstructInstance<SSceneComponent>();
//		if (!SceneComponent->LoadFromJson(RootJson))
//		{
//			return TRefCountPtr<SComponent>(nullptr);
//		}
//		return TRefCountPtr<SComponent>(SceneComponent.GetReference());
//	}
//	else if (ComponentName == TEXT("StaticMeshComponent"))
//	{
//		//TRefCountPtr<SStaticMeshComponent>  StaticMeshComponent = SObjectManager::ConstructInstance<SStaticMeshComponent>();
//		//if (!StaticMeshComponent->LoadFromJson(RootJson))
//		//{
//			//return TRefCountPtr<SComponent>(nullptr);
//		//}
//		//return TRefCountPtr<SComponent>(StaticMeshComponent.GetReference());
//	}
//	return TRefCountPtr<SComponent>(nullptr);
//}

TRefCountPtr<SComponent> SComponent::LoadFromNamedJson(const std::string& ComponentName, const Json::Value& RootJson)
{
	return nullptr;
}

SComponent::EComponentType SComponent::GetComponentType() const
{
	return ComponentType;
}
