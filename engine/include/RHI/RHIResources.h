// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#pragma once
#include <cassert>
#include "Math/YColor.h"
#include "Platform/ThreadSafeCounter.h"
#include "RHI/RHIDefinitions.h"
#include "Engine/YPixelFormat.h"
#include "Engine/LockFreeList.h"
#include "Platform/YPlatformAtomics.h"
#include "Platform/Windows/YWindowsPlatformMisc.h"
#include "Utility/SecureHash.h"
#include "Engine/YReferenceCount.h"
#include "RHI/RHI.h"
#include "Math/IntPoint.h"
#include "Math/IntVector.h"
#include "Utility/Crc.h"
#include <array>

#define DISABLE_RHI_DEFFERED_DELETE 0

struct FClearValueBinding;
struct FRHIResourceInfo;
enum class EClearBinding;

/** The base type of RHI resources. */
class  FRHIResource
{
public:
	FRHIResource(bool InbDoNotDeferDelete = false)
		: MarkedForDelete(0)
		, bDoNotDeferDelete(InbDoNotDeferDelete)
		, bCommitted(true)
	{
	}
	virtual ~FRHIResource()
	{
		assert(PlatformNeedsExtraDeletionLatency() || (NumRefs.GetValue() == 0 && (CurrentlyDeleting == this || bDoNotDeferDelete || Bypass()))); // this should not have any outstanding refs
	}
	inline uint32_t AddRef() const
	{
		int32_t NewValue = NumRefs.Increment();
		assert(NewValue > 0);
		return uint32_t(NewValue);
	}
	inline uint32_t Release() const
	{
		int32_t NewValue = NumRefs.Decrement();
		if (NewValue == 0)
		{
			if (!DeferDelete())
			{
				delete this;
			}
			else
			{
				if (FPlatformAtomics::InterlockedCompareExchange(&MarkedForDelete, 1, 0) == 0)
				{
					PendingDeletes.Push(const_cast<FRHIResource*>(this));
				}
			}
		}
		assert(NewValue >= 0);
		return uint32_t(NewValue);
	}
	inline uint32_t GetRefCount() const
	{
		int32_t CurrentValue = NumRefs.GetValue();
		assert(CurrentValue >= 0);
		return uint32_t(CurrentValue);
	}
	void DoNoDeferDelete()
	{
		assert(!MarkedForDelete);
		bDoNotDeferDelete = true;
		FPlatformMisc::MemoryBarrier(); //t
		assert(!MarkedForDelete);
	}

	static void FlushPendingDeletes(bool bFlushDeferredDeletes = false);

	FORCEINLINE static bool PlatformNeedsExtraDeletionLatency()
	{
		return GRHINeedsExtraDeletionLatency && GIsRHIInitialized;
	}

	static bool Bypass();

	// Transient resource tracking
	// We do this at a high level so we can catch errors even when transient resources are not supported
	void SetCommitted(bool bInCommitted)
	{
		assert(IsInRenderingThread());
		bCommitted = bInCommitted;
	}
	bool IsCommitted() const
	{
		assert(IsInRenderingThread());
		return bCommitted;
	}

private:
	mutable FThreadSafeCounter NumRefs;
	mutable int32_t MarkedForDelete;
	bool bDoNotDeferDelete;
	bool bCommitted;

	static TLockFreePointerListUnordered<FRHIResource, PLATFORM_CACHE_LINE_SIZE> PendingDeletes;
	static FRHIResource* CurrentlyDeleting;

	FORCEINLINE bool DeferDelete() const
	{
#if DISABLE_RHI_DEFFERED_DELETE
		return false;
#else
		// Defer if GRHINeedsExtraDeletionLatency or we are doing threaded rendering (unless otherwise requested).
		return !bDoNotDeferDelete && (GRHINeedsExtraDeletionLatency || !Bypass());
#endif
	}

	// Some APIs don't do internal reference counting, so we have to wait an extra couple of frames before deleting resources
	// to ensure the GPU has completely finished with them. This avoids expensive fences, etc.
	struct ResourcesToDelete
	{
		ResourcesToDelete(uint32_t InFrameDeleted = 0)
			: FrameDeleted(InFrameDeleted)
		{

		}

		std::vector<FRHIResource*>	Resources;
		uint32_t					FrameDeleted;
	};

	static std::vector<ResourcesToDelete> DeferredDeletionQueue;
	static uint32_t CurrentFrame;
};


//
// State blocks
//

class FRHISamplerState : public FRHIResource {};
class FRHIRasterizerState : public FRHIResource
{
public:
	virtual bool GetInitializer(struct FRasterizerStateInitializerRHI& Init) { return false; }
};
class FRHIDepthStencilState : public FRHIResource
{
public:
	virtual bool GetInitializer(struct FDepthStencilStateInitializerRHI& Init) { return false; }
};
class FRHIBlendState : public FRHIResource
{
public:
	virtual bool GetInitializer(class FBlendStateInitializerRHI& Init) { return false; }
};

//
// Shader bindings
//

typedef std::vector<struct FVertexElement> FVertexDeclarationElementList;
class FRHIVertexDeclaration : public FRHIResource
{
public:
	virtual bool GetInitializer(FVertexDeclarationElementList& Init) { return false; }
};

class FRHIBoundShaderState : public FRHIResource {};

//
// Shaders
//

class FRHIShader : public FRHIResource
{
public:
	void SetHash(FSHAHash InHash) { Hash = InHash; }
	FSHAHash GetHash() const { return Hash; }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// for debugging only e.g. MaterialName:ShaderFile.usf or ShaderFile.usf/EntryFunc
	std::string ShaderName;
#endif

private:
	FSHAHash Hash;
};

class FRHIVertexShader : public FRHIShader {};
class FRHIHullShader : public FRHIShader {};
class FRHIDomainShader : public FRHIShader {};
class FRHIPixelShader : public FRHIShader {};
class FRHIGeometryShader : public FRHIShader {};
class  FRHIComputeShader : public FRHIShader
{
public:
	FRHIComputeShader() : Stats(nullptr) {}

	inline void SetStats(struct FPipelineStateStats* Ptr) { Stats = Ptr; }
	void UpdateStats();

private:
	struct FPipelineStateStats* Stats;
};

//
// Pipeline States
//

class FRHIGraphicsPipelineState : public FRHIResource {};
class FRHIComputePipelineState : public FRHIResource {};

//
// Buffers
//

/** The layout of a uniform buffer in memory. */
struct FRHIUniformBufferLayout
{
	/** The size of the constant buffer in bytes. */
	uint32_t ConstantBufferSize;
	/** Byte offset to each resource in the uniform buffer memory. */
	std::vector<uint16_t> ResourceOffsets;
	/** The type of each resource (EUniformBufferBaseType). */
	std::vector<uint8_t> Resources;

	inline uint32_t GetHash() const
	{
		assert(Hash != 0);
		return Hash;
	}

	void ComputeHash()
	{
		uint32_t TmpHash = ConstantBufferSize << 16;

		for (int32_t ResourceIndex = 0; ResourceIndex < ResourceOffsets.size(); ResourceIndex++)
		{
			// Offset and therefore hash must be the same regardless of pointer size
			assert(ResourceOffsets[ResourceIndex] == YMath::Align(ResourceOffsets[ResourceIndex], 8));
			TmpHash ^= ResourceOffsets[ResourceIndex];
		}

		uint32_t N = Resources.size();
		while (N >= 4)
		{
			TmpHash ^= (Resources[--N] << 0);
			TmpHash ^= (Resources[--N] << 8);
			TmpHash ^= (Resources[--N] << 16);
			TmpHash ^= (Resources[--N] << 24);
		}
		while (N >= 2)
		{
			TmpHash ^= Resources[--N] << 0;
			TmpHash ^= Resources[--N] << 16;
		}
		while (N > 0)
		{
			TmpHash ^= Resources[--N];
		}
		Hash = TmpHash;
	}

	explicit FRHIUniformBufferLayout(std::string InName) :
		ConstantBufferSize(0),
		Name(InName),
		Hash(0)
	{
	}

	enum EInit
	{
		Zero
	};
	explicit FRHIUniformBufferLayout(EInit) :
		ConstantBufferSize(0),
		Name(""),
		Hash(0)
	{
	}

	void CopyFrom(const FRHIUniformBufferLayout& Source)
	{
		ConstantBufferSize = Source.ConstantBufferSize;
		ResourceOffsets = Source.ResourceOffsets;
		Resources = Source.Resources;
		Name = Source.Name;
		Hash = Source.Hash;
	}

	const std::string GetDebugName() const { return Name; }

private:
	// for debugging / error message
	std::string Name;

	uint32_t Hash;
};

/** Compare two uniform buffer layouts. */
inline bool operator==(const FRHIUniformBufferLayout& A, const FRHIUniformBufferLayout& B)
{
	return A.ConstantBufferSize == B.ConstantBufferSize
		&& A.ResourceOffsets == B.ResourceOffsets
		&& A.Resources == B.Resources;
}

class FRHIUniformBuffer : public FRHIResource
{
public:

	/** Initialization constructor. */
	FRHIUniformBuffer(const FRHIUniformBufferLayout& InLayout)
		: Layout(&InLayout)
		, LayoutConstantBufferSize(InLayout.ConstantBufferSize)
	{}

	/** @return The number of bytes in the uniform buffer. */
	uint32_t GetSize() const
	{
		assert(LayoutConstantBufferSize == Layout->ConstantBufferSize);
		return LayoutConstantBufferSize;
	}
	const FRHIUniformBufferLayout& GetLayout() const { return *Layout; }

private:
	/** Layout of the uniform buffer. */
	const FRHIUniformBufferLayout* Layout;

	uint32_t LayoutConstantBufferSize;
};

class FRHIIndexBuffer : public FRHIResource
{
public:

	/** Initialization constructor. */
	FRHIIndexBuffer(uint32_t InStride, uint32_t InSize, uint32_t InUsage)
		: Stride(InStride)
		, Size(InSize)
		, Usage(InUsage)
	{}

	/** @return The stride in bytes of the index buffer; must be 2 or 4. */
	uint32_t GetStride() const { return Stride; }

	/** @return The number of bytes in the index buffer. */
	uint32_t GetSize() const { return Size; }

	/** @return The usage flags used to create the index buffer. */
	uint32_t GetUsage() const { return Usage; }

private:
	uint32_t Stride;
	uint32_t Size;
	uint32_t Usage;
};

class FRHIVertexBuffer : public FRHIResource
{
public:

	/**
	 * Initialization constructor.
	 * @apram InUsage e.g. BUF_UnorderedAccess
	 */
	FRHIVertexBuffer(uint32_t InSize, uint32_t InUsage)
		: Size(InSize)
		, Usage(InUsage)
	{}

