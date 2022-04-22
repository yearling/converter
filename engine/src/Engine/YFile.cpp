#include "Engine/YFile.h"
#include "Engine/YLog.h"
#include <cstdio>
#include "Utility/YPath.h"
#include "Math/YRotator.h"
#include "Math/YQuaterion.h"
YFile::YFile()
	:type_(FileType::FT_NUM)
{

}

YFile::YFile( const std::string& path, FileType type )
	:path_(path), type_(type)
{

}

YFile::YFile(const std::string& path)
	:path_(path)
{

}

YFile::YFile(FileType type)
	:type_(type)
{

}

std::unique_ptr<MemoryFile> YFile::ReadFile()
{
	bool read_file_flag = (int)type_ && (int)FileType::FT_Read;
	bool binary_content = (int)type_ && (int)FileType::FT_BINARY;
	assert(read_file_flag);
	if (!read_file_flag)
	{
		return nullptr;
	}

	/*
	In text mode, a file is assumed to consist of lines of printable characters(perhaps including tabs).The routines in the stdio library(getc, putc, and all the rest) translate between the underlying system's end-of-line representation and the single \n used in C programs. C programs which simply read and write text therefore don't have to worry about the underlying system's newline conventions: when a C program writes a '\n', the stdio library writes the appropriate end-of-line indication, and when the stdio library detects an end-of-line while reading, it returns a single '\n' to the calling program. [footnote]

		In binary mode, on the other hand, bytes are readand written between the programand the file without any interpretation. (On MS - DOS systems, binary mode also turns off testing for control - Z as an in - band end - of - file character.)

		Text mode translations also affect the apparent size of a file as it's read. Because the characters read from and written to a file in text mode do not necessarily match the characters stored in the file exactly, the size of the file on disk may not always match the number of characters which can be read from it. Furthermore, for analogous reasons, the fseek and ftell functions do not necessarily deal in pure byte offsets from the beginning of the file. (Strictly speaking, in text mode, the offset values used by fseek and ftell should not be interpreted at all: a value returned by ftell should only be used as a later argument to fseek, and only values returned by ftell should be used as arguments to fseek.)

		In binary mode, fseekand ftell do use pure byte offsets.However, some systems may have to append a number of null bytes at the end of a binary file to pad it out to a full record.
		简单说， 在linux平台下， binary与txt没区别
		在windows下，txt模式需要转义。需要区分binary与txt, 因为txt转义后大小会变，需要用ftell来获取文件大小
	*/

	FILE* read_file = nullptr;
	read_file = fopen(path_.c_str(), binary_content ? "rb" : "r");
	if (!read_file)
	{
		LOG_INFO("open file failed: ", path_);
		fclose(read_file);
		return nullptr;
	}
	fseek(read_file, 0L, SEEK_END);
	size_t size = ftell(read_file);
	fseek(read_file, 0L, SEEK_SET);
	if (size == 0) {
		fclose(read_file);
		return std::make_unique<MemoryFile>();
	}
	std::unique_ptr<MemoryFile> mem_file = std::make_unique<MemoryFile>(MemoryFile::FileType::FT_Read);
	mem_file->AllocSizeUninitialized((uint32_t)size);
	size_t read_size = fread(mem_file->GetData(), sizeof(unsigned char), size, read_file);
	assert(read_size == size);
	fclose(read_file);
	return std::move(mem_file);
	return nullptr;
}

bool YFile::WriteFile( const MemoryFile* memory_file, bool create_directory_recurvie)
{
	assert(memory_file);
	if (!memory_file)
	{
		return false;
	}

	if (create_directory_recurvie)
	{
		YPath::CreateDirectoryRecursive(YPath::GetPath(path_));
	}
	bool write_file_flag = (int)type_ && (int)FileType::FT_Write;
	bool binary_content = (int)type_ && (int)FileType::FT_BINARY;
	assert(write_file_flag);
	FILE* write_file = nullptr;
	write_file = fopen(path_.c_str(), binary_content ? "wb" : "w");
	if (!write_file)
	{
		LOG_INFO("write file failed: ", path_);
		return false;
	}
	const std::vector<unsigned char>& content_to_write = memory_file->GetReadOnlyFileContent();
	if (content_to_write.empty())
	{
		fclose(write_file);
		return true;
	}
	size_t write_size = fwrite(&memory_file->GetReadOnlyFileContent()[0], sizeof(unsigned char), content_to_write.size(), write_file);
	if (write_size != (size_t)content_to_write.size())
	{
		fclose(write_file);
		return false;
	}
	fclose(write_file);
	return true;
}

