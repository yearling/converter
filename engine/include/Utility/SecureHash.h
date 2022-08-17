// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <string.h>
#include <stdint.h>
#include <string>
#include "Utility/YStringFormat.h"
#include <vector>
#include <map>
#include <unordered_map>

struct FMD5Hash;

/*-----------------------------------------------------------------------------
	MD5 functions.
-----------------------------------------------------------------------------*/

/** @name MD5 functions */
//@{
//
// MD5 Context.
//


//
// MD5 functions.
//!!it would be cool if these were implemented as subclasses of
// FArchive.
//
//  void appMD5Init( FMD5Context* context );
//  void appMD5Update( FMD5Context* context, uint8_t* input, int32_t inputLen );
//  void appMD5Final( uint8_t* digest, FMD5Context* context );
//  void appMD5Transform( uint32_t* state, uint8_t* block );
//  void appMD5Encode( uint8_t* output, uint32_t* input, int32_t len );
//  void appMD5Decode( uint32_t* output, uint8_t* input, int32_t len );

class  FMD5
{
public:
	FMD5();
	~FMD5();

	/**
	 * MD5 block update operation.  Continues an MD5 message-digest operation,
	 * processing another message block, and updating the context.
	 *
	 * @param input		input data
	 * @param inputLen	length of the input data in bytes
	 **/
	void Update(const uint8_t* input, int32_t inputLen);

	/**
	 * MD5 finalization. Ends an MD5 message-digest operation, writing the
	 * the message digest and zeroizing the context.
	 * Digest is 16 BYTEs.
	 *
	 * @param digest	pointer to a buffer where the digest should be stored ( must have at least 16 bytes )
	 **/
	void Final(uint8_t* digest);

	/**
	 * Helper to perform the very common case of hashing an ASCII string into a hex representation.
	 * 
	 * @param String	the string the hash
	 **/
	static std::string HashAnsiString(const char* String)
	{
		uint8_t Digest[16];

		FMD5 Md5Gen;

		Md5Gen.Update((unsigned char*)( String ),(int32_t) strlen( String ) );
		Md5Gen.Final( Digest );

		std::string MD5;
		for( int32_t i=0; i<16; i++ )
		{
			//MD5 += std::string::Printf(TEXT("%02x"), Digest[i]);
			//MD5 += std::format("%02x", Digest[i]);
			MD5 += StringFormat("%02x", Digest[i]);
		}
		return MD5;
	}
private:
	struct FContext
	{
		uint32_t state[4];
		uint32_t count[2];
		uint8_t buffer[64];
	};

	void Transform( uint32_t* state, const uint8_t* block );
	void Encode( uint8_t* output, const uint32_t* input, int32_t len );
	void Decode( uint32_t* output, const uint8_t* input, int32_t len );

	FContext Context;
};
//@}

struct FMD5Hash;

/** Simple helper struct to ease the caching of MD5 hashes */
struct FMD5Hash
{
	/** Default constructor */
	FMD5Hash() : bIsValid(false) {}

	/** Check whether this has hash is valid or not */
	bool IsValid() const { return bIsValid; }

	/** Set up the MD5 hash from a container */
	void Set(FMD5& MD5)
	{
		MD5.Final(Bytes);
		bIsValid = true;
	}

	/** Compare one hash with another */
	friend bool operator==(const FMD5Hash& LHS, const FMD5Hash& RHS)
	{
		return LHS.bIsValid == RHS.bIsValid && (!LHS.bIsValid || ::memcmp(LHS.Bytes, RHS.Bytes, 16) == 0);
	}

	/** Compare one hash with another */
	friend bool operator!=(const FMD5Hash& LHS, const FMD5Hash& RHS)
	{
		return LHS.bIsValid != RHS.bIsValid || (LHS.bIsValid && ::memcmp(LHS.Bytes, RHS.Bytes, 16) != 0);
	}

	/** Serialise this hash */
	//friend FArchive& operator<<(FArchive& Ar, FMD5Hash& Hash)
	//{
	//	Ar << Hash.bIsValid;
	//	if (Hash.bIsValid)
	//	{
	//		Ar.Serialize(Hash.Bytes, 16);
	//	}

