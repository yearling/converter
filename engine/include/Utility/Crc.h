// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <stdint.h>
#include <locale>
#include <cassert>



/** 
 * CRC hash generation for different types of input data
 **/
struct  FCrc
{
	/** lookup table with precalculated CRC values - slicing by 8 implementation */
	static uint32_t CRCTablesSB8[8][256];

	/** initializes the CRC lookup table. Must be called before any of the
		CRC functions are used. */
	static void Init();

	/** generates CRC hash of the memory area */
	static uint32_t MemCrc32( const void* Data, int32_t Length, uint32_t CRC=0 );

	/** String CRC. */
	//template <typename CharType>
	//static typename TEnableIf<sizeof(CharType) != 1, uint32_t>::Type StrCrc32(const CharType* Data, uint32_t CRC = 0)
	//{
	//	// We ensure that we never try to do a StrCrc32 with a CharType of more than 4 bytes.  This is because
	//	// we always want to treat every CRC as if it was based on 4 byte chars, even if it's less, because we
	//	// want consistency between equivalent strings with different character types.
	//	static_assert(sizeof(CharType) <= 4, "StrCrc32 only works with CharType up to 32 bits.");

	//	CRC = ~CRC;
	//	while (CharType Ch = *Data++)
	//	{
	//		CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
	//		Ch >>= 8;
	//		CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
	//		Ch >>= 8;
	//		CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
	//		Ch >>= 8;
	//		CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
	//	}
	//	return ~CRC;
	//}

	static char StrCrc32(const char* Data, uint32_t CRC = 0)
	{
		/* Overload for when CharType is a byte, which causes warnings when right-shifting by 8 */
		CRC = ~CRC;
		while (char Ch = *Data++)
		{
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC     ) & 0xFF];
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC     ) & 0xFF];
			CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC     ) & 0xFF];
		}
		return ~CRC;
	}

	/**
	 * DEPRECATED
	 * These tables and functions are deprecated because they're using tables and implementations
	 * which give values different from what a user of a typical CRC32 algorithm might expect.
	 */

	/** lookup table with precalculated CRC values */
	static uint32_t CRCTable_DEPRECATED[256];
	/** lookup table with precalculated CRC values - slicing by 8 implementation */
	static uint32_t CRCTablesSB8_DEPRECATED[8][256];

	/** String CRC. */
	static inline uint32_t StrCrc_DEPRECATED(const char* Data)
	{
		// make sure table is initialized
		assert(CRCTable_DEPRECATED[1] != 0);

		int32_t Length =(int32_t) strlen( Data );
		uint32_t CRC = 0xFFFFFFFF;
		for( int32_t i=0; i<Length; i++ )
		{
			char C = Data[i];
			int32_t CL = (C&255);
			CRC = (CRC << 8) ^ CRCTable_DEPRECATED[(CRC >> 24) ^ CL];
			int32_t CH = (C>>8)&255;
			CRC = (CRC << 8) ^ CRCTable_DEPRECATED[(CRC >> 24) ^ CH];
		}
		return ~CRC;
	}

	/** Case insensitive string hash function. */
	static inline uint32_t Strihash_DEPRECATED( const char* Data );

	/** generates CRC hash of the memory area */
	static uint32_t MemCrc_DEPRECATED( const void* Data, int32_t Length, uint32_t CRC=0 );
};

inline uint32_t FCrc::Strihash_DEPRECATED(const char* Data)
{
	// make sure table is initialized
	assert(CRCTable_DEPRECATED[1] != 0);

	uint32_t Hash=0;
	while( *Data )
	{
		char Ch = std::toupper(*Data++);
		uint8_t B  = Ch;
		Hash = ((Hash >> 8) & 0x00FFFFFF) ^ CRCTable_DEPRECATED[(Hash ^ B) & 0x000000FF];
	}
	return Hash;
}

//template <>
//inline uint32_t FCrc::Strihash_DEPRECATED(const WIDECHAR* Data)
//{
//	// make sure table is initialized
//	assert(CRCTable_DEPRECATED[1] != 0);
//
//	uint32_t Hash=0;
//	while( *Data )
//	{
//#if !PLATFORM_TCHAR_IS_4_BYTES
//		// for 2 byte wide chars use ansi toupper() if it's not an actual wide char (upper byte == 0)
//		// towupper() is painfully slow on Switch ATM.
//		WIDECHAR Ch = (*Data & 0xFF00) ? TChar<WIDECHAR>::ToUpper(*Data++) : TChar<char>::ToUpper(*Data++);
//#else
//		WIDECHAR Ch = TChar<WIDECHAR>::ToUpper(*Data++);
//#endif
//		uint16  B  = Ch;
//		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ CRCTable_DEPRECATED[(Hash ^ B) & 0x000000FF];
//		B        = Ch>>8;
//		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ CRCTable_DEPRECATED[(Hash ^ B) & 0x000000FF];
//	}
//	return Hash;
//}