YFile::~YFile()
{

}

MemoryFile::MemoryFile()
{

}

MemoryFile::MemoryFile(FileType type)
	:type_(type)
{

}

void MemoryFile::ReserveSize(uint32_t reserve_file_size)
{
	if (reserve_file_size > memory_content_.capacity())
	{
		memory_content_.reserve(reserve_file_size);
	}
}

void MemoryFile::AllocSizeUninitialized(uint32_t size)
{
	FitSize(size);
	memory_content_.resize(size);
}


const std::vector<unsigned char>& MemoryFile::GetReadOnlyFileContent() const
{
	return memory_content_;
}

bool MemoryFile::ReadBool(bool& value)
{
	return ReadElemts(&value, 1);
}

bool MemoryFile::ReadChar(char& value)
{
	return ReadElemts(&value, 1);
}

bool MemoryFile::ReadChars(char* value, int n)
{
	return ReadElemts(value, n);
}

bool MemoryFile::ReadUChar(unsigned char& value)
{
	return ReadElemts(&value, 1);
}

bool MemoryFile::ReadUChars(unsigned char* value, int n)
{
	return ReadElemts(value, n);
}

bool MemoryFile::ReadInt32(int& value)
{
	return ReadElemts(&value, 1);
}

bool MemoryFile::ReadUInt32(uint32_t& value)
{
	return ReadElemts(&value, 1);
}

bool MemoryFile::ReadInt32Vector(int* value, int n)
{
	return ReadElemts(value, n);
}

bool MemoryFile::ReadFloat32(float& value)
{
	return ReadElemts(&value, 1);
}

bool MemoryFile::ReadFloat32Vector(float* value, int n)
{
	return ReadElemts(value, n);
}

bool MemoryFile::ReadYVector(YVector& vec)
{
	return ReadElemts(&vec, 1);
}

bool MemoryFile::ReadYVector4(YVector4& vec)
{
	return ReadElemts(&vec, 1);
}

bool MemoryFile::ReadYMatrix(YMatrix& mat)
{
	return ReadElemts(&mat, 1);
}

bool MemoryFile::ReadString(std::string& str)
{
	int str_size = 0;
	if (!ReadInt32(str_size))
	{
		return false;
	}

	if (str_size == 0)
	{
		str.clear();
		return true;
	}
	else
	{
		str.resize(str_size);
		return ReadChars(&str[0], str_size);
	}
	return false;
}

void MemoryFile::WriteChar(char value)
{
	WriteElemts(&value, 1);
}

void MemoryFile::WriteChars(const char* value, int n)
{
	WriteElemts(value, n);
}

void MemoryFile::WriteUChar(unsigned char value)
{
	WriteElemts(&value, 1);
}

void MemoryFile::WriteInt32(int value)
{
	WriteElemts(&value, 1);
}

void MemoryFile::WriteUInt32(uint32_t value)
{
	WriteElemts(&value, 1);
}

void MemoryFile::WriteInt32Vector(const int* value, int n)
{
	WriteElemts(value, n);
}

void MemoryFile::WriteFloat32(float value)
{
	WriteElemts(&value, 1);
}

void MemoryFile::WriteFloat32Vector(const float* value, int n)
{
	WriteElemts(value, n);
}

void MemoryFile::WriteString(const std::string& str)
{
	WriteInt32((int)str.size());
	WriteChars(str.c_str(), (int)str.size());
}

void MemoryFile::WriteYVector(const YVector& vec)
{
	WriteElemts(&vec, 1);
}

void MemoryFile::WriteYVector4(const YVector4& vec)
{
	WriteElemts(&vec, 1);

}

