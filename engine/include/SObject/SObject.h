#pragma once
#include <json.h>
#include <stdint.h>
#include <string>
#include "Engine/YLog.h"
#include "Engine/YReferenceCount.h"
class SObject:public YRefCountedObject
{
public:
	virtual ~SObject();
	SObject(const SObject&) = delete;
	SObject(SObject&&) = default;
	SObject& operator=(const SObject&) = delete;
	SObject& operator=(SObject&&) = default;
	virtual bool LoadFromPackage(const std::string & Path);
	virtual void SaveToPackage(const std::string & Path);
	virtual bool LoadFromJson(const Json::Value& RootJson);
	virtual bool PostLoadOp();
protected:
	SObject();
private:
	friend class SObjectManager;
};
