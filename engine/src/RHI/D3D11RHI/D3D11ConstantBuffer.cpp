// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#include "Engine/YCommonHeader.h"
#include "RHI/D3D11RHI/D3D11ConstantBuffer.h"
#include "Math/YMath.h"
#include "Render/RenderResource.h"
#include <d3d11.h>
#include "RHI/D3D11RHI/D3D11RHI.h"

/*=============================================================================
	D3D11ConstantBuffer.cpp: D3D Constant buffer RHI implementation.
=============================================================================*/


// SHANOND: Looks like UpdateSubresource is going to be the way to update these CBs.
//			The driver writers are trying to optimize for UpdateSubresource and this
//			will also avoid any driver renaming issues we may hit with map_discard.

#define MAX_POOL_BUFFERS 1 // use update subresource and plop it into the command stream

/** Sizes of constant buffers defined in ED3D11ShaderOffsetBuffer. */
const uint32 GConstantBufferSizes[MAX_CONSTANT_BUFFER_SLOTS] = 
{
	// CBs must be a multiple of 16
	(uint32)YMath::Align(MAX_GLOBAL_CONSTANT_BUFFER_SIZE, 16),
};

// New circular buffer system for faster constant uploads.  Avoids CopyResource and speeds things up considerably
FD3D11ConstantBuffer::FD3D11ConstantBuffer(FD3D11DynamicRHI* InD3DRHI, uint32 InSize, uint32 SubBuffers) : 
	D3DRHI(InD3DRHI),
	MaxSize(InSize),
	ShadowData(NULL),
	CurrentUpdateSize(0),
	TotalUpdateSize(0)
{
	InitResource();
}

FD3D11ConstantBuffer::~FD3D11ConstantBuffer()
{
	ReleaseResource();
}

/**
* Creates a constant buffer on the device
*/
void FD3D11ConstantBuffer::InitDynamicRHI()
{
// New circular buffer system for faster constant uploads.  Avoids CopyResource and speeds things up considerably
	// aligned for best performance
	ShadowData = (uint8*)_aligned_malloc(MaxSize, 16);
	//FMemory::Memzero(ShadowData,MaxSize);
	memset(&ShadowData, 0, MaxSize);
}

void FD3D11ConstantBuffer::ReleaseDynamicRHI()
{
	if(ShadowData)
	{
		_aligned_free(ShadowData);
	}
}

void FWinD3D11ConstantBuffer::InitDynamicRHI()
{
	TRefCountPtr<ID3D11Buffer> CBuffer = NULL;
	D3D11_BUFFER_DESC BufferDesc;
	ZeroMemory(&BufferDesc, sizeof(D3D11_BUFFER_DESC));
	// Verify constant buffer ByteWidth requirements
	check(MaxSize <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT && (MaxSize % 16) == 0);
	BufferDesc.ByteWidth = MaxSize;

	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.MiscFlags = 0;

	Buffers = new TRefCountPtr<ID3D11Buffer>[NumSubBuffers];
	CurrentSubBuffer = 0;
	//note by zyx, create constant buffer mipmaps
	for (uint32 s = 0; s < NumSubBuffers; s++)
	{
		VERIFYD3D11RESULT_EX(D3DRHI->GetDevice()->CreateBuffer(&BufferDesc, NULL, Buffers[s].GetInitReference()), D3DRHI->GetDevice());
		UpdateBufferStats(Buffers[s], true);
		BufferDesc.ByteWidth = YMath::Align(BufferDesc.ByteWidth / 2, 16);
	}

	FD3D11ConstantBuffer::InitDynamicRHI();
}

void FWinD3D11ConstantBuffer::ReleaseDynamicRHI()
{
	FD3D11ConstantBuffer::ReleaseDynamicRHI();

	if (Buffers)
	{
		for (uint32 s = 0; s < NumSubBuffers; s++)
		{
			UpdateBufferStats(Buffers[s], false);
		}

		delete[] Buffers;
		Buffers = NULL;
	}
}

