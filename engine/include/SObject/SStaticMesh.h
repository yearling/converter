#pragma once 
#include "SObject.h"
#include "Engine/YStaticMesh.h"
#include "SMaterial.h"
class SStaticMesh :public SObject
{
public:
	SStaticMesh();
	SStaticMesh(SObject* parent);
	~SStaticMesh() override;
	static constexpr  bool IsInstance() { return false; };
	virtual bool LoadFromJson(const Json::Value& RootJson) override;

	virtual void SaveToPackage(const std::string& Path) override;


	virtual bool PostLoadOp() override;


	virtual void Update(double deta_time) override;
	YStaticMesh* GetStaticMesh() const;
    
    friend class YFbxImporter;
protected:
	std::unique_ptr<YStaticMesh> static_mesh_;
	std::vector<TRefCountPtr<SMaterial>> materials_;

};