	/** @return The number of bytes in the vertex buffer. */
	uint32_t GetSize() const { return Size; }

	/** @return The usage flags used to create the vertex buffer. e.g. BUF_UnorderedAccess */
	uint32_t GetUsage() const { return Usage; }

private:
	uint32_t Size;
	// e.g. BUF_UnorderedAccess
	uint32_t Usage;
};

class FRHIStructuredBuffer : public FRHIResource
{
public:

	/** Initialization constructor. */
	FRHIStructuredBuffer(uint32_t InStride, uint32_t InSize, uint32_t InUsage)
		: Stride(InStride)
		, Size(InSize)
		, Usage(InUsage)
	{}

	/** @return The stride in bytes of the structured buffer; must be 2 or 4. */
	uint32_t GetStride() const { return Stride; }

	/** @return The number of bytes in the structured buffer. */
	uint32_t GetSize() const { return Size; }

	/** @return The usage flags used to create the structured buffer. */
	uint32_t GetUsage() const { return Usage; }

private:
	uint32_t Stride;
	uint32_t Size;
	uint32_t Usage;
};

//
// Textures
//

class  FLastRenderTimeContainer
{
public:
	FLastRenderTimeContainer() : LastRenderTime(-FLT_MAX) {}

	double GetLastRenderTime() const { return LastRenderTime; }
	inline void SetLastRenderTime(double InLastRenderTime)
	{
		// avoid dirty caches from redundant writes
		if (LastRenderTime != InLastRenderTime)
		{
			LastRenderTime = InLastRenderTime;
		}
	}

private:
	/** The last time the resource was rendered. */
	double LastRenderTime;
};

class  FRHITexture : public FRHIResource
{
public:

	/** Initialization constructor. */
	FRHITexture(uint32_t InNumMips, uint32_t InNumSamples, EPixelFormat InFormat, uint32_t InFlags, FLastRenderTimeContainer* InLastRenderTime, const FClearValueBinding& InClearValue)
		: ClearValue(InClearValue)
		, NumMips(InNumMips)
		, NumSamples(InNumSamples)
		, Format(InFormat)
		, Flags(InFlags)
		, LastRenderTime(InLastRenderTime ? *InLastRenderTime : DefaultLastRenderTime)
	{}

	// Dynamic cast methods.
	virtual class FRHITexture2D* GetTexture2D() { return NULL; }
	virtual class FRHITexture2DArray* GetTexture2DArray() { return NULL; }
	virtual class FRHITexture3D* GetTexture3D() { return NULL; }
	virtual class FRHITextureCube* GetTextureCube() { return NULL; }
	virtual class FRHITextureReference* GetTextureReference() { return NULL; }

	// Slower method to get Size X, Y & Z information. Prefer sub-classes' GetSizeX(), etc
	virtual FIntVector GetSizeXYZ() const = 0;

	/**
	 * Returns access to the platform-specific native resource pointer.  This is designed to be used to provide plugins with access
	 * to the underlying resource and should be used very carefully or not at all.
	 *
	 * @return	The pointer to the native resource or NULL if it not initialized or not supported for this resource type for some reason
	 */
	virtual void* GetNativeResource() const
	{
		// Override this in derived classes to expose access to the native texture resource
		return nullptr;
	}

	/**
	 * Returns access to the platform-specific native shader resource view pointer.  This is designed to be used to provide plugins with access
	 * to the underlying resource and should be used very carefully or not at all.
	 *
	 * @return	The pointer to the native resource or NULL if it not initialized or not supported for this resource type for some reason
	 */
	virtual void* GetNativeShaderResourceView() const
	{
		// Override this in derived classes to expose access to the native texture resource
		return nullptr;
	}

	/**
	 * Returns access to the platform-specific RHI texture baseclass.  This is designed to provide the RHI with fast access to its base classes in the face of multiple inheritance.
	 * @return	The pointer to the platform-specific RHI texture baseclass or NULL if it not initialized or not supported for this RHI
	 */
	virtual void* GetTextureBaseRHI()
	{
		// Override this in derived classes to expose access to the native texture resource
		return nullptr;
	}

	/** @return The number of mip-maps in the texture. */
	uint32_t GetNumMips() const { return NumMips; }

	/** @return The format of the pixels in the texture. */
	EPixelFormat GetFormat() const { return Format; }

	/** @return The flags used to create the texture. */
	uint32_t GetFlags() const { return Flags; }

	/* @return the number of samples for multi-sampling. */
	uint32_t GetNumSamples() const { return NumSamples; }

	/** @return Whether the texture is multi sampled. */
	bool IsMultisampled() const { return NumSamples > 1; }

	FRHIResourceInfo ResourceInfo;

	/** sets the last time this texture was cached in a resource table. */
	inline void SetLastRenderTime(float InLastRenderTime)
	{
		LastRenderTime.SetLastRenderTime(InLastRenderTime);
	}

	/** Returns the last render time container, or NULL if none were specified at creation. */
	FLastRenderTimeContainer* GetLastRenderTimeContainer()
	{
		if (&LastRenderTime == &DefaultLastRenderTime)
		{
			return NULL;
		}
		return &LastRenderTime;
	}

	void SetName(const std::string& InName)
	{
		TextureName = InName;
	}

	std::string GetName() const
	{
		return TextureName;
	}

	bool HasClearValue() const
	{
		return ClearValue.ColorBinding != EClearBinding::ENoneBound;
	}

	FLinearColor GetClearColor() const
	{
		return ClearValue.GetClearColor();
	}

	void GetDepthStencilClearValue(float& OutDepth, uint32_t& OutStencil) const
	{
		return ClearValue.GetDepthStencil(OutDepth, OutStencil);
	}

	float GetDepthClearValue() const
	{
		float Depth;
		uint32_t Stencil;
		ClearValue.GetDepthStencil(Depth, Stencil);
		return Depth;
	}

	uint32_t GetStencilClearValue() const
	{
		float Depth;
		uint32_t Stencil;
		ClearValue.GetDepthStencil(Depth, Stencil);
		return Stencil;
	}

	const FClearValueBinding GetClearBinding() const
	{
		return ClearValue;
	}

private:
	FClearValueBinding ClearValue;
	uint32_t NumMips;
	uint32_t NumSamples;
	EPixelFormat Format;
	uint32_t Flags;
	FLastRenderTimeContainer& LastRenderTime;
	FLastRenderTimeContainer DefaultLastRenderTime;
	std::string TextureName;
};

class  FRHITexture2D : public FRHITexture
{
public:

	/** Initialization constructor. */
	FRHITexture2D(uint32_t InSizeX, uint32_t InSizeY, uint32_t InNumMips, uint32_t InNumSamples, EPixelFormat InFormat, uint32_t InFlags, const FClearValueBinding& InClearValue)
		: FRHITexture(InNumMips, InNumSamples, InFormat, InFlags, NULL, InClearValue)
		, SizeX(InSizeX)
		, SizeY(InSizeY)
	{}

	// Dynamic cast methods.
	virtual FRHITexture2D* GetTexture2D() { return this; }

	/** @return The width of the texture. */
	uint32_t GetSizeX() const { return SizeX; }

	/** @return The height of the texture. */
	uint32_t GetSizeY() const { return SizeY; }

	inline FIntPoint GetSizeXY() const
	{
		return FIntPoint(SizeX, SizeY);
	}

	virtual FIntVector GetSizeXYZ() const final override
	{
		return FIntVector(SizeX, SizeY, 1);
	}

private:

	uint32_t SizeX;
	uint32_t SizeY;
};

class  FRHITexture2DArray : public FRHITexture
{
public:

	/** Initialization constructor. */
	FRHITexture2DArray(uint32_t InSizeX, uint32_t InSizeY, uint32_t InSizeZ, uint32_t InNumMips, EPixelFormat InFormat, uint32_t InFlags, const FClearValueBinding& InClearValue)
		: FRHITexture(InNumMips, 1, InFormat, InFlags, NULL, InClearValue)
		, SizeX(InSizeX)
		, SizeY(InSizeY)
		, SizeZ(InSizeZ)
	{}

	// Dynamic cast methods.
	virtual FRHITexture2DArray* GetTexture2DArray() { return this; }

	/** @return The width of the textures in the array. */
	uint32_t GetSizeX() const { return SizeX; }

	/** @return The height of the texture in the array. */
	uint32_t GetSizeY() const { return SizeY; }

	/** @return The number of textures in the array. */
	uint32_t GetSizeZ() const { return SizeZ; }

	virtual FIntVector GetSizeXYZ() const final override
	{
		return FIntVector(SizeX, SizeY, SizeZ);
	}

private:

	uint32_t SizeX;
	uint32_t SizeY;
	uint32_t SizeZ;
};

class  FRHITexture3D : public FRHITexture
{
public:

	/** Initialization constructor. */
	FRHITexture3D(uint32_t InSizeX, uint32_t InSizeY, uint32_t InSizeZ, uint32_t InNumMips, EPixelFormat InFormat, uint32_t InFlags, const FClearValueBinding& InClearValue)
		: FRHITexture(InNumMips, 1, InFormat, InFlags, NULL, InClearValue)
		, SizeX(InSizeX)
		, SizeY(InSizeY)
		, SizeZ(InSizeZ)
	{}

	// Dynamic cast methods.
	virtual FRHITexture3D* GetTexture3D() { return this; }

	/** @return The width of the texture. */
	uint32_t GetSizeX() const { return SizeX; }

	/** @return The height of the texture. */
	uint32_t GetSizeY() const { return SizeY; }

	/** @return The depth of the texture. */
	uint32_t GetSizeZ() const { return SizeZ; }

	virtual FIntVector GetSizeXYZ() const final override
	{
		return FIntVector(SizeX, SizeY, SizeZ);
	}

private:

	uint32_t SizeX;
	uint32_t SizeY;
	uint32_t SizeZ;
};

class  FRHITextureCube : public FRHITexture
{
public:

	/** Initialization constructor. */
	FRHITextureCube(uint32_t InSize, uint32_t InNumMips, EPixelFormat InFormat, uint32_t InFlags, const FClearValueBinding& InClearValue)
		: FRHITexture(InNumMips, 1, InFormat, InFlags, NULL, InClearValue)
		, Size(InSize)
	{}

	// Dynamic cast methods.
	virtual FRHITextureCube* GetTextureCube() { return this; }

	/** @return The width and height of each face of the cubemap. */
	uint32_t GetSize() const { return Size; }

	virtual FIntVector GetSizeXYZ() const final override
	{
		return FIntVector(Size, Size, 1);
	}

private:

	uint32_t Size;
};

class  FRHITextureReference : public FRHITexture
{
public:
	explicit FRHITextureReference(FLastRenderTimeContainer* InLastRenderTime)
		: FRHITexture(0, 0, PF_Unknown, 0, InLastRenderTime, FClearValueBinding())
	{}