	//	return Ar;
	//}

	/** Hash the specified file contents (using the optionally supplied scratch buffer) */
	 static FMD5Hash HashFile(const char* InFilename, std::vector<uint8_t>* Buffer = nullptr);
	 //static FMD5Hash HashFileFromArchive(FArchive* Ar, TArray<uint8_t>* ScratchPad = nullptr);

	const uint8_t* GetBytes() const { return Bytes; }
	const int32_t GetSize() const { return sizeof(Bytes); }
    friend bool operator<(const FMD5Hash& LHS, const FMD5Hash& RHS)
    {
        union IntValue
        {
            uint8_t  bytes[8];
            uint64_t int_value;
        };

        IntValue value_lhs0;
        value_lhs0.bytes[0] = LHS.Bytes[0];
        value_lhs0.bytes[1] = LHS.Bytes[1];
        value_lhs0.bytes[2] = LHS.Bytes[2];
        value_lhs0.bytes[3] = LHS.Bytes[3];
        value_lhs0.bytes[4] = LHS.Bytes[4];
        value_lhs0.bytes[5] = LHS.Bytes[5];
        value_lhs0.bytes[6] = LHS.Bytes[6];
        value_lhs0.bytes[7] = LHS.Bytes[7];
        IntValue value_lhs1;
        value_lhs1.bytes[0] = RHS.Bytes[0];
        value_lhs1.bytes[1] = RHS.Bytes[1];
        value_lhs1.bytes[2] = RHS.Bytes[2];
        value_lhs1.bytes[3] = RHS.Bytes[3];
        value_lhs1.bytes[4] = RHS.Bytes[4];
        value_lhs1.bytes[5] = RHS.Bytes[5];
        value_lhs1.bytes[6] = RHS.Bytes[6];
        value_lhs1.bytes[7] = RHS.Bytes[7];
        return value_lhs0.int_value < value_lhs1.int_value;
    }
private:
	/** Whether this hash is valid or not */
	bool bIsValid;

	/** The bytes this hash comprises */
	uint8_t Bytes[16];

	friend  std::string LexToString(const FMD5Hash&);
	friend  void LexFromString(FMD5Hash& Hash, const char*);
};

/*-----------------------------------------------------------------------------
	SHA-1 functions.
-----------------------------------------------------------------------------*/

/*
 *	NOTE:
 *	100% free public domain implementation of the SHA-1 algorithm
 *	by Dominik Reichl <dominik.reichl@t-online.de>
 *	Web: http://www.dominik-reichl.de/
 */


typedef union
{
	uint8_t  c[64];
	uint32_t l[16];
} SHA1_WORKSPACE_BLOCK;

/** This divider string is beween full file hashes and script hashes */
#define HASHES_SHA_DIVIDER "+++"

/** Stores an SHA hash generated by FSHA1. */
class  FSHAHash
{
public:
	uint8_t Hash[20];

	FSHAHash()
	{
		::memset(Hash, 0, sizeof(Hash));
	}

	inline std::string ToString() const
	{
		return BytesToHex((const uint8_t*)Hash, sizeof(Hash));
	}
	void FromString(const std::string& Src)
	{
		assert(Src.size() == 40);
		HexToBytes(Src, Hash);
	}

	friend bool operator==(const FSHAHash& X, const FSHAHash& Y)
	{
		return ::memcmp(&X.Hash, &Y.Hash, sizeof(X.Hash)) == 0;
	}

	friend bool operator!=(const FSHAHash& X, const FSHAHash& Y)
	{
		return ::memcmp(&X.Hash, &Y.Hash, sizeof(X.Hash)) != 0;
	}

	//friend  FArchive& operator<<( FArchive& Ar, FSHAHash& G );
	
	friend  uint32_t GetTypeHash(FSHAHash const& InKey);
};

class  FSHA1
{
public:

	enum {DigestSize=20};
	// Constructor and Destructor
	FSHA1();
	~FSHA1();

	uint32_t m_state[5];
	uint32_t m_count[2];
	uint32_t __reserved1[1];
	uint8_t  m_buffer[64];
	uint8_t  m_digest[20];
	uint32_t __reserved2[3];

	void Reset();

