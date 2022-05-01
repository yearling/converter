#pragma  once
#include <string>
#include <vector>
#include <memory>
#include "Math/YVector.h"
#include "Math/YMatrix.h"
#include "Engine/YLog.h"
#include <unordered_map>
#include "YArchive.h"


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

class MemoryFile:public YArchive
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
	virtual int64 Tell() override;
	virtual int64 TotalSize() override;
	virtual bool AtEnd() override;
	virtual void Seek(int64 InPos) override;
	virtual void Flush() override;
	virtual bool Close() override;
	virtual bool GetError() override;
	virtual void Serialize(void* V, int64 Length) override;

protected:
	friend YFile;
	void FitSize(uint64 increase_size);
	uint64 read_pos_{ 0 };
	const int increase_block_size = 2 * 1024 * 1024;
	std::vector<unsigned char> memory_content_;
	FileType type_;
};

class YBufferReader :public YArchive
{
	YBufferReader(void* buffer, )
};