	virtual FRHITextureReference* GetTextureReference() override { return this; }
	inline FRHITexture* GetReferencedTexture() const { return ReferencedTexture.GetReference(); }

	void SetReferencedTexture(FRHITexture* InTexture)
	{
		ReferencedTexture = InTexture;
	}

	virtual FIntVector GetSizeXYZ() const final override
	{
		if (ReferencedTexture)
		{
			return ReferencedTexture->GetSizeXYZ();
		}
		return FIntVector(0, 0, 0);
	}

private:
	TRefCountPtr<FRHITexture> ReferencedTexture;
};

class  FRHITextureReferenceNullImpl : public FRHITextureReference
{
public:
	FRHITextureReferenceNullImpl()
		: FRHITextureReference(NULL)
	{}

	void SetReferencedTexture(FRHITexture* InTexture)
	{
		FRHITextureReference::SetReferencedTexture(InTexture);
	}
};

//
// Misc
//



/* Generic GPU fence class used by FRHIGPUMemoryReadback and FRHIGPUMemoryUpdate
* RHI specific fences derive from this
*/
class FRHIGPUFence : public FRHIResource
{
public:
	FRHIGPUFence(std::string InName)
		: FenceName(InName)
	{}

	virtual ~FRHIGPUFence()
	{}

	virtual bool Write()
	{
		return false;
	};

	virtual bool Poll() const
	{
		return false;
	};

	virtual bool Wait(float TimeoutMs) const
	{
		return false;
	};

private:
	std::string FenceName;
};


class FRHIRenderQuery : public FRHIResource {};

class FRHIComputeFence : public FRHIResource
{
public:

	FRHIComputeFence(std::string InName)
		: Name(InName)
		, bWriteEnqueued(false)
	{}

	FORCEINLINE std::string GetName() const
	{
		return Name;
	}

	FORCEINLINE bool GetWriteEnqueued() const
	{
		return bWriteEnqueued;
	}

	virtual void Reset()
	{
		bWriteEnqueued = false;
	}

	virtual void WriteFence()
	{
		//ensureMsgf(!bWriteEnqueued, TEXT("ComputeFence: %s already written this frame. You should use a new label"), *Name.ToString());
		ERROR_INFO("ComputeFence:", Name, "already written this frame.You should use a new label");
		bWriteEnqueued = true;
	}

private:
	//debug name of the label.
	std::string Name;

	//has the label been written to since being created.
	//assert this when queuing waits to catch GPU hangs on the CPU at command creation time.
	bool bWriteEnqueued;
};

class FRHIViewport : public FRHIResource
{
public:
	/**
	 * Returns access to the platform-specific native resource pointer.  This is designed to be used to provide plugins with access
	 * to the underlying resource and should be used very carefully or not at all.
	 *
	 * @return	The pointer to the native resource or NULL if it not initialized or not supported for this resource type for some reason
	 */
	virtual void* GetNativeSwapChain() const { return nullptr; }
	/**
	 * Returns access to the platform-specific native resource pointer to a backbuffer texture.  This is designed to be used to provide plugins with access
	 * to the underlying resource and should be used very carefully or not at all.
	 *
	 * @return	The pointer to the native resource or NULL if it not initialized or not supported for this resource type for some reason
	 */
	virtual void* GetNativeBackBufferTexture() const { return nullptr; }
	/**
	 * Returns access to the platform-specific native resource pointer to a backbuffer rendertarget. This is designed to be used to provide plugins with access
	 * to the underlying resource and should be used very carefully or not at all.
	 *
	 * @return	The pointer to the native resource or NULL if it not initialized or not supported for this resource type for some reason
	 */
	virtual void* GetNativeBackBufferRT() const { return nullptr; }

	/**
	 * Returns access to the platform-specific native window. This is designed to be used to provide plugins with access
	 * to the underlying resource and should be used very carefully or not at all.
	 *
	 * @return	The pointer to the native resource or NULL if it not initialized or not supported for this resource type for some reason.
	 * AddParam could represent any additional platform-specific data (could be null).
	 */
	virtual void* GetNativeWindow(void** AddParam = nullptr) const { return nullptr; }

	/**
	 * Sets custom Present handler on the viewport
	 */
	virtual void SetCustomPresent(class FRHICustomPresent*) {}

	/**
	 * Returns currently set custom present handler.
	 */
	virtual class FRHICustomPresent* GetCustomPresent() const { return nullptr; }
};

//
// Views
//

class FRHIUnorderedAccessView : public FRHIResource {};
class FRHIShaderResourceView : public FRHIResource {};



typedef FRHISamplerState* FSamplerStateRHIParamRef;
typedef TRefCountPtr<FRHISamplerState> FSamplerStateRHIRef;

typedef FRHIRasterizerState* FRasterizerStateRHIParamRef;
typedef TRefCountPtr<FRHIRasterizerState> FRasterizerStateRHIRef;

typedef FRHIDepthStencilState* FDepthStencilStateRHIParamRef;
typedef TRefCountPtr<FRHIDepthStencilState> FDepthStencilStateRHIRef;

typedef FRHIBlendState* FBlendStateRHIParamRef;
typedef TRefCountPtr<FRHIBlendState> FBlendStateRHIRef;

typedef FRHIVertexDeclaration* FVertexDeclarationRHIParamRef;
typedef TRefCountPtr<FRHIVertexDeclaration> FVertexDeclarationRHIRef;

typedef FRHIVertexShader* FVertexShaderRHIParamRef;
typedef TRefCountPtr<FRHIVertexShader> FVertexShaderRHIRef;

typedef FRHIHullShader* FHullShaderRHIParamRef;
typedef TRefCountPtr<FRHIHullShader> FHullShaderRHIRef;

typedef FRHIDomainShader* FDomainShaderRHIParamRef;
typedef TRefCountPtr<FRHIDomainShader> FDomainShaderRHIRef;

typedef FRHIPixelShader* FPixelShaderRHIParamRef;
typedef TRefCountPtr<FRHIPixelShader> FPixelShaderRHIRef;

typedef FRHIGeometryShader* FGeometryShaderRHIParamRef;
typedef TRefCountPtr<FRHIGeometryShader> FGeometryShaderRHIRef;

typedef FRHIComputeShader* FComputeShaderRHIParamRef;
typedef TRefCountPtr<FRHIComputeShader> FComputeShaderRHIRef;

typedef FRHIComputeFence* FComputeFenceRHIParamRef;
typedef TRefCountPtr<FRHIComputeFence>	FComputeFenceRHIRef;

typedef FRHIBoundShaderState* FBoundShaderStateRHIParamRef;
typedef TRefCountPtr<FRHIBoundShaderState> FBoundShaderStateRHIRef;

typedef FRHIUniformBuffer* FUniformBufferRHIParamRef;
typedef TRefCountPtr<FRHIUniformBuffer> FUniformBufferRHIRef;

typedef FRHIIndexBuffer* FIndexBufferRHIParamRef;
typedef TRefCountPtr<FRHIIndexBuffer> FIndexBufferRHIRef;

typedef FRHIVertexBuffer* FVertexBufferRHIParamRef;
typedef TRefCountPtr<FRHIVertexBuffer> FVertexBufferRHIRef;

typedef FRHIStructuredBuffer* FStructuredBufferRHIParamRef;
typedef TRefCountPtr<FRHIStructuredBuffer> FStructuredBufferRHIRef;

typedef FRHITexture* FTextureRHIParamRef;
typedef TRefCountPtr<FRHITexture> FTextureRHIRef;

typedef FRHITexture2D* FTexture2DRHIParamRef;
typedef TRefCountPtr<FRHITexture2D> FTexture2DRHIRef;

typedef FRHITexture2DArray* FTexture2DArrayRHIParamRef;
typedef TRefCountPtr<FRHITexture2DArray> FTexture2DArrayRHIRef;

typedef FRHITexture3D* FTexture3DRHIParamRef;
typedef TRefCountPtr<FRHITexture3D> FTexture3DRHIRef;

typedef FRHITextureCube* FTextureCubeRHIParamRef;
typedef TRefCountPtr<FRHITextureCube> FTextureCubeRHIRef;

typedef FRHITextureReference* FTextureReferenceRHIParamRef;
typedef TRefCountPtr<FRHITextureReference> FTextureReferenceRHIRef;

typedef FRHIRenderQuery* FRenderQueryRHIParamRef;
typedef TRefCountPtr<FRHIRenderQuery> FRenderQueryRHIRef;

typedef FRHIGPUFence* FGPUFenceRHIParamRef;
typedef TRefCountPtr<FRHIGPUFence>	FGPUFenceRHIRef;

typedef FRHIViewport* FViewportRHIParamRef;
typedef TRefCountPtr<FRHIViewport> FViewportRHIRef;

typedef FRHIUnorderedAccessView* FUnorderedAccessViewRHIParamRef;
typedef TRefCountPtr<FRHIUnorderedAccessView> FUnorderedAccessViewRHIRef;

typedef FRHIShaderResourceView* FShaderResourceViewRHIParamRef;
typedef TRefCountPtr<FRHIShaderResourceView> FShaderResourceViewRHIRef;

typedef FRHIGraphicsPipelineState* FGraphicsPipelineStateRHIParamRef;
typedef TRefCountPtr<FRHIGraphicsPipelineState> FGraphicsPipelineStateRHIRef;


/* Generic staging buffer class used by FRHIGPUMemoryReadback and FRHIGPUMemoryUpdate
* RHI specific staging buffers derive from this
*/
class FRHIStagingBuffer : public FRHIResource
{
public:
	FRHIStagingBuffer()
		:MappedPtr(nullptr)
		, LastLockedBuffer(nullptr)
	{
	}

	virtual void* Lock(FVertexBufferRHIRef GPUBuffer, uint32_t Offset, uint32_t NumBytes, EResourceLockMode LockMode)   // copyresource, map, return ptr
	{
		return MappedPtr;
	}

	virtual void Unlock()	// unmap, free memory
	{
	}


protected:
	// pointer to mapped buffer; null if unmapped
	void* MappedPtr;
	FVertexBufferRHIParamRef LastLockedBuffer;
};

typedef FRHIStagingBuffer* FStagingBufferRHIParamRef;
typedef TRefCountPtr<FRHIStagingBuffer>	FStagingBufferRHIRef;


class FRHIRenderTargetView
{
public:
	FTextureRHIParamRef Texture;
	uint32_t MipIndex;

	/** Array slice or texture cube face.  Only valid if texture resource was created with TexCreate_TargetArraySlicesIndependently! */
	uint32_t ArraySliceIndex;

	ERenderTargetLoadAction LoadAction;
	ERenderTargetStoreAction StoreAction;

