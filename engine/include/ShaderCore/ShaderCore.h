#pragma once
#include "Engine/YCommonHeader.h"
#include <vector>
#include "Engine/PlatformMacro.h"
#include "Engine/YFile.h"

struct FBaseShaderResourceTable
{
	/** Bits indicating which resource tables contain resources bound to this shader. */
	uint32 ResourceTableBits;

	/** Mapping of bound SRVs to their location in resource tables. */
	std::vector<uint32> ShaderResourceViewMap;

	/** Mapping of bound sampler states to their location in resource tables. */
	std::vector<uint32> SamplerMap;

	/** Mapping of bound UAVs to their location in resource tables. */
	std::vector<uint32> UnorderedAccessViewMap;

	/** Hash of the layouts of resource tables at compile time, used for runtime validation. */
	std::vector<uint32> ResourceTableLayoutHashes;

	FBaseShaderResourceTable() :
		ResourceTableBits(0)
	{
	}

	friend bool operator==(const FBaseShaderResourceTable& A, const FBaseShaderResourceTable& B)
	{
		bool bEqual = true;
		bEqual &= (A.ResourceTableBits == B.ResourceTableBits);
		bEqual &= (A.ShaderResourceViewMap.size() == B.ShaderResourceViewMap.size());
		bEqual &= (A.SamplerMap.size() == B.SamplerMap.size());
		bEqual &= (A.UnorderedAccessViewMap.size() == B.UnorderedAccessViewMap.size());
		bEqual &= (A.ResourceTableLayoutHashes.size() == B.ResourceTableLayoutHashes.size());
		if (!bEqual)
		{
			return false;
		}
		bEqual &= (memcmp(A.ShaderResourceViewMap.data(), B.ShaderResourceViewMap.data(), sizeof(std::vector<uint32>::size_type) * A.ShaderResourceViewMap.size()) == 0);
		bEqual &= (memcmp(A.SamplerMap.data(), B.SamplerMap.data(), sizeof(uint32) * A.SamplerMap.size()) == 0);
		bEqual &= (memcmp(A.UnorderedAccessViewMap.data(), B.UnorderedAccessViewMap.data(), sizeof(uint32) * A.UnorderedAccessViewMap.size()) == 0);
		bEqual &= (memcmp(A.ResourceTableLayoutHashes.data(), B.ResourceTableLayoutHashes.data(), sizeof(uint32) * A.ResourceTableLayoutHashes.size()) == 0);
		return bEqual;
	}
};

// later we can transform that to the actual class passed around at the RHI level
class FShaderCodeReader
{
	const std::vector<uint8>& ShaderCode;

public:
	FShaderCodeReader(const std::vector<uint8>& InShaderCode)
		: ShaderCode(InShaderCode)
	{
		check(ShaderCode.size());
	}

	uint32 GetActualShaderCodeSize() const
	{
		return ShaderCode.size() - GetOptionalDataSize();
	}

	// for convenience
	template <class T>
	const T* FindOptionalData() const
	{
		return (const T*)FindOptionalData(T::Key, sizeof(T));
	}


	// @param InKey e.g. FShaderCodePackedResourceCounts::Key
	// @return 0 if not found
	const uint8* FindOptionalData(uint8 InKey, uint8 ValueSize) const
	{
		check(ValueSize);

		const uint8* End = &ShaderCode[0] + ShaderCode.size();

		int32 LocalOptionalDataSize = GetOptionalDataSize();

		const uint8* Start = End - LocalOptionalDataSize;
		// while searching don't include the optional data size
		End = End - sizeof(LocalOptionalDataSize);
		const uint8* Current = Start;

		while (Current < End)
		{
			uint8 Key = *Current++;
			uint32 Size = *((const uint32*)Current);
			Current += sizeof(Size);

			if (Key == InKey && Size == ValueSize)
			{
				return Current;
			}

			Current += Size;
		}

		return 0;
	}

	const char* FindOptionalData(uint8 InKey) const
	{
		check(ShaderCode.size() >= 4);

		const uint8* End = &ShaderCode[0] + ShaderCode.size();

		int32 LocalOptionalDataSize = GetOptionalDataSize();

		const uint8* Start = End - LocalOptionalDataSize;
		// while searching don't include the optional data size
		End = End - sizeof(LocalOptionalDataSize);
		const uint8* Current = Start;

		while (Current < End)
		{
			uint8 Key = *Current++;
			uint32 Size = *((const uint32*)Current);
			Current += sizeof(Size);

			if (Key == InKey)
			{
				return (char*)Current;
			}

			Current += Size;
		}

		return 0;
	}

	// Returns nullptr and Size -1 if not key was not found
	const uint8* FindOptionalDataAndSize(uint8 InKey, int32& OutSize) const
	{
		check(ShaderCode.size() >= 4);

		const uint8* End = &ShaderCode[0] + ShaderCode.size();

		int32 LocalOptionalDataSize = GetOptionalDataSize();

		const uint8* Start = End - LocalOptionalDataSize;
		// while searching don't include the optional data size
		End = End - sizeof(LocalOptionalDataSize);
		const uint8* Current = Start;

		while (Current < End)
		{
			uint8 Key = *Current++;
			uint32 Size = *((const uint32*)Current);
			Current += sizeof(Size);

			if (Key == InKey)
			{
				OutSize = Size;
				return Current;
			}

			Current += Size;
		}

		OutSize = -1;
		return nullptr;
	}

