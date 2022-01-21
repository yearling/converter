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

bool SObject::LoadFromPackage(const std::string& Path)
{
	std::string asset_binary_path = Path + asset_extension_with_dot;
	std::string asset_json_path = Path + json_extension_with_dot;
	bool asset_binary_exist = YPath::FileExists(asset_binary_path);
	bool asset_json_exist = YPath::FileExists(asset_json_path);
	if (asset_binary_exist)
	{
		YFile asset_file(asset_binary_path, YFile::FileType(YFile::FT_BINARY | YFile::FT_Read));
		std::unique_ptr<MemoryFile> mem_file =  asset_file.ReadFile();
		if (!mem_file)
		{
			ERROR_INFO("load binary package ", Path ,"failed!, read file error");
			return false;
		}
		// todo reflection
		if (!LoadFromMemoryFile(std::move(mem_file)))
		{
			ERROR_INFO("load binary package ", Path, "failed!, serialize failed!");
			return false;
		}
	}
	else if (asset_json_exist)
	{
		Json::Value json_root;
		if (!YJsonHelper::LoadJsonFromFile(asset_json_path, json_root))
		{
			return false;
		}
		if (!LoadFromJson(json_root))
		{
			ERROR_INFO("load json package ", Path, "failed!, serialize failed!");
			return false;
		}
	}
	else
	{
		ERROR_INFO("load package ", Path ,"failed!, file not exist");
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

bool SObject::LoadFromMemoryFile(std::unique_ptr<MemoryFile> mem_file)
{
	return false;
}
