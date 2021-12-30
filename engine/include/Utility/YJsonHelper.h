#pragma once
#include "Math/YVector.h"
#include "json.h"
#include "Math/YRotator.h"
struct YJsonHelper
{
	static bool ConvertJsonToVector2(const Json::Value& value,YVector2& v);
	static bool ConvertJsonToVector(const Json::Value& value,YVector& v);
	static bool ConvertJsonToVector4(const Json::Value& value,YVector4& v);
	static bool ConvertJsonToRotator(const Json::Value& value,YRotator& v);
	static bool LoadJsonFromFile(const std::string& path, Json::Value& root);
};