bool FWinD3D11ConstantBuffer::CommitConstantsToDevice(bool bDiscardSharedConstants)
{
	// New circular buffer system for faster constant uploads.  Avoids CopyResource and speeds things up considerably
	if (CurrentUpdateSize == 0)
	{
		return false;
	}

	SCOPE_CYCLE_COUNTER(STAT_D3D11GlobalConstantBufferUpdateTime);

	if (bDiscardSharedConstants)
	{
		// If we're discarding shared constants, just use constants that have been updated since the last Commit.
		TotalUpdateSize = CurrentUpdateSize;
	}
	else
	{
		// If we're re-using shared constants, use all constants.
		TotalUpdateSize = YMath::Max(CurrentUpdateSize, TotalUpdateSize);
	}

	// This basically keeps track dynamically how much data has been updated every frame
	// and then divides up a "max" constant buffer size by halves down until it finds a large sections that more tightly covers
	// the amount updated, assuming that all data in a constant buffer is updated each draw call and contiguous.
	// This only used for the IndexSlot==0 constant buffer on the vertex shader.
	// Which will have an indeterminate number of constant values that are generated by material shaders.
	CurrentSubBuffer = 1;
	uint32 BufferSize = MaxSize / 2;
	while (BufferSize >= TotalUpdateSize && CurrentSubBuffer < NumSubBuffers)
	{
		CurrentSubBuffer++;
		BufferSize /= 2;
	}
	CurrentSubBuffer--;
	BufferSize *= 2;

	ID3D11Buffer* Buffer = Buffers[CurrentSubBuffer];
	check(((uint64)(ShadowData) & 0xf) == 0);
	D3DRHI->GetDeviceContext()->UpdateSubresource(Buffer, 0, NULL, (void*)ShadowData, BufferSize, BufferSize);

	CurrentUpdateSize = 0;

	return true;
}

void FD3D11DynamicRHI::InitConstantBuffers()
{
	// Allocate shader constant buffers.  All shader types can have access to all buffers.
	//  Index==0 is reserved for "custom" params, and the rest are reserved by the system for Common
	//	constants
	VSConstantBuffers.clear();
	VSConstantBuffers.reserve(MAX_CONSTANT_BUFFER_SLOTS);
	PSConstantBuffers.clear();
	PSConstantBuffers.reserve(MAX_CONSTANT_BUFFER_SLOTS);
	HSConstantBuffers.clear();
	HSConstantBuffers.reserve(MAX_CONSTANT_BUFFER_SLOTS);
	DSConstantBuffers.clear();
	DSConstantBuffers.reserve(MAX_CONSTANT_BUFFER_SLOTS);
	GSConstantBuffers.clear();
	GSConstantBuffers.reserve(MAX_CONSTANT_BUFFER_SLOTS);
	CSConstantBuffers.clear();
	CSConstantBuffers.reserve(MAX_CONSTANT_BUFFER_SLOTS);

	for (int32 BufferIndex = 0; BufferIndex < MAX_CONSTANT_BUFFER_SLOTS; BufferIndex++)
	{
		uint32 Size = GConstantBufferSizes[BufferIndex];
		uint32 SubBuffers = 1;
		if (BufferIndex == GLOBAL_CONSTANT_BUFFER_INDEX) //-V547
		{
			SubBuffers = 5;
		}

		// Vertex shader can have subbuffers for index==0.  This is from Epic's original design for the auto-fit of size to
		//	reduce the update costs of the buffer.

		// New circular buffer system for faster constant uploads.  Avoids CopyResource and speeds things up considerably
		VSConstantBuffers.push_back(new FWinD3D11ConstantBuffer(this, Size, SubBuffers));
		PSConstantBuffers.push_back(new FWinD3D11ConstantBuffer(this, Size, SubBuffers));
		HSConstantBuffers.push_back(new FWinD3D11ConstantBuffer(this, Size));
		DSConstantBuffers.push_back(new FWinD3D11ConstantBuffer(this, Size));
		GSConstantBuffers.push_back(new FWinD3D11ConstantBuffer(this, Size));
		CSConstantBuffers.push_back(new FWinD3D11ConstantBuffer(this, Size));
	}
}

//DEFINE_STAT(STAT_D3D11GlobalConstantBufferUpdateTime);