	FRHIRenderTargetView() :
		Texture(NULL),
		MipIndex(0),
		ArraySliceIndex(-1),
		LoadAction(ERenderTargetLoadAction::ENoAction),
		StoreAction(ERenderTargetStoreAction::ENoAction)
	{}

	FRHIRenderTargetView(const FRHIRenderTargetView& Other) :
		Texture(Other.Texture),
		MipIndex(Other.MipIndex),
		ArraySliceIndex(Other.ArraySliceIndex),
		LoadAction(Other.LoadAction),
		StoreAction(Other.StoreAction)
	{}

	//common case
	explicit FRHIRenderTargetView(FTextureRHIParamRef InTexture, ERenderTargetLoadAction InLoadAction) :
		Texture(InTexture),
		MipIndex(0),
		ArraySliceIndex(-1),
		LoadAction(InLoadAction),
		StoreAction(ERenderTargetStoreAction::EStore)
	{}

	//common case
	explicit FRHIRenderTargetView(FTextureRHIParamRef InTexture, ERenderTargetLoadAction InLoadAction, uint32_t InMipIndex, uint32_t InArraySliceIndex) :
		Texture(InTexture),
		MipIndex(InMipIndex),
		ArraySliceIndex(InArraySliceIndex),
		LoadAction(InLoadAction),
		StoreAction(ERenderTargetStoreAction::EStore)
	{}

	explicit FRHIRenderTargetView(FTextureRHIParamRef InTexture, uint32_t InMipIndex, uint32_t InArraySliceIndex, ERenderTargetLoadAction InLoadAction, ERenderTargetStoreAction InStoreAction) :
		Texture(InTexture),
		MipIndex(InMipIndex),
		ArraySliceIndex(InArraySliceIndex),
		LoadAction(InLoadAction),
		StoreAction(InStoreAction)
	{}

	bool operator==(const FRHIRenderTargetView& Other) const
	{
		return
			Texture == Other.Texture &&
			MipIndex == Other.MipIndex &&
			ArraySliceIndex == Other.ArraySliceIndex &&
			LoadAction == Other.LoadAction &&
			StoreAction == Other.StoreAction;
	}
};

class FExclusiveDepthStencil
{
public:
	enum Type
	{
		// don't use those directly, use the combined versions below
		// 4 bits are used for depth and 4 for stencil to make the hex value readable and non overlapping
		DepthNop = 0x00,
		DepthRead = 0x01,
		DepthWrite = 0x02,
		DepthMask = 0x0f,
		StencilNop = 0x00,
		StencilRead = 0x10,
		StencilWrite = 0x20,
		StencilMask = 0xf0,

		// use those:
		DepthNop_StencilNop = DepthNop + StencilNop,
		DepthRead_StencilNop = DepthRead + StencilNop,
		DepthWrite_StencilNop = DepthWrite + StencilNop,
		DepthNop_StencilRead = DepthNop + StencilRead,
		DepthRead_StencilRead = DepthRead + StencilRead,
		DepthWrite_StencilRead = DepthWrite + StencilRead,
		DepthNop_StencilWrite = DepthNop + StencilWrite,
		DepthRead_StencilWrite = DepthRead + StencilWrite,
		DepthWrite_StencilWrite = DepthWrite + StencilWrite,
	};

private:
	Type Value;

public:
	// constructor
	FExclusiveDepthStencil(Type InValue = DepthNop_StencilNop)
		: Value(InValue)
	{
	}

	inline bool IsUsingDepthStencil() const
	{
		return Value != DepthNop_StencilNop;
	}
	inline bool IsUsingDepth() const
	{
		return (ExtractDepth() != DepthNop);
	}
	inline bool IsUsingStencil() const
	{
		return (ExtractStencil() != StencilNop);
	}
	inline bool IsDepthWrite() const
	{
		return ExtractDepth() == DepthWrite;
	}
	inline bool IsStencilWrite() const
	{
		return ExtractStencil() == StencilWrite;
	}

	inline bool IsAnyWrite() const
	{
		return IsDepthWrite() || IsStencilWrite();
	}

	inline void SetDepthWrite()
	{
		Value = (Type)(ExtractStencil() | DepthWrite);
	}
	inline void SetStencilWrite()
	{
		Value = (Type)(ExtractDepth() | StencilWrite);
	}
	inline void SetDepthStencilWrite(bool bDepth, bool bStencil)
	{
		Value = DepthNop_StencilNop;

		if (bDepth)
		{
			SetDepthWrite();
		}
		if (bStencil)
		{
			SetStencilWrite();
		}
	}
	bool operator==(const FExclusiveDepthStencil& rhs) const
	{
		return Value == rhs.Value;
	}

	bool operator != (const FExclusiveDepthStencil& RHS) const
	{
		return Value != RHS.Value;
	}

	inline bool IsValid(FExclusiveDepthStencil& Current) const
	{
		Type Depth = ExtractDepth();

		if (Depth != DepthNop && Depth != Current.ExtractDepth())
		{
			return false;
		}

		Type Stencil = ExtractStencil();

		if (Stencil != StencilNop && Stencil != Current.ExtractStencil())
		{
			return false;
		}

		return true;
	}

	uint32_t GetIndex() const
	{
		// Note: The array to index has views created in that specific order.

		// we don't care about the Nop versions so less views are needed
		// we combine Nop and Write
		switch (Value)
		{
		case DepthWrite_StencilNop:
		case DepthNop_StencilWrite:
		case DepthWrite_StencilWrite:
		case DepthNop_StencilNop:
			return 0; // old DSAT_Writable

		case DepthRead_StencilNop:
		case DepthRead_StencilWrite:
			return 1; // old DSAT_ReadOnlyDepth

		case DepthNop_StencilRead:
		case DepthWrite_StencilRead:
			return 2; // old DSAT_ReadOnlyStencil

		case DepthRead_StencilRead:
			return 3; // old DSAT_ReadOnlyDepthAndStencil
		}
		// should never happen
		assert(0);
		return -1;
	}
	static const uint32_t MaxIndex = 4;

private:
	inline Type ExtractDepth() const
	{
		return (Type)(Value & DepthMask);
	}
	inline Type ExtractStencil() const
	{
		return (Type)(Value & StencilMask);
	}
};

class FRHIDepthRenderTargetView
{
public:
	FTextureRHIParamRef Texture;

	ERenderTargetLoadAction		DepthLoadAction;
	ERenderTargetStoreAction	DepthStoreAction;
	ERenderTargetLoadAction		StencilLoadAction;

private:
	ERenderTargetStoreAction	StencilStoreAction;
	FExclusiveDepthStencil		DepthStencilAccess;
public:

	// accessor to prevent write access to StencilStoreAction
	ERenderTargetStoreAction GetStencilStoreAction() const { return StencilStoreAction; }
	// accessor to prevent write access to DepthStencilAccess
	FExclusiveDepthStencil GetDepthStencilAccess() const { return DepthStencilAccess; }

	explicit FRHIDepthRenderTargetView() :
		Texture(nullptr),
		DepthLoadAction(ERenderTargetLoadAction::ENoAction),
		DepthStoreAction(ERenderTargetStoreAction::ENoAction),
		StencilLoadAction(ERenderTargetLoadAction::ENoAction),
		StencilStoreAction(ERenderTargetStoreAction::ENoAction),
		DepthStencilAccess(FExclusiveDepthStencil::DepthNop_StencilNop)
	{
		Validate();
	}

	//common case
	explicit FRHIDepthRenderTargetView(FTextureRHIParamRef InTexture, ERenderTargetLoadAction InLoadAction, ERenderTargetStoreAction InStoreAction) :
		Texture(InTexture),
		DepthLoadAction(InLoadAction),
		DepthStoreAction(InStoreAction),
		StencilLoadAction(InLoadAction),
		StencilStoreAction(InStoreAction),
		DepthStencilAccess(FExclusiveDepthStencil::DepthWrite_StencilWrite)
	{
		Validate();
	}

	explicit FRHIDepthRenderTargetView(FTextureRHIParamRef InTexture, ERenderTargetLoadAction InLoadAction, ERenderTargetStoreAction InStoreAction, FExclusiveDepthStencil InDepthStencilAccess) :
		Texture(InTexture),
		DepthLoadAction(InLoadAction),
		DepthStoreAction(InStoreAction),
		StencilLoadAction(InLoadAction),
		StencilStoreAction(InStoreAction),
		DepthStencilAccess(InDepthStencilAccess)
	{
		Validate();
	}

	explicit FRHIDepthRenderTargetView(FTextureRHIParamRef InTexture, ERenderTargetLoadAction InDepthLoadAction, ERenderTargetStoreAction InDepthStoreAction, ERenderTargetLoadAction InStencilLoadAction, ERenderTargetStoreAction InStencilStoreAction) :
		Texture(InTexture),
		DepthLoadAction(InDepthLoadAction),
		DepthStoreAction(InDepthStoreAction),
		StencilLoadAction(InStencilLoadAction),
		StencilStoreAction(InStencilStoreAction),
		DepthStencilAccess(FExclusiveDepthStencil::DepthWrite_StencilWrite)
	{
		Validate();
	}

	explicit FRHIDepthRenderTargetView(FTextureRHIParamRef InTexture, ERenderTargetLoadAction InDepthLoadAction, ERenderTargetStoreAction InDepthStoreAction, ERenderTargetLoadAction InStencilLoadAction, ERenderTargetStoreAction InStencilStoreAction, FExclusiveDepthStencil InDepthStencilAccess) :
		Texture(InTexture),
		DepthLoadAction(InDepthLoadAction),
		DepthStoreAction(InDepthStoreAction),
		StencilLoadAction(InStencilLoadAction),
		StencilStoreAction(InStencilStoreAction),
		DepthStencilAccess(InDepthStencilAccess)
	{
		Validate();
	}

	void Validate() const
	{
		//ensureMsgf(DepthStencilAccess.IsDepthWrite() || DepthStoreAction == ERenderTargetStoreAction::ENoAction, TEXT("Depth is read-only, but we are performing a store.  This is a waste on mobile.  If depth can't change, we don't need to store it out again"));
		//ensureMsgf(DepthStencilAccess.IsStencilWrite() || StencilStoreAction == ERenderTargetStoreAction::ENoAction, TEXT("Stencil is read-only, but we are performing a store.  This is a waste on mobile.  If stencil can't change, we don't need to store it out again"));
#if defined(DEBUG) || defined(_DEBUG)
		if (DepthStencilAccess.IsDepthWrite() || DepthStoreAction == ERenderTargetStoreAction::ENoAction)
		{
			WARNING_INFO("Depth is read - only, but we are performing a store.This is a waste on mobile.If depth can't change, we don't need to store it out again");
		}

		if (DepthStencilAccess.IsStencilWrite() || StencilStoreAction == ERenderTargetStoreAction::ENoAction)
		{
			WARNING_INFO("Stencil is read - only, but we are performing a store.This is a waste on mobile.If stencil can't change, we don't need to store it out again");
		}
#endif

	}

