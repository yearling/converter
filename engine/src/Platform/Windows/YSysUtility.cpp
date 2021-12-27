#include "Platform/Windows/YSysUtility.h"
#include <stdio.h>
#include <locale.h>
#include <iostream>
#include <vector>
#include "Utility/YPath.h"
#include "Engine/YLog.h"
void YSysUtility::AllocWindowsConsole()
{
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONIN$", "w+t", stdin);
	freopen_s(&stream, "CONOUT$", "w+t", stderr);
	setlocale(LC_ALL, "chs");
	std::ios::sync_with_stdio();
	std::cout.clear();
}
std::string YSysUtility::UTF8ToString(const std::string& str) {
	int len_w = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	std::vector<wchar_t> buf_pw;
	buf_pw.resize(len_w + 1);
	memset(&buf_pw[0], 0, len_w * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(),(int) str.length(), &buf_pw[0], len_w);
	int len_c = WideCharToMultiByte(CP_ACP, 0, &buf_pw[0], -1, NULL, NULL, NULL, NULL);
	std::string buf_des;
	buf_des.resize(len_c + 1);
	memset(&buf_des[0], 0, len_c + 1);
	WideCharToMultiByte(CP_ACP, 0, &buf_pw[0], len_w, &buf_des[0], len_c, NULL, NULL);
	return buf_des;
}

bool YSysUtility::IsDirectoryExist(const std::string& str)
{
	DWORD fileAttributes = ::GetFileAttributesA(str.c_str());
	if (FILE_ATTRIBUTE_DIRECTORY & fileAttributes)
	{
		return true;
	}
	return false;
}

bool YSysUtility::FileExists(const std::string& str)
{
	uint32_t Result = GetFileAttributesA(str.c_str());
	if (Result != 0xFFFFFFFF && !(Result & FILE_ATTRIBUTE_DIRECTORY))
	{
		return true;
	}
	return false;
}

void YSysUtility::CreateDirectoryRecursive(const std::string& directory)
{
	if (directory.empty())
	{
		return;
	}
	if (YPath::DirectoryExists(directory))
	{
		return;
	}
	std::vector<std::string> base_paths=  YPath::GetFilePaths(directory);
	std::string created_path;
	for (const std::string& relative_child_path : base_paths)
	{
		created_path = YPath::PathCombine(created_path, relative_child_path);
		BOOL result = ::CreateDirectoryA(created_path.c_str(), nullptr);
		if (!result)
		{
			DWORD hr = ::GetLastError();
			if (hr == ERROR_ALREADY_EXISTS)
			{
				continue;
			}
			else if (hr == ERROR_ALREADY_EXISTS)
			{
				WARNING_INFO("path: ", directory, " create failed , because 	One or more intermediate directories do not exist; this function will only create the final directory in the path.");
				break;
			}
		}
	}
}
