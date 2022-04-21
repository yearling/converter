#pragma once
#include <d3d11.h>
#include <stdint.h>
#include <stdlib.h>
#include "Math/YMath.h"
#include "Engine/TypeHash.h"
#include "Engine/YReferenceCount.h"
/**
 * Checks that the given result isn't a failure.  If it is, the application exits with an appropriate error message.
 * @param	Result - The result code to check.
 * @param	Code - The code which yielded the result.
 * @param	Filename - The filename of the source file containing Code.
 * @param	Line - The line number of Code within Filename.
 */
void VerifyD3D11Result(HRESULT Result, const char* Code, const char* Filename, uint32_t Line, ID3D11Device* Device);
/**
 * Checks that the given result isn't a failure.  If it is, the application exits with an appropriate error message.
 * @param	Shader - The shader we are trying to create.
 * @param	Result - The result code to check.
 * @param	Code - The code which yielded the result.
 * @param	Filename - The filename of the source file containing Code.
 * @param	Line - The line number of Code within Filename.
 * @param	Device - The D3D device used to create the shadr.
 */
void VerifyD3D11ShaderResult(class FRHIShader* Shader, HRESULT Result, const char* Code, const char* Filename, uint32 Line, ID3D11Device* Device);

/**
* Checks that the given result isn't a failure.  If it is, the application exits with an appropriate error message.
* @param	Result - The result code to check
* @param	Code - The code which yielded the result.
* @param	Filename - The filename of the source file containing Code.
* @param	Line - The line number of Code within Filename.
*/
void VerifyD3D11CreateTextureResult(HRESULT D3DResult, const char* Code, const char* Filename, uint32 Line,
	uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 D3DFormat, uint32 NumMips, uint32 Flags, ID3D11Device* Device);


void VerifyD3D11ResizeViewportResult(HRESULT D3DResult, const char* Code, const char* Filename, uint32 Line, uint32 SizeX, uint32 SizeY, uint8 D3DFormat, ID3D11Device* Device);

#define VERIFYD3D11RESULT_EX(x, Device)	{HRESULT hr = x; if (FAILED(hr)) { VerifyD3D11Result(hr,#x,__FILE__,__LINE__, Device); }}
#define VERIFYD3D11RESULT(x)			{HRESULT hr = x; if (FAILED(hr)) { VerifyD3D11Result(hr,#x,__FILE__,__LINE__, 0); }}
#define VERIFYD3D11SHADERRESULT(Result, Shader, Device) {HRESULT hr = (Result); if (FAILED(hr)) { VerifyD3D11ShaderResult(Shader, hr, #Result,__FILE__,__LINE__, Device); }}
#define VERIFYD3D11CREATETEXTURERESULT(x,SizeX,SizeY,SizeZ,Format,NumMips,Flags, Device) {HRESULT hr = x; if (FAILED(hr)) { VerifyD3D11CreateTextureResult(hr,#x,__FILE__,__LINE__,SizeX,SizeY,SizeZ,Format,NumMips,Flags, Device); }}
#define VERIFYD3D11RESIZEVIEWPORTRESULT(x,SizeX,SizeY,Format, Device) {HRESULT hr = x; if (FAILED(hr)) { VerifyD3D11ResizeViewportResult(hr,#x,__FILE__,__LINE__,SizeX,SizeY,Format, Device); }}
/**
 * Checks that a COM object has the expected number of references.
 */
void VerifyComRefCount(IUnknown* Object, int32 ExpectedRefs, const char* Code, const char* Filename, int32 Line);
#define checkComRefCount(Obj,ExpectedRefs) VerifyComRefCount(Obj,ExpectedRefs, #Obj , __FILE__,__LINE__)

const char* GetD3D11TextureFormatString(DXGI_FORMAT TextureFormat);
/**
 * Keeps track of Locks for D3D11 objects
 */
class FD3D11LockedKey
{
public:
	void* SourceObject;
	uint32 Subresource;

public:
	FD3D11LockedKey() : SourceObject(NULL)
		, Subresource(0)
	{}
	FD3D11LockedKey(ID3D11Texture2D* source, uint32 subres = 0) : SourceObject((void*)source)
		, Subresource(subres)
	{}
	FD3D11LockedKey(ID3D11Texture3D* source, uint32 subres = 0) : SourceObject((void*)source)
		, Subresource(subres)
	{}
	FD3D11LockedKey(ID3D11Buffer* source, uint32 subres = 0) : SourceObject((void*)source)
		, Subresource(subres)
	{}
	bool operator==(const FD3D11LockedKey& Other) const
	{
		return SourceObject == Other.SourceObject && Subresource == Other.Subresource;
	}
	bool operator!=(const FD3D11LockedKey& Other) const
	{
		return SourceObject != Other.SourceObject || Subresource != Other.Subresource;
	}
	FD3D11LockedKey& operator=(const FD3D11LockedKey& Other)
	{
		SourceObject = Other.SourceObject;
		Subresource = Other.Subresource;
		return *this;
	}
	uint32 GetHash() const
	{
		return PointerHash(SourceObject);
	}

	/** Hashing function. */
	friend uint32 GetTypeHash(const FD3D11LockedKey& K)
	{
		return K.GetHash();
	}
};

namespace std
{

	template <>
	struct hash<FD3D11LockedKey>
	{
		size_t operator()(const FD3D11LockedKey& k) const
		{
			size_t res = (size_t)GetTypeHash(k);
			return res;
		}
	};
}

/** Information about a D3D resource that is currently locked. */
struct FD3D11LockedData
{
	TRefCountPtr<ID3D11Resource> StagingResource;
	uint32 Pitch;
	uint32 DepthPitch;

	// constructor
	FD3D11LockedData()
		: bAllocDataWasUsed(false)
	{
	}

	// 16 byte alignment for best performance  (can be 30x faster than unaligned)
	void AllocData(uint32 Size)
	{
		//Data = (uint8*)FMemory::Malloc(Size, 16);
		Data = (uint8*)_aligned_malloc( Size, 16);
		bAllocDataWasUsed = true;
	}

	// Some driver might return aligned memory so we don't enforce the alignment
	void SetData(void* InData)
	{
		check(!bAllocDataWasUsed); Data = (uint8*)InData;
	}

	uint8* GetData() const
	{
		return Data;
	}

	// only call if AllocData() was used
	void FreeData()
	{
		check(bAllocDataWasUsed);
		_aligned_free (Data);
		Data = 0;
	}

private:
	//
	uint8* Data;
	// then FreeData
	bool bAllocDataWasUsed;
};