	bool operator==(const FRHIDepthRenderTargetView& Other) const
	{
		return
			Texture == Other.Texture &&
			DepthLoadAction == Other.DepthLoadAction &&
			DepthStoreAction == Other.DepthStoreAction &&
			StencilLoadAction == Other.StencilLoadAction &&
			StencilStoreAction == Other.StencilStoreAction &&
			DepthStencilAccess == Other.DepthStencilAccess;
	}
};

class FRHISetRenderTargetsInfo
{
public:
	// Color Render Targets Info
	FRHIRenderTargetView ColorRenderTarget[MaxSimultaneousRenderTargets];
	int32_t NumColorRenderTargets;
	bool bClearColor;

	// Depth/Stencil Render Target Info
	FRHIDepthRenderTargetView DepthStencilRenderTarget;
	bool bClearDepth;
	bool bClearStencil;

	// UAVs info.
	FUnorderedAccessViewRHIRef UnorderedAccessView[MaxSimultaneousUAVs];
	int32_t NumUAVs;

	FRHISetRenderTargetsInfo() :
		NumColorRenderTargets(0),
		bClearColor(false),
		bClearDepth(false),
		bClearStencil(false),
		NumUAVs(0)
	{}

	FRHISetRenderTargetsInfo(int32_t InNumColorRenderTargets, const FRHIRenderTargetView* InColorRenderTargets, const FRHIDepthRenderTargetView& InDepthStencilRenderTarget) :
		NumColorRenderTargets(InNumColorRenderTargets),
		bClearColor(InNumColorRenderTargets > 0 && InColorRenderTargets[0].LoadAction == ERenderTargetLoadAction::EClear),
		DepthStencilRenderTarget(InDepthStencilRenderTarget),
		bClearDepth(InDepthStencilRenderTarget.Texture&& InDepthStencilRenderTarget.DepthLoadAction == ERenderTargetLoadAction::EClear),
		bClearStencil(InDepthStencilRenderTarget.Texture&& InDepthStencilRenderTarget.StencilLoadAction == ERenderTargetLoadAction::EClear),
		NumUAVs(0)
	{
		assert(InNumColorRenderTargets <= 0 || InColorRenderTargets);
		for (int32_t Index = 0; Index < InNumColorRenderTargets; ++Index)
		{
			ColorRenderTarget[Index] = InColorRenderTargets[Index];
		}
	}
	// @todo metal mrt: This can go away after all the cleanup is done
	void SetClearDepthStencil(bool bInClearDepth, bool bInClearStencil = false)
	{
		if (bInClearDepth)
		{
			DepthStencilRenderTarget.DepthLoadAction = ERenderTargetLoadAction::EClear;
		}
		if (bInClearStencil)
		{
			DepthStencilRenderTarget.StencilLoadAction = ERenderTargetLoadAction::EClear;
		}
		bClearDepth = bInClearDepth;
		bClearStencil = bInClearStencil;
	}

	uint32_t CalculateHash() const
	{
		// Need a separate struct so we can memzero/remove dependencies on reference counts
		struct FHashableStruct
		{
			// Depth goes in the last slot
			FRHITexture* Texture[MaxSimultaneousRenderTargets + 1];
			uint32_t MipIndex[MaxSimultaneousRenderTargets];
			uint32_t ArraySliceIndex[MaxSimultaneousRenderTargets];
			ERenderTargetLoadAction LoadAction[MaxSimultaneousRenderTargets];
			ERenderTargetStoreAction StoreAction[MaxSimultaneousRenderTargets];

			ERenderTargetLoadAction		DepthLoadAction;
			ERenderTargetStoreAction	DepthStoreAction;
			ERenderTargetLoadAction		StencilLoadAction;
			ERenderTargetStoreAction	StencilStoreAction;
			FExclusiveDepthStencil		DepthStencilAccess;

			bool bClearDepth;
			bool bClearStencil;
			bool bClearColor;
			FRHIUnorderedAccessView* UnorderedAccessView[MaxSimultaneousUAVs];

			void Set(const FRHISetRenderTargetsInfo& RTInfo)
			{
				memset(this, 0, sizeof(FRHISetRenderTargetsInfo));
				for (int32_t Index = 0; Index < RTInfo.NumColorRenderTargets; ++Index)
				{
					Texture[Index] = RTInfo.ColorRenderTarget[Index].Texture;
					MipIndex[Index] = RTInfo.ColorRenderTarget[Index].MipIndex;
					ArraySliceIndex[Index] = RTInfo.ColorRenderTarget[Index].ArraySliceIndex;
					LoadAction[Index] = RTInfo.ColorRenderTarget[Index].LoadAction;
					StoreAction[Index] = RTInfo.ColorRenderTarget[Index].StoreAction;
				}

				Texture[MaxSimultaneousRenderTargets] = RTInfo.DepthStencilRenderTarget.Texture;
				DepthLoadAction = RTInfo.DepthStencilRenderTarget.DepthLoadAction;
				DepthStoreAction = RTInfo.DepthStencilRenderTarget.DepthStoreAction;
				StencilLoadAction = RTInfo.DepthStencilRenderTarget.StencilLoadAction;
				StencilStoreAction = RTInfo.DepthStencilRenderTarget.GetStencilStoreAction();
				DepthStencilAccess = RTInfo.DepthStencilRenderTarget.GetDepthStencilAccess();

				bClearDepth = RTInfo.bClearDepth;
				bClearStencil = RTInfo.bClearStencil;
				bClearColor = RTInfo.bClearColor;

				for (int32_t Index = 0; Index < MaxSimultaneousUAVs; ++Index)
				{
					UnorderedAccessView[Index] = RTInfo.UnorderedAccessView[Index];
				}
			}
		};

		FHashableStruct RTHash;
		memset(&RTHash, 0, sizeof(FHashableStruct));
		RTHash.Set(*this);
		return FCrc::MemCrc32(&RTHash, sizeof(RTHash));
	}
};

class FRHICustomPresent : public FRHIResource
{
public:
	FRHICustomPresent() {}

	virtual ~FRHICustomPresent() {} // should release any references to D3D resources.

	// Called when viewport is resized.
	virtual void OnBackBufferResize() = 0;

	// Called from render thread to see if a native present will be requested for this frame.
	// @return	true if native Present will be requested for this frame; false otherwise.  Must
	// match value subsequently returned by Present for this frame.
	virtual bool NeedsNativePresent() = 0;

	// Called from RHI thread to perform custom present.
	// @param InOutSyncInterval - in out param, indicates if vsync is on (>0) or off (==0).
	// @return	true if native Present should be also be performed; false otherwise. If it returns
	// true, then InOutSyncInterval could be modified to switch between VSync/NoVSync for the normal 
	// Present.  Must match value previously returned by NeedsNormalPresent for this frame.
	virtual bool Present(int32_t& InOutSyncInterval) = 0;

	// Called from RHI thread after native Present has been called
	virtual void PostPresent() {};

	// Called when rendering thread is acquired
	virtual void OnAcquireThreadOwnership() {}
	// Called when rendering thread is released
	virtual void OnReleaseThreadOwnership() {}
};


typedef FRHICustomPresent* FCustomPresentRHIParamRef;
typedef TRefCountPtr<FRHICustomPresent> FCustomPresentRHIRef;

// Template magic to convert an FRHI*Shader to its enum
template<typename TRHIShader> struct TRHIShaderToEnum {};
template<> struct TRHIShaderToEnum<FRHIVertexShader> { enum { ShaderFrequency = SF_Vertex }; };
template<> struct TRHIShaderToEnum<FRHIHullShader> { enum { ShaderFrequency = SF_Hull }; };
template<> struct TRHIShaderToEnum<FRHIDomainShader> { enum { ShaderFrequency = SF_Domain }; };
template<> struct TRHIShaderToEnum<FRHIPixelShader> { enum { ShaderFrequency = SF_Pixel }; };
template<> struct TRHIShaderToEnum<FRHIGeometryShader> { enum { ShaderFrequency = SF_Geometry }; };
template<> struct TRHIShaderToEnum<FRHIComputeShader> { enum { ShaderFrequency = SF_Compute }; };
template<> struct TRHIShaderToEnum<FVertexShaderRHIParamRef> { enum { ShaderFrequency = SF_Vertex }; };
template<> struct TRHIShaderToEnum<FHullShaderRHIParamRef> { enum { ShaderFrequency = SF_Hull }; };
template<> struct TRHIShaderToEnum<FDomainShaderRHIParamRef> { enum { ShaderFrequency = SF_Domain }; };
template<> struct TRHIShaderToEnum<FPixelShaderRHIParamRef> { enum { ShaderFrequency = SF_Pixel }; };
template<> struct TRHIShaderToEnum<FGeometryShaderRHIParamRef> { enum { ShaderFrequency = SF_Geometry }; };
template<> struct TRHIShaderToEnum<FComputeShaderRHIParamRef> { enum { ShaderFrequency = SF_Compute }; };
template<> struct TRHIShaderToEnum<FVertexShaderRHIRef> { enum { ShaderFrequency = SF_Vertex }; };
template<> struct TRHIShaderToEnum<FHullShaderRHIRef> { enum { ShaderFrequency = SF_Hull }; };
template<> struct TRHIShaderToEnum<FDomainShaderRHIRef> { enum { ShaderFrequency = SF_Domain }; };
template<> struct TRHIShaderToEnum<FPixelShaderRHIRef> { enum { ShaderFrequency = SF_Pixel }; };
template<> struct TRHIShaderToEnum<FGeometryShaderRHIRef> { enum { ShaderFrequency = SF_Geometry }; };
template<> struct TRHIShaderToEnum<FComputeShaderRHIRef> { enum { ShaderFrequency = SF_Compute }; };

struct FBoundShaderStateInput
{
	FVertexDeclarationRHIParamRef VertexDeclarationRHI;
	FVertexShaderRHIParamRef VertexShaderRHI;
	FHullShaderRHIParamRef HullShaderRHI;
	FDomainShaderRHIParamRef DomainShaderRHI;
	FPixelShaderRHIParamRef PixelShaderRHI;
	FGeometryShaderRHIParamRef GeometryShaderRHI;

	FORCEINLINE FBoundShaderStateInput()
		: VertexDeclarationRHI(nullptr)
		, VertexShaderRHI(nullptr)
		, HullShaderRHI(nullptr)
		, DomainShaderRHI(nullptr)
		, PixelShaderRHI(nullptr)
		, GeometryShaderRHI(nullptr)
	{
	}