	// Update the hash value
	void Update(const uint8_t *data, uint32_t len);

	// Update the hash value with string
	void UpdateWithString(const char *data, uint32_t len);

	// Finalize hash and report
	void Final();

	// Report functions: as pre-formatted and raw data
	void GetHash(uint8_t *puDest);

	/**
	 * Calculate the hash on a single block and return it
	 *
	 * @param Data Input data to hash
	 * @param DataSize Size of the Data block
	 * @param OutHash Resulting hash value (20 byte buffer)
	 */
	static void HashBuffer(const void* Data, uint32_t DataSize, uint8_t* OutHash);

	/**
	 * Generate the HMAC (Hash-based Message Authentication Code) for a block of data.
	 * https://en.wikipedia.org/wiki/Hash-based_message_authentication_code
	 *
	 * @param Key		The secret key to be used when generating the HMAC
	 * @param KeySize	The size of the key
	 * @param Data		Input data to hash
	 * @param DataSize	Size of the Data block
	 * @param OutHash	Resulting hash value (20 byte buffer)
	 */
	static void HMACBuffer(const void* Key, uint32_t KeySize, const void* Data, uint32_t DataSize, uint8_t* OutHash);

	/**
	 * Shared hashes.sha reading code (each platform gets a buffer to the data,
	 * then passes it to this function for processing)
	 *
	 * @param Buffer Contents of hashes.sha (probably loaded from an a section in the executable)
	 * @param BufferSize Size of Buffer
	 * @param bDuplicateKeyMemory If Buffer is not always loaded, pass true so that the 20 byte hashes are duplicated 
	 */
	static void InitializeFileHashesFromBuffer(uint8_t* Buffer, int32_t BufferSize, bool bDuplicateKeyMemory=false);

	/**
	 * Gets the stored SHA hash from the platform, if it exists. This function
	 * must be able to be called from any thread.
	 *
	 * @param Pathname Pathname to the file to get the SHA for
	 * @param Hash 20 byte array that receives the hash
	 * @param bIsFullPackageHash true if we are looking for a full package hash, instead of a script code only hash
	 *
	 * @return true if the hash was found, false otherwise
	 */
	static bool GetFileSHAHash(const char* Pathname, uint8_t Hash[20], bool bIsFullPackageHash=true);

private:
	// Private SHA-1 transformation
	void Transform(uint32_t *state, const uint8_t *buffer);

	// Member variables
	uint8_t m_workspace[64];
	SHA1_WORKSPACE_BLOCK *m_block; // SHA1 pointer to the byte array above

	/** Global map of filename to hash value, filled out in InitializeFileHashesFromBuffer */
	static std::unordered_map<std::string, uint8_t*> FullFileSHAHashMap;

	/** Global map of filename to hash value, but for script-only SHA hashes */
	static std::unordered_map<std::string, uint8_t*> ScriptSHAHashMap;
};


/**
 * Asynchronous SHA verification
 */
class  FAsyncSHAVerify
{
protected:
	/** Buffer to run the has on. This class can take ownership of the buffer is bShouldDeleteBuffer is true */
	void* Buffer;

	/** Size of Buffer */
	int32_t BufferSize;

	/** Hash to compare against */
	uint8_t Hash[20];

	/** Filename to lookup hash value (can be empty if hash was passed to constructor) */
	std::string Pathname;

	/** If this is true, and looking up the hash by filename fails, this will abort execution */
	bool bIsUnfoundHashAnError;

	/** Should this class delete the buffer memory when verification is complete? */
	bool bShouldDeleteBuffer;

public:

	/**
	 * Constructor. 
	 * 
	 * @param	InBuffer				Buffer of data to calculate has on. MUST be valid until this task completes (use Counter or pass ownership via bInShouldDeleteBuffer)
	 * @param	InBufferSize			Size of InBuffer
	 * @param	bInShouldDeleteBuffer	true if this task should FMemory::Free InBuffer on completion of the verification. Useful for a fire & forget verification
	 *									NOTE: If you pass ownership to the task MAKE SURE you are done using the buffer as it could go away at ANY TIME
	 * @param	Pathname				Pathname to use to have the platform lookup the hash value
	 * @param	bInIsUnfoundHashAnError true if failing to lookup the hash value results in a fail (only for Shipping PC)
	 */
	FAsyncSHAVerify(
		void* InBuffer, 
		int32_t InBufferSize, 
		bool bInShouldDeleteBuffer, 
		const char* InPathname, 
		bool bInIsUnfoundHashAnError)
		:	Buffer(InBuffer)
		,	BufferSize(InBufferSize)
		,	Pathname(InPathname)
		,	bIsUnfoundHashAnError(bInIsUnfoundHashAnError)
		,	bShouldDeleteBuffer(bInShouldDeleteBuffer)
	{
	}

