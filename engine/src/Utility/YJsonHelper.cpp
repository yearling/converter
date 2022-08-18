#include "Utility/YJsonHelper.h"
#include "Engine/YFile.h"
#include <memory>
#include "Engine/YLog.h"
#include "Engine/YArchive.h"
bool YJsonHelper::ConvertJsonToVector2(const Json::Value& value, YVector2& v)
{
	if (value.isArray() && value.size() == 2)
	{
		v.x = value[0].asFloat();
		v.y = value[1].asFloat();
		return true;
	}
	return false;
}

bool YJsonHelper::ConvertJsonToVector(const Json::Value& value, YVector& v)
{
	if (value.isArray() && value.size() == 3)
	{
		v.x = value[0].asFloat();
		v.y = value[1].asFloat();
		v.z = value[2].asFloat();
		return true;
	}
	return false;
}

bool YJsonHelper::ConvertJsonToVector4(const Json::Value& value, YVector4& v)
{
	if (value.isArray() && value.size() == 4)
	{
		v.x = value[0].asFloat();
		v.y = value[1].asFloat();
		v.z = value[2].asFloat();
		v.z = value[3].asFloat();
		return true;
	}
	return false;
}

bool YJsonHelper::ConvertJsonToRotator(const Json::Value& value, YRotator& v)
{
	if (value.isArray() && value.size() == 3)
	{
		v.pitch = value[0].asFloat();
		v.yaw = value[1].asFloat();
		v.roll = value[2].asFloat();
		return true;
	}
	return false;
}

Json::Value YJsonHelper::ConvertVector2ToJson(const YVector2& v)
{
    Json::Value value;
    value.append(v.x);
    value.append(v.y);
    return value;
}

Json::Value YJsonHelper::ConvertVectorToJson(const YVector& v)
{
    Json::Value value;
    value.append(v.x);
    value.append(v.y);
    value.append(v.z);
    return value;
}

Json::Value YJsonHelper::ConvertVector4ToJson(const YVector4& v)
{
    Json::Value value;
    value.append(v.x);
    value.append(v.y);
    value.append(v.z);
    value.append(v.w);
    return value;
}

Json::Value YJsonHelper::ConvertRotatorToJson(const YRotator& v)
{
    Json::Value value;
    value.append(v.pitch);
    value.append(v.yaw);
    value.append(v.roll);
    return value;
}

bool YJsonHelper::LoadJsonFromFile(const std::string& path, Json::Value& root)
{
	YFile json_file(path, YFile::FileType(YFile::FT_TXT | YFile::FT_Read));
	std::unique_ptr<MemoryFile> mem_file = json_file.ReadFile();
	if (!mem_file)
	{
		ERROR_INFO("load json package ", path, "failed!, read file error");
		return false;
	}
	Json::Reader json_reader;
	if (!json_reader.parse((const char*)mem_file->GetData(), (const char*)(mem_file->GetData() + mem_file->GetSize()), root, true))
	{
		ERROR_INFO("load json package ", path, "failed!, json parse failed, reason ", json_reader.getFormattedErrorMessages().c_str());
		return false;
	}
	return true;
}

bool YJsonHelper::SaveJsonToFile(const std::string& path, Json::Value& root)
{
    YFile file_to_write(path, YFile::FileType(YFile::FT_TXT | YFile::FT_Write));
    MemoryFile mem_file(MemoryFile::FT_Write);
    std::string style_string = root.toStyledString();
    mem_file.Serialize(style_string.data(),style_string.size());
    if (!file_to_write.WriteFile(&mem_file, true))
    {
        ERROR_INFO("save json ", path , " failed!");
        return false;
    }
    return true;
}

