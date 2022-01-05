#pragma once
#include <string>
#include <vector>
#include "SObject/SObject.h"
#include "SObject/SActor.h"
#include "Engine/YRenderScene.h"

class SWorld :public SObject
{
public:
	SWorld();
	virtual ~SWorld();
	static constexpr bool IsInstance() { return false; };
	virtual bool LoadFromJson(const Json::Value& RootJson);
	virtual bool PostLoadOp();
	std::unique_ptr<YRenderScene> GenerateRenderScene();
	void Update(double deta_time) override;
	static SWorld* GetWorld() ;
	static void SetWorld(TRefCountPtr<SWorld>& world);
	//todo load camera

	void SetCamera(CameraBase* camera);

	inline std::vector<TRefCountPtr<SActor>> GetAllActors() { return actors_; }
	std::vector<TRefCountPtr<SActor>> GetAllActorsWithComponent(const std::vector<SComponent::EComponentType> component_types)const;
protected:
	std::vector<TRefCountPtr<SActor>> actors_;
	std::unique_ptr<YScene> scene_;
	CameraBase* camera_;
};