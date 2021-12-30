#pragma  once
#include <string>
#include <vector>
#include <memory>
#include "Math/YVector.h"
#include "Math/YMatrix.h"
#include "Engine/YLog.h"
#include <unordered_map>


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
	YFile(const std::string& path, FileType type);
	std::unique_ptr<MemoryFile> ReadFile();
	bool WriteFile(const MemoryFile* memory_file, bool create_directory_recurvie = true);
	inline FileType GetFileType() const {
		return type_;
	};
	inline std::string GetFilePath() const { return path_; }
protected:
	std::string path_;
	FileType type_;
};

class MemoryFile
{
public:
	enum FileType
	{
		FT_Read = 1 << 1,
		FT_Write = 1 << 2
	};

	MemoryFile();
	~MemoryFile();
	explicit MemoryFile(FileType type);
	bool IsReading() const { return (FileType::FT_Read & type_); }
	void ReserveSize(uint32_t reserve_file_size);
	void AllocSizeUninitialized(uint32_t reserve_file_size);
	inline uint32_t GetSize() { return (uint32_t)memory_content_.size(); }
	const unsigned char* GetData() const { return memory_content_.data(); }
	unsigned char* GetData() { return memory_content_.data(); }
	const std::vector<unsigned char>& GetReadOnlyFileContent()const;
	bool ReadBool(bool& value);
	bool ReadChar(char& value);
	bool ReadChars(char* value, int n);
	bool ReadUChar(unsigned char& value);
	bool ReadUChars(unsigned char* value, int n);
	bool ReadInt32(int& value);
	bool ReadUInt32(uint32_t& value);
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
	void WriteUInt32(uint32_t value);
	void WriteInt32Vector(const int* value, int n);
	void WriteFloat32(float value);
	void WriteFloat32Vector(const float* value, int n);
	void WriteString(const std::string& str);
	void WriteYVector(const YVector& vec);
	void WriteYVector4(const YVector4& vec);
	void WriteYMatrix(const YMatrix& mat);
	void WriteBool(bool value);
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
		AllocSizeUninitialized((uint32_t)(current_size + write_size));
		memcpy(&memory_content_[current_size], value, write_size);
	}

	MemoryFile& operator<<(bool& value);
	MemoryFile& operator<<(int& value);
	MemoryFile& operator<<(uint32_t& value);
	MemoryFile& operator<<(float& value);
	MemoryFile& operator<<(char& value);
	MemoryFile& operator<<(unsigned char& value);
	MemoryFile& operator<<(std::string& value);
	MemoryFile& operator<<(YVector2& value);
	MemoryFile& operator<<(YVector& value);
	MemoryFile& operator<<(YVector4& value);
	MemoryFile& operator<<(YMatrix& value);
	MemoryFile& operator<<(YRotator& value);
	MemoryFile& operator<<(YQuat& value);
protected:
	friend YFile;
	void FitSize(size_t increase_size);
	uint32_t read_pos_{ 0 };
	const int increase_block_size = 2 * 1024 * 1024;
	std::vector<unsigned char> memory_content_;
	FileType type_;
};


template <typename T>
typename std::enable_if<std::is_pod<T>::value>::type SFINAE_Operator(MemoryFile& mem_file, std::vector<T>& value)
{
	if (mem_file.IsReading())
	{
		uint32_t value_element_size = 0;
		mem_file << value_element_size;
		value.resize(value_element_size);
		if (value_element_size > 0)
		{
			mem_file.ReadElemts(&value[0], value_element_size);
		}
	}
	else
	{
		uint32_t value_element_size = (uint32_t)value.size();
		mem_file << value_element_size;
		if (value_element_size > 0)
		{
			mem_file.WriteElemts(&value[0], value_element_size);
		}
	}
}

template <typename T>
typename std::enable_if<!std::is_pod<T>::value>::type SFINAE_Operator(MemoryFile& mem_file, std::vector<T>& value)
{
	if (mem_file.IsReading())
	{
		uint32_t vector_size = 0;
		mem_file << vector_size;
		value.resize(vector_size);
		for (auto& elem : value)
		{
			mem_file << elem;
		}
	}
	else
	{
		uint32_t value_element_size = (uint32_t)value.size();
		mem_file << value_element_size;
		for (auto& elem : value)
		{
			mem_file << elem;
		}
	}
}

template<class T>
MemoryFile& operator<<(MemoryFile& mem_file, std::vector<T>& value)
{
	SFINAE_Operator(mem_file, value);
	return mem_file;
}

template<class k, class v>
MemoryFile& operator<<(MemoryFile& mem_file, std::unordered_map<k, v>& in_map)
{
	if (mem_file.IsReading())
	{
		uint32_t item_count = 0;
		mem_file << item_count;
		for (uint32_t i = 0; i < item_count; ++i)
		{
			k k_value;
			mem_file << k_value;
			v v_value;
			mem_file << v_value;
			in_map.insert({ k_value, v_value });
		}
	}
	else
	{
		uint32_t item_count = (uint32_t)in_map.size();
		mem_file << item_count;
		for (auto& kv : in_map)
		{
			mem_file << (k)(kv.first);
			mem_file << kv.second;
		}
	}

	return mem_file;
}
