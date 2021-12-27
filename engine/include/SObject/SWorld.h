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
	virtual bool LoadFromPackage(const std::string& Path);
	virtual bool PostLoadOp();
	void UpdateToScene();
protected:
	std::vector<TRefCountPtr<SActor>> Actors;
};