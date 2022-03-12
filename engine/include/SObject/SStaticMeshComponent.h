#pragma once
#include "SObject/SComponent.h"
#include "Engine/YStaticMesh.h"
#include <memory>
#include "SObject/SMaterial.h"
#include "SStaticMesh.h"
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
	void UpdateBound() override;
protected:
	//std::unique_ptr<YStaticMesh> static_mesh_;
	//std::vector<TRefCountPtr<SMaterial>> materials_;
	TRefCountPtr<SStaticMesh> static_mesh_;
	float move_dir_neg = 1.0;
};