	int32 GetOptionalDataSize() const
	{
		if (ShaderCode.size() < sizeof(int32))
		{
			return 0;
		}

		const uint8* End = &ShaderCode[0] + ShaderCode.size();

		int32 LocalOptionalDataSize = ((const int32*)End)[-1];

		check(LocalOptionalDataSize >= 0);
		check(ShaderCode.size() >= LocalOptionalDataSize);

		return LocalOptionalDataSize;
	}

	int32 GetShaderCodeSize() const
	{
		return ShaderCode.size() - GetOptionalDataSize();
	}
};

class FShaderCode
{
	// -1 if ShaderData was finalized
	mutable int32 OptionalDataSize;
	// access through class methods
	mutable std::vector<uint8> ShaderCodeWithOptionalData;

public:

	FShaderCode()
		: OptionalDataSize(0)
	{
	}

	// adds CustomData or does nothing if that was already done before
	void FinalizeShaderCode() const
	{
		if (OptionalDataSize != -1)
		{
			OptionalDataSize += sizeof(OptionalDataSize);
			//ShaderCodeWithOptionalData.Append((const uint8*)&OptionalDataSize, sizeof(OptionalDataSize));
			ShaderCodeWithOptionalData.insert(ShaderCodeWithOptionalData.end(), (const uint8*)&OptionalDataSize, (const uint8*)&OptionalDataSize + sizeof(OptionalDataSize));
			OptionalDataSize = -1;
		}
	}

	// for write access
	std::vector<uint8>& GetWriteAccess()
	{
		return ShaderCodeWithOptionalData;
	}

	int32 GetShaderCodeSize() const
	{
		FinalizeShaderCode();

		FShaderCodeReader Wrapper(ShaderCodeWithOptionalData);

		return Wrapper.GetShaderCodeSize();
	}

	// inefficient, will/should be replaced by GetShaderCodeToRead()
	void GetShaderCodeLegacy(std::vector<uint8>& Out) const
	{
		//Out.Empty();
		Out.clear();

		//Out.AddUninitialized(GetShaderCodeSize());
		Out.resize(GetShaderCodeSize());
		memcpy(Out.data(), GetReadAccess().data(), ShaderCodeWithOptionalData.size());
	}

	// for read access, can have additional data attached to the end
	const std::vector<uint8>& GetReadAccess() const
	{
		FinalizeShaderCode();

		return ShaderCodeWithOptionalData;
	}

	// for convenience
	template <class T>
	void AddOptionalData(const T& In)
	{
		AddOptionalData(T::Key, (uint8*)&In, sizeof(T));
	}

	// Note: we don't hash the optional attachments in GenerateOutputHash() as they would prevent sharing (e.g. many material share the save VS)
	// can be called after the non optional data was stored in ShaderData
	// @param Key uint8 to save memory so max 255, e.g. FShaderCodePackedResourceCounts::Key
	// @param Size >0, only restriction is that sum of all optional data values must be < 4GB
	void AddOptionalData(uint8 Key, const uint8* ValuePtr, uint32 ValueSize)
	{
		check(ValuePtr);

		// don't add after Finalize happened
		check(OptionalDataSize >= 0);

		ShaderCodeWithOptionalData.push_back(Key);
		ShaderCodeWithOptionalData.insert(ShaderCodeWithOptionalData.end(),(const uint8*)&ValueSize, (const uint8*)&ValueSize+ sizeof(ValueSize));
		ShaderCodeWithOptionalData.insert(ShaderCodeWithOptionalData.end(), ValuePtr,ValuePtr + ValueSize);
		OptionalDataSize += sizeof(uint8) + sizeof(ValueSize) + (uint32)ValueSize;
	}

	// Note: we don't hash the optional attachments in GenerateOutputHash() as they would prevent sharing (e.g. many material share the save VS)
	// convenience, silently drops the data if string is too long
	// @param e.g. 'n' for the ShaderSourceFileName
	void AddOptionalData(uint8 Key, const char* InString)
	{
		uint32 Size =strlen(InString) + 1;
		AddOptionalData(Key, (uint8*)InString, Size);
	}
	friend MemoryFile& operator<<(MemoryFile& mem_file, FShaderCode& out_put)
	{
		if (mem_file.IsReading())
		{
			out_put.OptionalDataSize = -1;
		}
		else
		{
			out_put.FinalizeShaderCode();
		}
		mem_file << out_put.ShaderCodeWithOptionalData;
		return mem_file;
	}
	//friend FArchive& operator<<(FArchive& Ar, FShaderCode& Output)
	//{
	//	if (Ar.IsLoading())
	//	{
	//		Output.OptionalDataSize = -1;
	//	}
	//	else
	//	{
	//		Output.FinalizeShaderCode();
	//	}

	//	// Note: this serialize is used to pass between UE4 and the shader compile worker, recompile both when modifying
	//	Ar << Output.ShaderCodeWithOptionalData;
	//	return Ar;
	//}
};