	FORCEINLINE FBoundShaderStateInput(
		FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
		FVertexShaderRHIParamRef InVertexShaderRHI,
		FHullShaderRHIParamRef InHullShaderRHI,
		FDomainShaderRHIParamRef InDomainShaderRHI,
		FPixelShaderRHIParamRef InPixelShaderRHI,
		FGeometryShaderRHIParamRef InGeometryShaderRHI
	)
		: VertexDeclarationRHI(InVertexDeclarationRHI)
		, VertexShaderRHI(InVertexShaderRHI)
		, HullShaderRHI(InHullShaderRHI)
		, DomainShaderRHI(InDomainShaderRHI)
		, PixelShaderRHI(InPixelShaderRHI)
		, GeometryShaderRHI(InGeometryShaderRHI)
	{
	}
};

class FGraphicsPipelineStateInitializer
{
public:
	using TRenderTargetFormats = std::array<EPixelFormat, MaxSimultaneousRenderTargets>;
	using TRenderTargetFlags = std::array<uint32_t, MaxSimultaneousRenderTargets>;
	using TRenderTargetLoadActions = std::array<ERenderTargetLoadAction, MaxSimultaneousRenderTargets>;
	using TRenderTargetStoreActions = std::array<ERenderTargetStoreAction, MaxSimultaneousRenderTargets>;

	FGraphicsPipelineStateInitializer()
		: BlendState(nullptr)
		, RasterizerState(nullptr)
		, DepthStencilState(nullptr)
		, PrimitiveType(PT_Num)
		, RenderTargetsEnabled(0)
		//, RenderTargetFormats(PF_Unknown)
		//, RenderTargetFlags(0)
		//, RenderTargetLoadActions(ERenderTargetLoadAction::ENoAction)
		//, RenderTargetStoreActions(ERenderTargetStoreAction::ENoAction)
		, DepthStencilTargetFormat(PF_Unknown)
		, DepthStencilTargetFlag(0)
		, DepthTargetLoadAction(ERenderTargetLoadAction::ENoAction)
		, DepthTargetStoreAction(ERenderTargetStoreAction::ENoAction)
		, StencilTargetLoadAction(ERenderTargetLoadAction::ENoAction)
		, StencilTargetStoreAction(ERenderTargetStoreAction::ENoAction)
		, NumSamples(0)
	{
		RenderTargetFormats.fill(PF_Unknown);
		RenderTargetFlags.fill(0);
		RenderTargetLoadActions.fill(ERenderTargetLoadAction::ENoAction);
		RenderTargetStoreActions.fill(ERenderTargetStoreAction::ENoAction);
	}

	FGraphicsPipelineStateInitializer(
		FBoundShaderStateInput				InBoundShaderState,
		FBlendStateRHIParamRef				InBlendState,
		FRasterizerStateRHIParamRef			InRasterizerState,
		FDepthStencilStateRHIParamRef		InDepthStencilState,
		EPrimitiveType						InPrimitiveType,
		uint32_t								InRenderTargetsEnabled,
		const TRenderTargetFormats& InRenderTargetFormats,
		const TRenderTargetFlags& InRenderTargetFlags,
		const TRenderTargetLoadActions& InRenderTargetLoadActions,
		const TRenderTargetStoreActions& InRenderTargetStoreActions,
		EPixelFormat						InDepthStencilTargetFormat,
		uint32_t								InDepthStencilTargetFlag,
		ERenderTargetLoadAction				InDepthTargetLoadAction,
		ERenderTargetStoreAction			InDepthTargetStoreAction,
		ERenderTargetLoadAction				InStencilTargetLoadAction,
		ERenderTargetStoreAction			InStencilTargetStoreAction,
		FExclusiveDepthStencil				InDepthStencilAccess,
		uint32_t								InNumSamples
	)
		: BoundShaderState(InBoundShaderState)
		, BlendState(InBlendState)
		, RasterizerState(InRasterizerState)
		, DepthStencilState(InDepthStencilState)
		, PrimitiveType(InPrimitiveType)
		, RenderTargetsEnabled(InRenderTargetsEnabled)
		, RenderTargetFormats(InRenderTargetFormats)
		, RenderTargetFlags(InRenderTargetFlags)
		, RenderTargetLoadActions(InRenderTargetLoadActions)
		, RenderTargetStoreActions(InRenderTargetStoreActions)
		, DepthStencilTargetFormat(InDepthStencilTargetFormat)
		, DepthStencilTargetFlag(InDepthStencilTargetFlag)
		, DepthTargetLoadAction(InDepthTargetLoadAction)
		, DepthTargetStoreAction(InDepthTargetStoreAction)
		, StencilTargetLoadAction(InStencilTargetLoadAction)
		, StencilTargetStoreAction(InStencilTargetStoreAction)
		, DepthStencilAccess(InDepthStencilAccess)
		, NumSamples(InNumSamples)
	{
	}

	bool operator==(const FGraphicsPipelineStateInitializer& rhs) const
	{
		if (BoundShaderState.VertexDeclarationRHI != rhs.BoundShaderState.VertexDeclarationRHI ||
			BoundShaderState.VertexShaderRHI != rhs.BoundShaderState.VertexShaderRHI ||
			BoundShaderState.PixelShaderRHI != rhs.BoundShaderState.PixelShaderRHI ||
			BoundShaderState.GeometryShaderRHI != rhs.BoundShaderState.GeometryShaderRHI ||
			BoundShaderState.DomainShaderRHI != rhs.BoundShaderState.DomainShaderRHI ||
			BoundShaderState.HullShaderRHI != rhs.BoundShaderState.HullShaderRHI ||
			BlendState != rhs.BlendState ||
			RasterizerState != rhs.RasterizerState ||
			DepthStencilState != rhs.DepthStencilState ||
			bDepthBounds != rhs.bDepthBounds ||
			PrimitiveType != rhs.PrimitiveType ||
			RenderTargetsEnabled != rhs.RenderTargetsEnabled ||
			RenderTargetFormats != rhs.RenderTargetFormats ||
			RenderTargetFlags != rhs.RenderTargetFlags ||
			RenderTargetLoadActions != rhs.RenderTargetLoadActions ||
			RenderTargetStoreActions != rhs.RenderTargetStoreActions ||
			DepthStencilTargetFormat != rhs.DepthStencilTargetFormat ||
			DepthStencilTargetFlag != rhs.DepthStencilTargetFlag ||
			DepthTargetLoadAction != rhs.DepthTargetLoadAction ||
			DepthTargetStoreAction != rhs.DepthTargetStoreAction ||
			StencilTargetLoadAction != rhs.StencilTargetLoadAction ||
			StencilTargetStoreAction != rhs.StencilTargetStoreAction ||
			DepthStencilAccess != rhs.DepthStencilAccess ||
			NumSamples != rhs.NumSamples)
		{
			return false;
		}

		return true;
	}

#define COMPARE_FIELD_BEGIN(Field) \
		if (Field != rhs.Field) \
		{ return Field COMPARE_OP rhs.Field; }

#define COMPARE_FIELD(Field) \
		else if (Field != rhs.Field) \
		{ return Field COMPARE_OP rhs.Field; }

#define COMPARE_FIELD_END \
		else { return false; }

	bool operator<(FGraphicsPipelineStateInitializer& rhs) const
	{
#define COMPARE_OP <

		COMPARE_FIELD_BEGIN(BoundShaderState.VertexDeclarationRHI)
			COMPARE_FIELD(BoundShaderState.VertexShaderRHI)
			COMPARE_FIELD(BoundShaderState.PixelShaderRHI)
			COMPARE_FIELD(BoundShaderState.GeometryShaderRHI)
			COMPARE_FIELD(BoundShaderState.DomainShaderRHI)
			COMPARE_FIELD(BoundShaderState.HullShaderRHI)
			COMPARE_FIELD(BlendState)
			COMPARE_FIELD(RasterizerState)
			COMPARE_FIELD(DepthStencilState)
			COMPARE_FIELD(bDepthBounds)
			COMPARE_FIELD(PrimitiveType)
			COMPARE_FIELD_END;

#undef COMPARE_OP
	}

	bool operator>(FGraphicsPipelineStateInitializer& rhs) const
	{
#define COMPARE_OP >

		COMPARE_FIELD_BEGIN(BoundShaderState.VertexDeclarationRHI)
			COMPARE_FIELD(BoundShaderState.VertexShaderRHI)
			COMPARE_FIELD(BoundShaderState.PixelShaderRHI)
			COMPARE_FIELD(BoundShaderState.GeometryShaderRHI)
			COMPARE_FIELD(BoundShaderState.DomainShaderRHI)
			COMPARE_FIELD(BoundShaderState.HullShaderRHI)
			COMPARE_FIELD(BlendState)
			COMPARE_FIELD(RasterizerState)
			COMPARE_FIELD(DepthStencilState)
			COMPARE_FIELD(bDepthBounds)
			COMPARE_FIELD(PrimitiveType)
			COMPARE_FIELD_END;

#undef COMPARE_OP
	}

#undef COMPARE_FIELD_BEGIN
#undef COMPARE_FIELD
#undef COMPARE_FIELD_END

	uint32_t ComputeNumValidRenderTargets() const
	{
		// Get the count of valid render targets (ignore those at the end of the array with PF_Unknown)
		if (RenderTargetsEnabled > 0)
		{
			int32_t LastValidTarget = -1;
			for (int32_t i = (int32_t)RenderTargetsEnabled - 1; i >= 0; i--)
			{
				if (RenderTargetFormats[i] != PF_Unknown)
				{
					LastValidTarget = i;
					break;
				}
			}
			return uint32_t(LastValidTarget + 1);
		}
		return RenderTargetsEnabled;
	}

	// TODO: [PSO API] - As we migrate reuse existing API objects, but eventually we can move to the direct initializers. 
	// When we do that work, move this to RHI.h as its more appropriate there, but here for now since dependent typdefs are here.
	FBoundShaderStateInput			BoundShaderState;
	FBlendStateRHIParamRef			BlendState;
	FRasterizerStateRHIParamRef		RasterizerState;
	FDepthStencilStateRHIParamRef	DepthStencilState;
	bool							bDepthBounds = false;
	EPrimitiveType					PrimitiveType;
	uint32_t							RenderTargetsEnabled;
	TRenderTargetFormats			RenderTargetFormats;
	TRenderTargetFlags				RenderTargetFlags;
	TRenderTargetLoadActions		RenderTargetLoadActions;
	TRenderTargetStoreActions		RenderTargetStoreActions;
	EPixelFormat					DepthStencilTargetFormat;
	uint32_t							DepthStencilTargetFlag;
	ERenderTargetLoadAction			DepthTargetLoadAction;
	ERenderTargetStoreAction		DepthTargetStoreAction;
	ERenderTargetLoadAction			StencilTargetLoadAction;
	ERenderTargetStoreAction		StencilTargetStoreAction;
	FExclusiveDepthStencil			DepthStencilAccess;
	uint16_t							NumSamples;

