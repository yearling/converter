#pragma  once
#include <string>
#include <vector>
#include <memory>
#include "Math/YVector.h"
#include "Math/YMatrix.h"
#include "Engine/YLog.h"


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
	bool WriteFile(const std::string& path, const MemoryFile* memory_file,bool create_directory_recurvie =true);
protected:
	FileType type_;
	std::string path_;
};

class MemoryFile
{
public:
	enum FileType
	{
		FT_Read = 0,
		FT_Write = 1
	};

	MemoryFile();
	~MemoryFile();
	explicit MemoryFile(FileType type);
	bool IsReading() const { return (FileType::FT_Read & type_); }
	void ReserveSize(uint32_t reserve_file_size);
	void AllocSizeUninitialized(uint32_t reserve_file_size);
	inline uint32_t GetSize() { return (uint32_t) memory_content_.size(); }
	std::vector<unsigned char>& GetFileContent();
	const unsigned char* GetData() const { return memory_content_.data(); }
	unsigned char* GetData()  { return memory_content_.data(); }
	const std::vector<unsigned char>& GetReadOnlyFileContent()const;
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

	 MemoryFile& operator<<( int value);
	 MemoryFile& operator<<( uint32_t value);
	 MemoryFile& operator<<( float value);
	 MemoryFile& operator<<( char value);
	 MemoryFile& operator<<(  unsigned char value);
	 MemoryFile& operator<<( const std::string& value);
	 MemoryFile& operator<<( const YVector2& value);
	 MemoryFile& operator<<( const YVector& value);
	 MemoryFile& operator<<( const YVector4& value);
	 MemoryFile& operator<<( const YMatrix& value);
	 MemoryFile& operator<<( const YRotator& value);
	 MemoryFile& operator<<( const YQuat& value);
protected:
	void FitSize(size_t increase_size);
	uint32_t read_pos_{ 0 };
	const int increase_block_size = 2 * 1024 * 1024;
	std::vector<unsigned char> memory_content_;
	FileType type_;
};


template <typename T>
typename std::enable_if<std::is_pod<T>::value>::type SFINAE_Operator(MemoryFile& mem_file, const std::vector<T>& value)
{
	LOG_INFO("memory file use pod copy");
	mem_file << (uint32_t)value.size();
	uint32_t old_size = mem_file.GetSize();
	uint32_t value_byte_counts =(uint32_t)( value.size() * sizeof(T));
	uint32_t new_size = old_size + value_byte_counts;
	mem_file.AllocSizeUninitialized(new_size);
	unsigned char* des_ptr = mem_file.GetData();
	if (des_ptr && value.size()!=0)
	{
		memcpy(des_ptr, &value[0], value_byte_counts);
	}
}

template <typename T>
typename std::enable_if<!std::is_pod<T>::value>::type SFINAE_Operator(MemoryFile& mem_file, const std::vector<T>& value)
{
	LOG_INFO("memory file use non pod copy");
	mem_file <<(int) value.size();
	for (auto& elem : value)
	{
		mem_file << elem;
	}
}

template<class T>
MemoryFile& operator<<(MemoryFile& mem_file,const std::vector<T>& value)
{
	SFINAE_Operator(mem_file, value);
	return mem_file;
}
