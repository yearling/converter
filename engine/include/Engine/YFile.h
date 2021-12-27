#pragma  once
#include <string>
#include <vector>
#include <memory>
#include "Math/YVector.h"
#include "Math/YMatrix.h"


class MemoryFile;
class YFile
{
public:
	enum  FileType
	{
		FT_Read = 1 << 1,
		FT_Write = 1 << 2,
		FT_TXT = 1 << 3,
		FT_BINARY = 1 << 4,
		FT_NUM
	};
	YFile();
	~YFile();
	explicit YFile(const std::string& path);
	explicit YFile(FileType type);
	YFile(FileType type, const std::string& path);
	std::unique_ptr<MemoryFile> ReadFile();
	bool WriteFile(const std::string& path, const MemoryFile* memory_file);
protected:
	FileType type_;
	std::string path_;
};

class MemoryFile
{
public:
	enum class FileType
	{
		FT_Read = 0,
		FT_Write = 1
	};

	MemoryFile();
	~MemoryFile();
	explicit MemoryFile(FileType type);
	void ReserveSize(uint32_t reserve_file_size);
	void AllocSizeUninitialized(uint32_t reserve_file_size);
	std::vector<unsigned char>& GetFileContent();
	const std::vector<unsigned char>& GetReadOnlyFileContent()const;
	bool ReadChar(char& value);
	bool ReadChars(char* value, int n);
	bool ReadUChar(unsigned char& value);
	bool ReadUChars(unsigned char* value, int n);
	bool ReadInt32(int& value);
	bool ReadInt32Vector(int* value, int n);
	bool ReadFloat32(float& value);
	bool ReadFloat32Vector(float* value, int n);
	bool ReadYVector(YVector& vec);
	bool ReadYVector4(YVector4& vec);
	bool ReadYMatrix(YMatrix& mat);
	bool ReadString(std::string& str);
	void WriteChar(char value);
	void WriteChars(const char* value, int n);
	void WriteUChar(unsigned char value);
	void WriteInt32(int value);
	void WriteInt32Vector(const int* value, int n);
	void Writeloat32(float value);
	void WriteFloat32Vector(float* value, int n);
	void WriteString(const std::string& str);
	void WriteYVector(const YVector& vec);
	void WriteYVector4(const YVector4& vec);
	void WriteYMatrix(const YMatrix& mat);
	template<typename T>
	bool ReadElemts(T* value, int n)
	{
		size_t read_size = sizeof(T) * n;
		if (read_pos_ + read_size > memory_content_.size())
		{
			return false;
		}
		memcpy(value, &memory_content_[read_pos_], read_size);
		read_pos_ += (uint32_t)read_size;
		return true;
	}


	template<typename T>
	void WriteElemts(const T* value, int n)
	{
		size_t write_size = sizeof(T) * n;
		FitSize(memory_content_.size() + write_size);
		size_t current_size = memory_content_.size();
		memory_content_.resize(current_size + write_size);
		memcpy(&memory_content_[current_size], &value, write_size);
	}
protected:
	void FitSize(size_t increase_size);
	uint32_t read_pos_{ 0 };
	const int increase_block_size = 2 * 1024 * 1024;
	std::vector<unsigned char> memory_content_;
	FileType type_;
};