#include "Utility/YPath.h"
#include "Platform/Windows/YSysUtility.h"
const std::string YPath::separators = "\\/";
std::vector<std::string> YPath::GetFilePathsSeperate(const std::string& path)
{
	std::vector<std::string> file_pathes;
	std::string split_path = path;
	size_t pos = split_path.find_last_of(YPath::separators);
	while (pos != std::string::npos)
	{
		std::string parent_dir = split_path.substr(0, pos);
		file_pathes.push_back(split_path.substr(pos+1));
		split_path = parent_dir;
		pos = split_path.find_last_of(YPath::separators);
	}
	if (!split_path.empty())
	{
		file_pathes.push_back(split_path);
	}
	return std::vector<std::string>(file_pathes.rbegin(), file_pathes.rend());
}

std::vector<std::string> YPath::GetDirectoryPathsSeperate(const std::string directory)
{
	return YPath::GetFilePathsSeperate(directory);
}

std::string YPath::GetCleanFilename(const std::string& path)
{
	size_t pos = path.find_last_of(YPath::separators);
	if (pos == std::string::npos)
	{
		return path;
	}
	return path.substr(pos+1);
}

std::string YPath::GetBaseFilename(const std::string& in_path, bool remove_path /*= true*/)
{
	size_t pos = in_path.find_last_of('.');
	std::string path_no_extension;
	if (pos == std::string::npos)
	{
		path_no_extension = in_path;
	}
	else
	{
		path_no_extension = in_path.substr(0, pos);
	}
	if (remove_path)
	{
		return GetCleanFilename(path_no_extension);
	}
	return path_no_extension;
}

std::string YPath::GetPath(const std::string& in_path)
{
	size_t pos = in_path.find_last_of(YPath::separators);
	if (pos == std::string::npos)
	{
		return "";
	}
	return in_path.substr(0, pos);
}

std::string YPath::GetFileExtension(const std::string& path, bool include_dot)
{
	size_t pos = path.find_last_of('.');
	if (pos == std::string::npos)
	{
		return "";
	}
	else
	{
		return path.substr(include_dot ? pos : pos+1);
	}
}

std::string YPath::ChangeExtension(const std::string& InPath, const std::string& InNewExtension)
{
	return GetBaseFilename(InPath,false)+"."+InNewExtension;
}

bool YPath::FileExists(const std::string& InPath)
{
	return YSysUtility::FileExists(InPath);
}

bool YPath::DirectoryExists(const std::string& InPath)
{
	return 	YSysUtility::IsDirectoryExist(InPath);
}

bool YPath::IsRelative(const std::string& InPath)
{
	return false;
}

void YPath::NormalizeFilename(std::string& InPath)
{

}

std::string YPath::PathCombine(const std::string& a, const std::string& b)
{
	if (a.empty())
	{
		return b;
	}
	return a + directory_seperater + b;
}

void YPath::CreateDirectoryRecursive(const std::string& dir_path)
{
	return 	YSysUtility::CreateDirectoryRecursive(dir_path);
}

const char YPath::directory_seperater;