	/**
	 * Performs the async hash verification
	 */
	void DoWork();

	/**
	 * Task API, return true to indicate that we can abandon
	 */
	bool CanAbandon()
	{
		return true;
	}

	/**
	 * Abandon task, deletes the buffer if that is what was requested
	 */
	void Abandon()
	{
		if( bShouldDeleteBuffer )
		{
			free( Buffer );
			Buffer = 0;
		}
	}

	//FORCEINLINE TStatId GetStatId() const
	//{
		//RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncSHAVerify, STATGROUP_ThreadPoolAsyncTasks);
	//}
};

/**
 * Callback that is called if the asynchronous SHA verification fails
 * This will be called from a pooled thread.
 *
 * NOTE: Each platform is expected to implement this!
 *
 * @param FailedPathname Pathname of file that failed to verify
 * @param bFailedDueToMissingHash true if the reason for the failure was that the hash was missing, and that was set as being an error condition
 */
 void appOnFailSHAVerification(const char* FailedPathname, bool bFailedDueToMissingHash);

/**
 * Similar to FBufferReader, but will verify the contents of the buffer on close (on close to that 
 * we know we don't need the data anymore)
 */
//class FBufferReaderWithSHA : public FBufferReaderBase
//{
//public:
//	/**
//	 * Constructor
//	 * 
//	 * @param Data Buffer to use as the source data to read from
//	 * @param Size Size of Data
//	 * @param bInFreeOnClose If true, Data will be FMemory::Free'd when this archive is closed
//	 * @param SHASourcePathname Path to the file to use to lookup the SHA hash value
//	 * @param bIsPersistent Uses this value for SetIsPersistent()
//	 * @param bInIsUnfoundHashAnError true if failing to lookup the hash should trigger an error (only in ShippingPC)
//	 */
//	FBufferReaderWithSHA( 
//		void* Data, 
//		int32_t Size, 
//		bool bInFreeOnClose, 
//		const char* SHASourcePathname, 
//		bool bIsPersistent=false, 
//		bool bInIsUnfoundHashAnError=false 
//		)
//	// we force the base class to NOT free buffer on close, as we will let the SHA task do it if needed
//	: FBufferReaderBase(Data, Size, bInFreeOnClose, bIsPersistent)
//	, SourcePathname(SHASourcePathname)
//	, bIsUnfoundHashAnError(bInIsUnfoundHashAnError)
//	{
//	}
//
//	~FBufferReaderWithSHA()
//	{
//		Close();
//	}
//
//	bool Close()
//	{
//		// don't redo if we were already closed
//		if (ReaderData)
//		{
//			// kick off an SHA verification task to verify. this will handle any errors we get
//			(new FAutoDeleteAsyncTask<FAsyncSHAVerify>(ReaderData, ReaderSize, bFreeOnClose, *SourcePathname, bIsUnfoundHashAnError))->StartBackgroundTask();
//			ReaderData = NULL;
//		}
//		
//		// note that we don't allow the base class CLose to happen, as the FAsyncSHAVerify will free the buffer if needed
//		return !ArIsError;
//	}
//	/**
//  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
//	 * is in when a loading error occurs.
//	 *
//	 * This is overridden for the specific Archive Types
//	 **/
//	virtual std::string GetArchiveName() const { return TEXT("FBufferReaderWithSHA"); }
//
//protected:
//	/** Path to the file to use to lookup the SHA hash value */
//	std::string SourcePathname;
//	/** true if failing to lookup the hash should trigger an error */
//	bool bIsUnfoundHashAnError;
//};
//


