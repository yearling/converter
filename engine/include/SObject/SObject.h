#pragma once
#include <json.h>
#include <stdint.h>
#include <string>
#include "Engine/YLog.h"
#include "Engine/YReferenceCount.h"
class MemoryFile;
class SObject :public YRefCountedObject
{
public:
	virtual ~SObject();
	SObject(const SObject&) = delete;
	SObject(SObject&&) = default;
	SObject& operator=(const SObject&) = delete;
	SObject& operator=(SObject&&) = default;
	virtual bool LoadFromJson(const Json::Value& RootJson);
	virtual bool LoadFromMemoryFile(std::unique_ptr<MemoryFile> mem_file);
	virtual void SaveToPackage(const std::string& Path);
	virtual bool PostLoadOp();
	virtual void Update(double deta_time);
	static const std::string  asset_extension;
	static const std::string  asset_extension_with_dot;
	static const std::string  json_extension;
	static const std::string  json_extension_with_dot;
	const std::string& GetName() const; 
	std::string GetAbsolutePath() const;
	SObject* GetParent() const;
	void SetParent(SObject* parent);
protected:
	SObject();
	explicit SObject(SObject* parent);
	virtual bool LoadFromPackage(const std::string& path);
	std::string GetAbsoluteCurDir() const;
	std::string name_;
	SObject* parent_ = nullptr;
private:
	friend class SObjectManager;
};
