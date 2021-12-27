#include <windows.h>
#include <string>
class YSysUtility
{
public:
	static void AllocWindowsConsole();
	static void CreateDirectoryRecursive(const std::string& directory);
	static std::string UTF8ToString(const std::string& str);
	static bool IsDirectoryExist(const std::string& str);
	static bool FileExists(const std::string& str);
};