void MemoryFile::WriteYMatrix(const YMatrix& mat)
{
	WriteElemts(&mat, 1);
}

void MemoryFile::WriteBool(bool value)
{
	WriteElemts(&value, 1);
}

int64 MemoryFile::Tell()
{
	throw std::logic_error("The method or operation is not implemented.");
}

int64 MemoryFile::TotalSize()
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool MemoryFile::AtEnd()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void MemoryFile::Seek(int64 InPos)
{
	throw std::logic_error("The method or operation is not implemented.");
}

void MemoryFile::Flush()
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool MemoryFile::Close()
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool MemoryFile::GetError()
{
	throw std::logic_error("The method or operation is not implemented.");
}

void MemoryFile::Serialize(void* V, int64 Length)
{
	if (IsReading())
	{
		if (read_pos_ + Length > memory_content_.size())
		{
			SetError();
			return;
		}
		memcpy(V, &memory_content_[read_pos_], Length);
		read_pos_ += Length;
	}
	else
	{
		FitSize(memory_content_.size() + Length);
		size_t current_size = memory_content_.size();
		AllocSizeUninitialized((uint32_t)(current_size + Length));
		memcpy(&memory_content_[current_size], V, Length);
	}
}

//MemoryFile& MemoryFile::operator<<( int& value)
//{
//	if(IsReading())
//	{ 
//		ReadInt32(value);
//	}
//	else
//	{
//		WriteInt32(value);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<(float& value)
//{
//	if (IsReading())
//	{
//		ReadFloat32(value);
//	}
//	else 
//	{
//		WriteFloat32(value);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<(char& value)
//{
//	if (IsReading())
//	{
//		ReadChar(value);
//	}
//	else
//	{
//		WriteChar(value);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<(unsigned char& value)
//{
//	if (IsReading())
//	{
//		ReadUChar(value);
//	}
//	else
//	{
//		WriteUChar(value);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<(std::string& value)
//{
//	if(IsReading())
//	{
//		ReadString(value);
//	}
//	else
//	{
//		WriteString(value);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<(YVector2& value)
//{
//	if (IsReading())
//	{
//		ReadFloat32Vector(&value.x, 2);
//	}
//	else
//	{
//		WriteFloat32Vector(&value.x, 2);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<(YVector& value)
//{
//	if(IsReading())
//	{ 
//		ReadFloat32Vector(&value.x, 3);
//	}
//	else
//	{
//		WriteFloat32Vector(&value.x, 3);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<( YVector4& value)
//{
//	if (IsReading())
//	{
//		ReadFloat32Vector(&value.x, 4);
//	}
//	else
//	{
//		WriteFloat32Vector(&value.x, 4);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<( YMatrix& value)
//{
//
//	if (IsReading())
//	{
//		ReadFloat32Vector(&value.m[0][0], 16);
//	}
//	else
//	{
//		WriteFloat32Vector(&value.m[0][0], 16);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<( YRotator& value)
//{
//	if (IsReading())
//	{
//		ReadFloat32Vector(&value.pitch, 3);
//	}
//	else
//	{
//		WriteFloat32Vector(&value.pitch, 3);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<( YQuat& value)
//{
//	if (IsReading())
//	{
//		ReadFloat32Vector(&value.x, 4);
//	}
//	else
//	{
//		WriteFloat32Vector(&value.x, 4);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<(uint32_t& value)
//{
//	if (IsReading())
//	{
//		ReadUInt32(value);
//	}
//	else
//	{
//		WriteUInt32(value);
//	}
//	return *this;
//}
//
//MemoryFile& MemoryFile::operator<<(bool& value)
//{
//	if (IsReading())
//	{
//		ReadBool(value);
//	}
//	else
//	{
//		WriteBool(value);
//	}
//	return *this;
//}

void MemoryFile::FitSize(uint64 increase_size)
{
	if (increase_size > memory_content_.capacity())
	{
		size_t new_size = memory_content_.capacity();
		while (new_size < increase_size)
		{
			new_size += increase_block_size;
		}
		memory_content_.reserve(new_size);
	}
}

MemoryFile::~MemoryFile()
{

}
