#pragma once
#include <string>
#include <stdarg.h>
#include <cstdio>
#include <cassert>
#include <stdint.h>

template< typename... Args >
std::string StringFormat(const char* format, Args... args) {
	int length = std::snprintf(nullptr, 0, format, args...);
	assert(length >= 0);
	const int cache_size = 512;
	char cache_buffer[cache_size];
	if (length < cache_size)
	{
		std::snprintf(cache_buffer, length + 1, format, args...);
		return std::string(cache_buffer);
	}
	else
	{
		std::string str(length + 1,0);
		std::snprintf(str.data(), length + 1, format, args...);
		return str;
	}
}

/** @return Char value of Nibble */
inline char NibbleToTChar(uint8_t Num)
{
	if (Num > 9)
	{
		return char('A') + char(Num - 10);
	}
	return char('0') + char(Num);
}

/**
 * Convert a byte to hex
 * @param In byte value to convert
 * @param Result out hex value output
 */
inline void ByteToHex(uint8_t In, std::string& Result)
{
	Result += NibbleToTChar(In >> 4);
	Result += NibbleToTChar(In & 15);
}

/**
 * Convert an array of bytes to hex
 * @param In byte array values to convert
 * @param Count number of bytes to convert
 * @return Hex value in string.
 */
inline std::string BytesToHex(const uint8_t* In, int32_t Count)
{
	std::string Result;
	Result.clear();
	Result.resize(Count * 2);

	while (Count)
	{
		ByteToHex(*In++, Result);
		Count--;
	}
	return Result;
}

/**
 * Checks if the TChar is a valid hex character
 * @param Char		The character
 * @return	True if in 0-9 and A-F ranges
 */
inline const bool CheckTCharIsHex(const char Char)
{
	return (Char >= char('0') && Char <= char('9')) || (Char >= char('A') && Char <= char('F')) || (Char >= char('a') && Char <= char('f'));
}

/**
 * Convert a TChar to equivalent hex value as a uint8_t
 * @param Char		The character
 * @return	The uint8_t value of a hex character
 */
inline const uint8_t TCharToNibble(const char Char)
{
	assert(CheckTCharIsHex(Char));
	if (Char >= char('0') && Char <= char('9'))
	{
		return Char - char('0');
	}
	else if (Char >= char('A') && Char <= char('F'))
	{
		return (Char - char('A')) + 10;
	}
	return (Char - char('a')) + 10;
}

/**
 * Convert std::string of Hex digits into the byte array.
 * @param HexString		The std::string of Hex values
 * @param OutBytes		Ptr to memory must be preallocated large enough
 * @return	The number of bytes copied
 */
inline int32_t HexToBytes(const std::string& HexString, uint8_t* OutBytes)
{
	int32_t NumBytes = 0;
	const bool bPadNibble = (HexString.size() % 2) == 1;
	const char* CharPos = HexString.c_str();
	if (bPadNibble)
	{
		OutBytes[NumBytes++] = TCharToNibble(*CharPos++);
	}
	while (*CharPos)
	{
		OutBytes[NumBytes] = TCharToNibble(*CharPos++) << 4;
		OutBytes[NumBytes] += TCharToNibble(*CharPos++);
		++NumBytes;
	}
	return NumBytes;
}
