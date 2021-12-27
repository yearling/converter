#pragma  once
#include <string>
#include <vector>
struct YPath
{
	// c:\user\admin\desktop\a.txt ==> [c:] [user] [admin] [desktop] [a.txt]
	static std::vector<std::string> GetFilePaths(const std::string& path);
	static std::vector<std::string> GetDirectoryPaths(const std::string directory);
	// Returns the filename (with extension), minus any path information.
	static std::string GetCleanFilename(const std::string& path);

	// Returns the same thing as GetCleanFilename, but without the extension
	static std::string GetBaseFilename(const std::string& in_path, bool remove_path = true);

	// Returns the path in front of the filename
	static  std::string  GetPath(const  std::string& in_path);

	static std::string GetFileExtension(const std::string& path,bool include_dot= false);

	// Changes the extension of the given filename
	static std::string ChangeExtension(const std::string& InPath, const std::string& InNewExtension);

	/** @return true if this file was found, false otherwise */
	static bool FileExists(const std::string& InPath);

	/** @return true if this directory was found, false otherwise */
	static bool DirectoryExists(const std::string& InPath);

	/** @return true if this path is relative */
	static bool IsRelative(const std::string& InPath);

	/** Convert all / and \ to TEXT("/") */
	static void NormalizeFilename(std::string& InPath);

	static std::string PathCombine(const std::string& a, const std::string& b);
	template<class T, class ...Args>
	static std::string PathCombine(const T& head, Args... rest)
	{
		return PathCombine(head, PathCombine(rest));
	}
	static void CreateDirectoryRecursive(const std::string& dir_path);
	static const char directory_seperater = '/';
	static const std::string separators;
};