	friend class FMeshDrawingPolicy;
};

// This PSO is used as a fallback for RHIs that dont support PSOs. It is used to set the graphics state using the legacy state setting APIs
class FRHIGraphicsPipelineStateFallBack : public FRHIGraphicsPipelineState
{
public:
	FRHIGraphicsPipelineStateFallBack() {}

	FRHIGraphicsPipelineStateFallBack(const FGraphicsPipelineStateInitializer& Init)
		: Initializer(Init)
	{
	}

	FGraphicsPipelineStateInitializer Initializer;
};

class FRHIComputePipelineStateFallback : public FRHIComputePipelineState
{
public:
	FRHIComputePipelineStateFallback(FRHIComputeShader* InComputeShader)
		: ComputeShader(InComputeShader)
	{
		assert(InComputeShader);
	}

	FRHIComputeShader* GetComputeShader()
	{
		return ComputeShader;
	}

protected:
	TRefCountPtr<FRHIComputeShader> ComputeShader;
};

//
// Shader Library
//

class FRHIShaderLibrary : public FRHIResource
{
public:
	FRHIShaderLibrary(EShaderPlatform InPlatform, std::string const& InName) : Platform(InPlatform), LibraryName(InName), LibraryId(GetTypeHash(InName)) {}
	virtual ~FRHIShaderLibrary() {}

	FORCEINLINE EShaderPlatform GetPlatform(void) const { return Platform; }
	FORCEINLINE std::string GetName(void) const { return LibraryName; }
	FORCEINLINE uint32_t GetId(void) const { return LibraryId; }

	virtual bool IsNativeLibrary() const = 0;

	//Library iteration
	struct FShaderLibraryEntry
	{
		FShaderLibraryEntry() : Frequency(SF_NumFrequencies), Platform(SP_NumPlatforms) {}

		FSHAHash Hash;
		EShaderFrequency Frequency;
		EShaderPlatform Platform;

		bool IsValid() const { return (Frequency < SF_NumFrequencies) && (Platform < SP_NumPlatforms); }
	};

	class FShaderLibraryIterator : public FRHIResource
	{
	public:
		FShaderLibraryIterator(FRHIShaderLibrary* ShaderLibrary) : ShaderLibrarySource(ShaderLibrary) {}
		virtual ~FShaderLibraryIterator() {}

		//Is the iterator valid
		virtual bool IsValid() const = 0;

		//Iterator position access
		virtual FShaderLibraryEntry operator*()	const = 0;

		//Iterator next operation
		virtual FShaderLibraryIterator& operator++() = 0;

		//Access the library we are iterating through - allow query e.g. GetPlatform from iterator object
		FRHIShaderLibrary* GetLibrary() const { return ShaderLibrarySource; };

	protected:
		//Control source object lifetime while iterator is 'active'
		TRefCountPtr<FRHIShaderLibrary> ShaderLibrarySource;
	};

	virtual TRefCountPtr<FShaderLibraryIterator> CreateIterator(void) = 0;
	//virtual bool RequestEntry(const FSHAHash& Hash, FArchive* Ar) = 0;
	virtual bool ContainsEntry(const FSHAHash& Hash) = 0;
	virtual uint32_t GetShaderCount(void) const = 0;

protected:
	EShaderPlatform Platform;
	std::string LibraryName;
	uint32_t LibraryId;
};

typedef FRHIShaderLibrary* FRHIShaderLibraryParamRef;
typedef TRefCountPtr<FRHIShaderLibrary>	FRHIShaderLibraryRef;

class FRHIPipelineBinaryLibrary : public FRHIResource
{
public:
	FRHIPipelineBinaryLibrary(EShaderPlatform InPlatform, std::string const& FilePath) : Platform(InPlatform) {}
	virtual ~FRHIPipelineBinaryLibrary() {}

	FORCEINLINE EShaderPlatform GetPlatform(void) const { return Platform; }

protected:
	EShaderPlatform Platform;
};
typedef FRHIPipelineBinaryLibrary* FRHIPipelineBinaryLibraryParamRef;
typedef TRefCountPtr<FRHIPipelineBinaryLibrary>	FRHIPipelineBinaryLibraryRef;

enum class ERenderTargetActions : uint8_t
{
	LoadOpMask = 2,

#define RTACTION_MAKE_MASK(Load, Store) (((uint8_t)ERenderTargetLoadAction::Load << (uint8_t)LoadOpMask) | (uint8_t)ERenderTargetStoreAction::Store)

	DontLoad_DontStore = RTACTION_MAKE_MASK(ENoAction, ENoAction),

	DontLoad_Store = RTACTION_MAKE_MASK(ENoAction, EStore),
	Clear_Store = RTACTION_MAKE_MASK(EClear, EStore),
	Load_Store = RTACTION_MAKE_MASK(ELoad, EStore),

	Clear_DontStore = RTACTION_MAKE_MASK(EClear, ENoAction),
	Load_DontStore = RTACTION_MAKE_MASK(ELoad, ENoAction),
	Clear_Resolve = RTACTION_MAKE_MASK(EClear, EMultisampleResolve),
	Load_Resolve = RTACTION_MAKE_MASK(ELoad, EMultisampleResolve),

#undef RTACTION_MAKE_MASK
};

inline ERenderTargetActions MakeRenderTargetActions(ERenderTargetLoadAction Load, ERenderTargetStoreAction Store)
{
	return (ERenderTargetActions)(((uint8_t)Load << (uint8_t)ERenderTargetActions::LoadOpMask) | (uint8_t)Store);
}

inline ERenderTargetLoadAction GetLoadAction(ERenderTargetActions Action)
{
	return (ERenderTargetLoadAction)((uint8_t)Action >> (uint8_t)ERenderTargetActions::LoadOpMask);
}

inline ERenderTargetStoreAction GetStoreAction(ERenderTargetActions Action)
{
	return (ERenderTargetStoreAction)((uint8_t)Action & ((1 << (uint8_t)ERenderTargetActions::LoadOpMask) - 1));
}

enum class EDepthStencilTargetActions : uint8_t
{
	DepthMask = 4,

#define RTACTION_MAKE_MASK(Depth, Stencil) (((uint8_t)ERenderTargetActions::Depth << (uint8_t)DepthMask) | (uint8_t)ERenderTargetActions::Stencil)

	DontLoad_DontStore = RTACTION_MAKE_MASK(DontLoad_DontStore, DontLoad_DontStore),
	DontLoad_StoreDepthStencil = RTACTION_MAKE_MASK(DontLoad_Store, DontLoad_Store),
	DontLoad_StoreStencilNotDepth = RTACTION_MAKE_MASK(DontLoad_DontStore, DontLoad_Store),
	ClearDepthStencil_StoreDepthStencil = RTACTION_MAKE_MASK(Clear_Store, Clear_Store),
	LoadDepthStencil_StoreDepthStencil = RTACTION_MAKE_MASK(Load_Store, Load_Store),
	LoadDepthNotStencil_DontStore = RTACTION_MAKE_MASK(Load_DontStore, DontLoad_DontStore),
	LoadDepthStencil_StoreStencilNotDepth = RTACTION_MAKE_MASK(Load_DontStore, Load_Store),

	ClearDepthStencil_DontStoreDepthStencil = RTACTION_MAKE_MASK(Clear_DontStore, Clear_DontStore),
	LoadDepthStencil_DontStoreDepthStencil = RTACTION_MAKE_MASK(Load_DontStore, Load_DontStore),
	ClearDepthStencil_StoreDepthNotStencil = RTACTION_MAKE_MASK(Clear_Store, Clear_DontStore),
	ClearDepthStencil_StoreStencilNotDepth = RTACTION_MAKE_MASK(Clear_DontStore, Clear_Store),
	ClearDepthStencil_ResolveDepthNotStencil = RTACTION_MAKE_MASK(Clear_Resolve, Clear_DontStore),
	ClearDepthStencil_ResolveStencilNotDepth = RTACTION_MAKE_MASK(Clear_DontStore, Clear_Resolve),

	ClearStencilDontLoadDepth_StoreStencilNotDepth = RTACTION_MAKE_MASK(DontLoad_DontStore, Clear_Store),

#undef RTACTION_MAKE_MASK
};

inline constexpr EDepthStencilTargetActions MakeDepthStencilTargetActions(const ERenderTargetActions Depth, const ERenderTargetActions Stencil)
{
	return (EDepthStencilTargetActions)(((uint8_t)Depth << (uint8_t)EDepthStencilTargetActions::DepthMask) | (uint8_t)Stencil);
}

inline ERenderTargetActions GetDepthActions(EDepthStencilTargetActions Action)
{
	return (ERenderTargetActions)((uint8_t)Action >> (uint8_t)EDepthStencilTargetActions::DepthMask);
}

inline ERenderTargetActions GetStencilActions(EDepthStencilTargetActions Action)
{
	return (ERenderTargetActions)((uint8_t)Action & ((1 << (uint8_t)EDepthStencilTargetActions::DepthMask) - 1));
}

struct FRHIRenderPassInfo
{
	struct FColorEntry
	{
		FRHITexture* RenderTarget;
		FRHITexture* ResolveTarget;
		int32_t ArraySlice;
		uint8_t MipIndex;
		ERenderTargetActions Action;
	};
	FColorEntry ColorRenderTargets[MaxSimultaneousRenderTargets];

	struct FDepthStencilEntry
	{
		FRHITexture* DepthStencilTarget;
		FRHITexture* ResolveTarget;
		EDepthStencilTargetActions Action;
		FExclusiveDepthStencil ExclusiveDepthStencil;
	};
	FDepthStencilEntry DepthStencilRenderTarget;

	FResolveParams ResolveParameters;

	// Some RHIs require a hint that occlusion queries will be used in this render pass
	uint32_t NumOcclusionQueries = 0;
	bool bOcclusionQueries = false;

	// Some RHIs need to know if this render pass is going to be reading and writing to the same texture in the case of generating mip maps for partial resource transitions
	bool bGeneratingMips = false;

	// Color, no depth, optional resolve, optional mip, optional array slice
	explicit FRHIRenderPassInfo(FRHITexture* ColorRT, ERenderTargetActions ColorAction, FRHITexture* ResolveRT = nullptr, uint32_t InMipIndex = 0, int32_t InArraySlice = -1)
	{
		assert(ColorRT);
		ColorRenderTargets[0].RenderTarget = ColorRT;
		ColorRenderTargets[0].ResolveTarget = ResolveRT;
		ColorRenderTargets[0].ArraySlice = InArraySlice;
		ColorRenderTargets[0].MipIndex = InMipIndex;
		ColorRenderTargets[0].Action = ColorAction;
		DepthStencilRenderTarget.DepthStencilTarget = nullptr;
		DepthStencilRenderTarget.Action = EDepthStencilTargetActions::DontLoad_DontStore;
		DepthStencilRenderTarget.ExclusiveDepthStencil = FExclusiveDepthStencil::DepthNop_StencilNop;
		bIsMSAA = ColorRT->GetNumSamples() > 1;
		memset(&ColorRenderTargets[1], 0, sizeof(FColorEntry) * (MaxSimultaneousRenderTargets - 1));
	}

