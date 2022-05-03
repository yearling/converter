#include "Engine/YArchive.h"
#include "Math/YMath.h"

class MemoryArchive :public YArchive
{

public:
	int64 Tell() final
	{
		return offset;
	}


	void Seek(int64 InPos) final
	{
		offset = InPos;
	}
protected:
	MemoryArchive() :YArchive(),offset(0) {}
	int64 offset;
};

class YMemoryReader :public MemoryArchive
{
public:
	virtual int64 TotalSize() override
	{
		return YMath::Min((int64)Bytes.size(), LimitSize);
	}

	void Serialize(void* Data, int64 Num)
	{
		if (Num && !ArIsError)
		{
			// Only serialize if we have the requested amount of data
			if (offset + Num <= TotalSize())
			{
				memcpy(Data, &Bytes[offset], Num);
				offset += Num;
			}
			else
			{
				ArIsError = true;
			}
		}
	}

	explicit YMemoryReader(const std::vector<uint8>& InBytes)
		: Bytes(InBytes)
		, LimitSize(INT64_MAX)
	{
		this->SetIsReading(true);
	}

	/** With this method it's possible to attach data behind some serialized data. */
	void SetLimitSize(int64 NewLimitSize)
	{
		LimitSize = NewLimitSize;
	}
private:
	const std::vector<uint8>& Bytes;
	int64                LimitSize;
};

class FBufferReaderBase : public YArchive
{
public:
	/**
	* Constructor
	*
	* @param Data Buffer to use as the source data to read from
	* @param Size Size of Data
	* @param bInFreeOnClose If true, Data will be FMemory::Free'd when this archive is closed
	* @param bIsPersistent Uses this value for SetIsPersistent()
	* @param bInSHAVerifyOnClose It true, an async SHA verification will be done on the Data buffer (bInFreeOnClose will be passed on to the async task)
	*/
	FBufferReaderBase(void* Data, int64 Size, bool bInFreeOnClose)
		: ReaderData(Data)
		, ReaderPos(0)
		, ReaderSize(Size)
		, bFreeOnClose(bInFreeOnClose)
	{
		this->SetIsReading(true);
		//this->SetIsPersistent(bIsPersistent);
	}

	~FBufferReaderBase()
	{
		Close();
	}
	bool Close()
	{
		if (bFreeOnClose)
		{
			free(ReaderData);
			ReaderData = nullptr;
		}
		return !ArIsError;
	}
	void Serialize(void* Data, int64 Num) final
	{
		check(ReaderPos >= 0);
		check(ReaderPos + Num <= ReaderSize);
		memcpy(Data, (uint8*)ReaderData + ReaderPos, Num);
		ReaderPos += Num;
	}
	int64 Tell() final
	{
		return ReaderPos;
	}
	int64 TotalSize() final
	{
		return ReaderSize;
	}
	void Seek(int64 InPos) final
	{
		check(InPos >= 0);
		check(InPos <= ReaderSize);
		ReaderPos = InPos;
	}
	bool AtEnd() final
	{
		return ReaderPos >= ReaderSize;
	}
	/**
	* Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	* is in when a loading error occurs.
	*
	* This is overridden for the specific Archive Types
	**/
	//virtual FString GetArchiveName() const { return TEXT("FBufferReaderBase"); }
protected:
	void* ReaderData;
	int64		ReaderPos;
	int64		ReaderSize;
	bool	bFreeOnClose;
};

/**
 * Similar to FMemoryReader, but able to internally
 * manage the memory for the buffer.
 */
class FBufferReader final : public FBufferReaderBase
{
public:
	/**
	 * Constructor
	 *
	 * @param Data Buffer to use as the source data to read from
	 * @param Size Size of Data
	 * @param bInFreeOnClose If true, Data will be FMemory::Free'd when this archive is closed
	 * @param bIsPersistent Uses this value for SetIsPersistent()
	 * @param bInSHAVerifyOnClose It true, an async SHA verification will be done on the Data buffer (bInFreeOnClose will be passed on to the async task)
	 */
	FBufferReader(void* Data, int64 Size, bool bInFreeOnClose)
		: FBufferReaderBase(Data, Size, bInFreeOnClose)
	{
	}

	//virtual FString GetArchiveName() const { return TEXT("FBufferReader"); }
};