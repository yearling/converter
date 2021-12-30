#pragma once
#include "SObject.h"
#include "Engine/YReferenceCount.h"
#include<string>
#include "json.h"
#include "Math/YVector.h"
#include "Math/YRotator.h"
#include "Math/YTransform.h"
#include "json.h"
class YRenderScene;
class SActor;
class SSceneComponent;
class SComponent :public SObject
{
public:
	enum EComponentType
	{
		Base,
		SceneComponent,
		RenderComponent,
		StaticMeshComponent,
		LightComponenet,
		DirectLightComponent,
		ComNum
	};
	//SObject
	static constexpr  bool IsInstance() { return true; };

	//SComponent
	// Type
	explicit SComponent(EComponentType Type) :component_type_(Type) {}
	EComponentType GetComponentType() const;

	// Interface & Update
	virtual void RegisterToScene(class YScene* scene);

	// Parent
	SActor* GetParentActor()const;

	//Create Factory
	static TRefCountPtr<SSceneComponent> ComponentFactory(const Json::Value& RootJson);

protected:
	virtual bool LoadChildFromJson(const Json::Value& root_json);

protected:
	EComponentType component_type_;
	SActor* actor_parent_{ nullptr };
};

class SSceneComponent :public SComponent
{
public:
	SSceneComponent() :SComponent(EComponentType::SceneComponent) {}
	explicit SSceneComponent(EComponentType type);
	//todo 
	//FBoxSphereBounds Bounds;
	YVector local_translation_;
	YRotator local_rotation_;
	/**
	*	Non-uniform scaling of the component relative to its parent.
	*	Note that scaling is always applied in local space (no shearing etc)
	*/
	YVector local_scale_;


	// update 
	void Update(double deta_time) override;
	virtual void UpdateComponentToWorld();
	void SetComponentToWorld(const YTransform& NewComponentToWorld);
	const YTransform& GetComponentTransform() const;

	// load
	bool LoadFromJson(const Json::Value& RootJson)override;
	virtual bool PostLoadOp();
	virtual void RegisterToScene(class YScene* scene) override;
	virtual void OnTransformChange();
	// child
	std::vector<TRefCountPtr<SSceneComponent>>& GetChildComponents() { return child_components_; }
protected:
	void UpdateComponentToWorldWithParentRecursive();
	void PropagateTransformUpdate();
	virtual void UpdateBound();
	void UpdateChildTransforms();
protected:
	YTransform component_to_world_;
	bool is_component_to_world_update_ = false;
	SSceneComponent* parent_component_{ nullptr };
	std::vector<TRefCountPtr<SSceneComponent>> child_components_;
};

class SRenderComponent :public SSceneComponent
{
public:
	static constexpr  bool IsInstance()
	{
		return true;
	};
	SRenderComponent();
	explicit SRenderComponent(EComponentType Type);
	void Update(double deta_time) override;
	virtual void RegisterToScene(class YScene* scene);
	bool LoadFromJson(const Json::Value& RootJson)override;
	~SRenderComponent();

};

class SLightComponent :public SRenderComponent
{
public:
	SLightComponent();
	explicit SLightComponent(EComponentType type);
	void Update(double deta_time) override;
	bool LoadFromJson(const Json::Value& RootJson)override;
	void RegisterToScene(class YScene* scene) override;
	~SLightComponent();
};

class DirectLight;
class SDirectionLightComponent :public SLightComponent
{
public:
	SDirectionLightComponent();
	~SDirectionLightComponent();
	bool LoadFromJson(const Json::Value& RootJson) override;
	void RegisterToScene(class YScene* scene) override;
	virtual bool PostLoadOp();
	void OnTransformChange() override;
	void Update(double deta_time) override;
	std::unique_ptr<DirectLight> dir_light_;
};