	// Color MRTs, no depth
	explicit FRHIRenderPassInfo(int32_t NumColorRTs, FRHITexture* ColorRTs[], ERenderTargetActions ColorAction)
	{
		assert(NumColorRTs > 0);
		for (int32_t Index = 0; Index < NumColorRTs; ++Index)
		{
			assert(ColorRTs[Index]);
			ColorRenderTargets[Index].RenderTarget = ColorRTs[Index];
			ColorRenderTargets[Index].ResolveTarget = nullptr;
			ColorRenderTargets[Index].ArraySlice = -1;
			ColorRenderTargets[Index].MipIndex = 0;
			ColorRenderTargets[Index].Action = ColorAction;
		}
		DepthStencilRenderTarget.DepthStencilTarget = nullptr;
		DepthStencilRenderTarget.Action = EDepthStencilTargetActions::DontLoad_DontStore;
		DepthStencilRenderTarget.ExclusiveDepthStencil = FExclusiveDepthStencil::DepthNop_StencilNop;
		if (NumColorRTs < MaxSimultaneousRenderTargets)
		{
			memset(&ColorRenderTargets[NumColorRTs], 0, sizeof(FColorEntry) * (MaxSimultaneousRenderTargets - NumColorRTs));
		}
	}

	// Color MRTs, no depth
	explicit FRHIRenderPassInfo(int32_t NumColorRTs, FRHITexture* ColorRTs[], ERenderTargetActions ColorAction, FRHITexture* ResolveTargets[])
	{
		assert(NumColorRTs > 0);
		for (int32_t Index = 0; Index < NumColorRTs; ++Index)
		{
			assert(ColorRTs[Index]);
			ColorRenderTargets[Index].RenderTarget = ColorRTs[Index];
			ColorRenderTargets[Index].ResolveTarget = ResolveTargets[Index];
			ColorRenderTargets[Index].ArraySlice = -1;
			ColorRenderTargets[Index].MipIndex = 0;
			ColorRenderTargets[Index].Action = ColorAction;
		}
		DepthStencilRenderTarget.DepthStencilTarget = nullptr;
		DepthStencilRenderTarget.Action = EDepthStencilTargetActions::DontLoad_DontStore;
		DepthStencilRenderTarget.ExclusiveDepthStencil = FExclusiveDepthStencil::DepthNop_StencilNop;
		if (NumColorRTs < MaxSimultaneousRenderTargets)
		{
			memset(&ColorRenderTargets[NumColorRTs], 0, sizeof(FColorEntry) * (MaxSimultaneousRenderTargets - NumColorRTs));
		}
	}

	// Color MRTs and depth
	explicit FRHIRenderPassInfo(int32_t NumColorRTs, FRHITexture* ColorRTs[], ERenderTargetActions ColorAction, FRHITexture* DepthRT, EDepthStencilTargetActions DepthActions, FExclusiveDepthStencil InEDS = FExclusiveDepthStencil::DepthWrite_StencilWrite)
	{
		assert(NumColorRTs > 0);
		for (int32_t Index = 0; Index < NumColorRTs; ++Index)
		{
			assert(ColorRTs[Index]);
			ColorRenderTargets[Index].RenderTarget = ColorRTs[Index];
			ColorRenderTargets[Index].ResolveTarget = nullptr;
			ColorRenderTargets[Index].ArraySlice = -1;
			ColorRenderTargets[Index].MipIndex = 0;
			ColorRenderTargets[Index].Action = ColorAction;
		}
		assert(DepthRT);
		DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
		DepthStencilRenderTarget.ResolveTarget = nullptr;
		DepthStencilRenderTarget.Action = DepthActions;
		DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
		bIsMSAA = DepthRT->GetNumSamples() > 1;
		if (NumColorRTs < MaxSimultaneousRenderTargets)
		{
			memset(&ColorRenderTargets[NumColorRTs],0, sizeof(FColorEntry) * (MaxSimultaneousRenderTargets - NumColorRTs));
		}
	}

	// Color MRTs and depth
	explicit FRHIRenderPassInfo(int32_t NumColorRTs, FRHITexture* ColorRTs[], ERenderTargetActions ColorAction, FRHITexture* ResolveRTs[], FRHITexture* DepthRT, EDepthStencilTargetActions DepthActions, FRHITexture* ResolveDepthRT, FExclusiveDepthStencil InEDS = FExclusiveDepthStencil::DepthWrite_StencilWrite)
	{
		assert(NumColorRTs > 0);
		for (int32_t Index = 0; Index < NumColorRTs; ++Index)
		{
			assert(ColorRTs[Index]);
			ColorRenderTargets[Index].RenderTarget = ColorRTs[Index];
			ColorRenderTargets[Index].ResolveTarget = ResolveRTs[Index];
			ColorRenderTargets[Index].ArraySlice = -1;
			ColorRenderTargets[Index].MipIndex = 0;
			ColorRenderTargets[Index].Action = ColorAction;
		}
		assert(DepthRT);
		DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
		DepthStencilRenderTarget.ResolveTarget = ResolveDepthRT;
		DepthStencilRenderTarget.Action = DepthActions;
		DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
		bIsMSAA = DepthRT->GetNumSamples() > 1;
		if (NumColorRTs < MaxSimultaneousRenderTargets)
		{
			memset(&ColorRenderTargets[NumColorRTs], 0, sizeof(FColorEntry) * (MaxSimultaneousRenderTargets - NumColorRTs));
		}
	}

	// Depth, no color
	explicit FRHIRenderPassInfo(FRHITexture* DepthRT, EDepthStencilTargetActions DepthActions, FRHITexture* ResolveDepthRT = nullptr, FExclusiveDepthStencil InEDS = FExclusiveDepthStencil::DepthWrite_StencilWrite)
	{
		assert(DepthRT);
		DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
		DepthStencilRenderTarget.ResolveTarget = ResolveDepthRT;
		DepthStencilRenderTarget.Action = DepthActions;
		DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
		bIsMSAA = DepthRT->GetNumSamples() > 1;
		memset(ColorRenderTargets,0, sizeof(FColorEntry) * MaxSimultaneousRenderTargets);
	}

	// Depth, no color, occlusion queries
	explicit FRHIRenderPassInfo(FRHITexture* DepthRT, uint32_t InNumOcclusionQueries, EDepthStencilTargetActions DepthActions, FRHITexture* ResolveDepthRT = nullptr, FExclusiveDepthStencil InEDS = FExclusiveDepthStencil::DepthWrite_StencilWrite)
		: NumOcclusionQueries(InNumOcclusionQueries)
		, bOcclusionQueries(true)
	{
		assert(DepthRT);
		DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
		DepthStencilRenderTarget.ResolveTarget = ResolveDepthRT;
		DepthStencilRenderTarget.Action = DepthActions;
		DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
		bIsMSAA = DepthRT->GetNumSamples() > 1;
		memset(ColorRenderTargets,0, sizeof(FColorEntry) * MaxSimultaneousRenderTargets);
	}

	// Color and depth
	explicit FRHIRenderPassInfo(FRHITexture* ColorRT, ERenderTargetActions ColorAction, FRHITexture* DepthRT, EDepthStencilTargetActions DepthActions, FExclusiveDepthStencil InEDS = FExclusiveDepthStencil::DepthWrite_StencilWrite)
	{
		assert(ColorRT);
		ColorRenderTargets[0].RenderTarget = ColorRT;
		ColorRenderTargets[0].ResolveTarget = nullptr;
		ColorRenderTargets[0].ArraySlice = -1;
		ColorRenderTargets[0].MipIndex = 0;
		ColorRenderTargets[0].Action = ColorAction;
		bIsMSAA = ColorRT->GetNumSamples() > 1;
		assert(DepthRT);
		DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
		DepthStencilRenderTarget.ResolveTarget = nullptr;
		DepthStencilRenderTarget.Action = DepthActions;
		DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
		memset(&ColorRenderTargets[1],0, sizeof(FColorEntry) * (MaxSimultaneousRenderTargets - 1));
	}

	// Color and depth with resolve
	explicit FRHIRenderPassInfo(FRHITexture* ColorRT, ERenderTargetActions ColorAction, FRHITexture* ResolveColorRT,
		FRHITexture* DepthRT, EDepthStencilTargetActions DepthActions, FRHITexture* ResolveDepthRT, FExclusiveDepthStencil InEDS = FExclusiveDepthStencil::DepthWrite_StencilWrite)
	{
		assert(ColorRT);
		ColorRenderTargets[0].RenderTarget = ColorRT;
		ColorRenderTargets[0].ResolveTarget = ResolveColorRT;
		ColorRenderTargets[0].ArraySlice = -1;
		ColorRenderTargets[0].MipIndex = 0;
		ColorRenderTargets[0].Action = ColorAction;
		bIsMSAA = ColorRT->GetNumSamples() > 1;
		assert(DepthRT);
		DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
		DepthStencilRenderTarget.ResolveTarget = ResolveDepthRT;
		DepthStencilRenderTarget.Action = DepthActions;
		DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
		memset(&ColorRenderTargets[1],0, sizeof(FColorEntry) * (MaxSimultaneousRenderTargets - 1));
	}

	inline int32_t GetNumColorRenderTargets() const
	{
		int32_t ColorIndex = 0;
		for (; ColorIndex < MaxSimultaneousRenderTargets; ++ColorIndex)
		{
			const FColorEntry& Entry = ColorRenderTargets[ColorIndex];
			if (!Entry.RenderTarget)
			{
				break;
			}
		}

		return ColorIndex;
	}

	explicit FRHIRenderPassInfo()
	{
		memset(this,0,sizeof(FRHIRenderPassInfo));
	}

	inline bool IsMSAA() const
	{
		return bIsMSAA;
	}

	void Validate() const;
	void ConvertToRenderTargetsInfo(FRHISetRenderTargetsInfo& OutRTInfo) const;

	//#RenderPasses
	int32_t UAVIndex = -1;
	int32_t NumUAVs = 0;
	FUnorderedAccessViewRHIRef UAVs[MaxSimultaneousUAVs];

	FRHIRenderPassInfo& operator = (const FRHIRenderPassInfo& In)
	{
		memcpy(this, &In,sizeof(FRHIRenderPassInfo));
		return *this;
	}

private:
	bool bIsMSAA = false;
};
