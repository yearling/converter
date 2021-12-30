#pragma once
#include "SObject/SComponent.h"
#include "Engine/YStaticMesh.h"
#include <memory>
class SStaticMeshComponent:public SRenderComponent
{
public:
	SStaticMeshComponent();
	~SStaticMeshComponent();
	bool LoadFromJson(const Json::Value& RootJson) override;
	bool PostLoadOp() override;
	void Update(double deta_time) override;
	void RegisterToScene(class YScene* scene) override;
	YStaticMesh* GetMesh();
protected:
	std::unique_ptr<YStaticMesh> static_mesh_;
};