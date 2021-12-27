#pragma once
#include "SObject.h"
#include "Engine/YReferenceCount.h"
#include<string>
#include "json.h"
#include "Math/YVector.h"
#include "Math/YRotator.h"
#include "Math/YTransform.h"
#include "json.h"

class SComponent :public SObject
{
public:
	enum EComponentType
	{
		Base,
		SceneComponent,
		StaticMeshComponent,
		LightComponenet
	};
	explicit SComponent(EComponentType Type) :ComponentType(Type) {}
	static TRefCountPtr<SComponent> LoadFromNamedJson(const std::string& ComponentName, const Json::Value& RootJson);
	static constexpr  bool IsInstance()
	{
		return true;
	};
	EComponentType GetComponentType() const;
protected:
	EComponentType ComponentType;
};

class SSceneComponent :public SComponent
{
public:
	static constexpr  bool IsInstance()
	{
		return true;
	};
	SSceneComponent() :SComponent(EComponentType::SceneComponent) {}
	/** Current bounds of the component */
	//todo 
	//FBoxSphereBounds Bounds;
	/** Location of the component relative to its parent */
	YVector local_translation;

	/** Rotation of the component relative to its parent */
	YRotator local_rotation;
	/**
	*	Non-uniform scaling of the component relative to its parent.
	*	Note that scaling is always applied in local space (no shearing etc)
	*/
	YVector local_scale;
	SSceneComponent* ParentCompnent = nullptr;
	std::vector<TRefCountPtr<SSceneComponent>> ChildrenComponents;
	virtual void UpdateComponentToWorld();
	/** Sets the cached component to world directly. This should be used very rarely. */
	FORCEINLINE void SetComponentToWorld(const YTransform& NewComponentToWorld)
	{
		is_component_to_world_update_ = true;
		component_to_world = NewComponentToWorld;
	}


	/** Get the current component-to-world transform for this component */
	FORCEINLINE const YTransform& GetComponentTransform() const
	{
		return component_to_world;
	}
	void UpdateComponentToWorldWithParentRecursive();
	void PropagateTransformUpdate();
	virtual void UpdateBound();
	void UpdateChildTransforms();
	virtual bool LoadFromJson(const Json::Value& RootJson);
	virtual bool PostLoadOp();
private:
	/** Current transform of the component, relative to the world */
	YTransform component_to_world;
	bool is_component_to_world_update_ = false;
};
