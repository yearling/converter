#pragma once
#include "SObject/SObject.h"
#include "SObject/SComponent.h"
#include "Engine/YLog.h"
#include "json.h"
#include <vector>
#include <string>

class SActor :public SObject
{
public:
	SActor();
	SActor(SObject* parent);
	virtual ~SActor();
	static constexpr  bool IsInstance() { return true; }
	//virtual bool LoadFromJson(const TSharedPtr<FJsonObject>&RootJson);
	virtual bool LoadFromJson(const Json::Value& RootJson);
	virtual bool PostLoadOp();
	void Update(double deta_time) override;
	template<typename T>
	void RecursiveGetTypeComponent(SComponent* CurrentComponent, SComponent::EComponentType ComponentType, std::vector<T*>& InOutSelectComponents)
	{
		if (!CurrentComponent)
			return;

		if (CurrentComponent->GetComponentType() == ComponentType)
		{
			InOutSelectComponents.push_back(dynamic_cast<T*>(CurrentComponent));
		}

		SSceneComponent* SceneComponent = dynamic_cast<SSceneComponent*>(CurrentComponent);
		if (SceneComponent)
		{
			for (TRefCountPtr<SSceneComponent>& Child : SceneComponent->GetChildComponents())
			{
				RecursiveGetTypeComponent(Child.GetReference(), ComponentType, InOutSelectComponents);
			}
		}
	}
	template<typename T>
	void RecurisveGetTypeComponent(SComponent::EComponentType ComponentType, std::vector<T*>& InOutSelectComponents)
	{
		if (root_component_->GetComponentType() == ComponentType)
		{
			InOutSelectComponents.push_back(dynamic_cast<T*>(root_component_.GetReference()));
		}
		std::vector<TRefCountPtr<SSceneComponent>> children = root_component_->GetChildComponents();
		for (TRefCountPtr<SSceneComponent>& Component : children)
		{
			RecursiveGetTypeComponent(Component.GetReference(), ComponentType, InOutSelectComponents);
		}
	}

	void RecursiveGetComponent(SComponent::EComponentType ComponentType, std::vector<SComponent*>& select_component);
	void RegisterToScene(class YScene* render_scene);
protected:
	TRefCountPtr<SSceneComponent> root_component_;
	int id_ = -1;
	std::string name_;
};
