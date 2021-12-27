#pragma once
#include "SObject/SObject.h"
#include "SObject/SComponent.h"
#include "Engine/YLog.h"
#include "json.h"
#include <vector>
class SActor :public SObject
{
public:
	SActor();
	virtual ~SActor();
	static constexpr  bool IsInstance() { return true; }
	//virtual bool LoadFromJson(const TSharedPtr<FJsonObject>&RootJson);
	virtual bool LoadFromJson(Json::Value& RootJson);
	virtual bool PostLoadOp();

	template<typename T>
	void RecursiveGetTypeComponent(SComponent* CurrentComponent, SComponent::EComponentType ComponentType, std::vector<T*>& InOutSelectComponents)
	{
		if (!CurrentComponent)
			return;

		if (CurrentComponent->GetComponentType() == ComponentType)
		{
			InOutSelectComponents.AddUnique(dynamic_cast<T*>(CurrentComponent));
		}

		SSceneComponent* SceneComponent = dynamic_cast<SSceneComponent*>(CurrentComponent);
		if (SceneComponent)
		{
			for (TRefCountPtr<SSceneComponent>& Child : SceneComponent->ChildrenComponents)
			{
				RecursiveGetTypeComponent(Child.GetReference(), ComponentType, InOutSelectComponents);
			}
		}
	}
	template<typename T>
	void RecurisveGetTypeComponent(SComponent::EComponentType ComponentType, std::vector<T*>& InOutSelectComponents)
	{
		for (TRefCountPtr<SComponent>& Component : Components)
		{
			RecursiveGetTypeComponent(Component.GetReference(), ComponentType, InOutSelectComponents);
		}
	}

public:
	std::vector<TRefCountPtr<SComponent>> Components;
};
