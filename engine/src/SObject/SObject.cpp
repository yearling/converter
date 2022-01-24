#include "SObject/SObject.h"
#include "Utility/YPath.h"
#include "Engine/YLog.h"
#include "Engine/YFile.h"
#include "reader.h"
#include "Utility/YJsonHelper.h"

SObject::~SObject()
{

}

SObject::SObject()
{

}

SObject::SObject(SObject* parent)
	:parent_(parent)
{

}

bool SObject::LoadFromPackage(const std::string& path)
{
	name_ = path;
	std::string asset_binary_asset_path = name_ + asset_extension_with_dot;
	std::string asset_json_asset_path = name_ + json_extension_with_dot;
	bool asset_binary_exist = YPath::IsAssetExist(asset_binary_asset_path);
	bool asset_json_exist = YPath::IsAssetExist(asset_json_asset_path);
	if (asset_binary_exist)
	{
		const std::string asset_binaray_file_path = YPath::ConverAssetPathToFilePath(asset_binary_asset_path);
		YFile asset_file(asset_binaray_file_path, YFile::FileType(YFile::FT_BINARY | YFile::FT_Read));
		std::unique_ptr<MemoryFile> mem_file = asset_file.ReadFile();
		if (!mem_file)
		{
			ERROR_INFO("load binary package ", name_, "failed!, read file error");
			return false;
		}
		// todo reflection
		if (!LoadFromMemoryFile(std::move(mem_file)))
		{
			ERROR_INFO("load binary package ", name_, "failed!, serialize failed!");
			return false;
		}
	}
	else if (asset_json_exist)
	{
		Json::Value json_root;
		const std::string asset_json_file_path = YPath::ConverAssetPathToFilePath(asset_json_asset_path);
		if (!YJsonHelper::LoadJsonFromFile(asset_json_file_path, json_root))
		{
			return false;
		}
		if (!LoadFromJson(json_root))
		{
			ERROR_INFO("load json package ", name_, "failed!, serialize failed!");
			return false;
		}
	}
	else
	{
		ERROR_INFO("load package ", name_, "failed!, file not exist");
		return false;
	}

	return true;
}

void SObject::SaveToPackage(const std::string& Path)
{

}

bool SObject::LoadFromJson(const Json::Value& RootJson)
{
	return true;
}

bool SObject::PostLoadOp()
{
	return true;
}

void SObject::Update(double deta_time)
{
}

const std::string SObject::asset_extension = "yasset";

const std::string SObject::asset_extension_with_dot = ".yasset";

const std::string SObject::json_extension = "json";

const std::string SObject::json_extension_with_dot = ".json";

const std::string& SObject::GetName() const
{
	return name_;
}

std::string SObject::GetAbsoluteCurDir() const
{
	if (YPath::IsAssetAbsolutePath(name_))
	{
		return YPath::GetPath(name_);
	}
	else
	{
		if (!parent_)
		{
			ERROR_INFO("relative path have no parent: ", name_);
			return "";
		}
		
		return YPath::PathCombine(GetAbsoluteCurDir(), name_);
	}
}


std::string SObject::GetAbsolutePath() const
{
	if (YPath::IsAssetAbsolutePath(name_))
	{
		return name_;
	}
	else
	{
		return YPath::PathCombine(parent_->GetAbsoluteCurDir(), name_);
	}
}

SObject* SObject::GetParent() const
{
	return parent_;
}

void SObject::SetParent(SObject* parent)
{
	parent_ = parent;
}

bool SObject::LoadFromMemoryFile(std::unique_ptr<MemoryFile> mem_file)
{
	return false;
}
