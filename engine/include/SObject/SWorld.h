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
protected:
	std::vector<TRefCountPtr<SActor>> Actors;
	std::unique_ptr<YScene> scene_;
	CameraBase* camera_;
};