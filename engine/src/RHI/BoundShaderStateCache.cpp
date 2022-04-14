// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BoundShaderStateCache.cpp: Bound shader state cache implementation.
=============================================================================*/

#include "RHI/BoundShaderStateCache.h"
#include "Utility/ScopeLock.h"
#include <unordered_map>


typedef std::unordered_map<FBoundShaderStateLookupKey,FCachedBoundShaderStateLink*> FBoundShaderStateCache;
typedef std::unordered_map<FBoundShaderStateLookupKey,FCachedBoundShaderStateLink_Threadsafe*> FBoundShaderStateCache_Threadsafe;

static FBoundShaderStateCache GBoundShaderStateCache;
static FBoundShaderStateCache_Threadsafe GBoundShaderStateCache_ThreadSafe;

/** Lazily initialized bound shader state cache singleton. */
static FBoundShaderStateCache& GetBoundShaderStateCache()
{
	return GBoundShaderStateCache;
}

/** Lazily initialized bound shader state cache singleton. */
static FBoundShaderStateCache_Threadsafe& GetBoundShaderStateCache_Threadsafe()
{
	return GBoundShaderStateCache_ThreadSafe;
}

static FCriticalSection BoundShaderStateCacheLock;


FCachedBoundShaderStateLink::FCachedBoundShaderStateLink(
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FVertexShaderRHIParamRef VertexShader,
	FPixelShaderRHIParamRef PixelShader,
	FHullShaderRHIParamRef HullShader,
	FDomainShaderRHIParamRef DomainShader,
	FGeometryShaderRHIParamRef GeometryShader,
	FBoundShaderStateRHIParamRef InBoundShaderState,
	bool bAddToSingleThreadedCache
	):
	BoundShaderState(InBoundShaderState),
	Key(VertexDeclaration,VertexShader,PixelShader,HullShader,DomainShader,GeometryShader),
	bAddedToSingleThreadedCache(bAddToSingleThreadedCache)
{
	if (bAddToSingleThreadedCache)
	{
		GetBoundShaderStateCache()[Key] = this;
	}
}

FCachedBoundShaderStateLink::FCachedBoundShaderStateLink(
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FVertexShaderRHIParamRef VertexShader,
	FPixelShaderRHIParamRef PixelShader,
	FBoundShaderStateRHIParamRef InBoundShaderState,
	bool bAddToSingleThreadedCache
	):
	BoundShaderState(InBoundShaderState),
	Key(VertexDeclaration,VertexShader,PixelShader),
	bAddedToSingleThreadedCache(bAddToSingleThreadedCache)
{
	if (bAddToSingleThreadedCache)
	{
		GetBoundShaderStateCache()[Key] = this;
	}
}

FCachedBoundShaderStateLink::~FCachedBoundShaderStateLink()
{
	if (bAddedToSingleThreadedCache)
	{
		GetBoundShaderStateCache().erase(Key);
		bAddedToSingleThreadedCache = false;
	}
}

FCachedBoundShaderStateLink* GetCachedBoundShaderState(
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FVertexShaderRHIParamRef VertexShader,
	FPixelShaderRHIParamRef PixelShader,
	FHullShaderRHIParamRef HullShader,
	FDomainShaderRHIParamRef DomainShader,
	FGeometryShaderRHIParamRef GeometryShader
	)
{
	// Find the existing bound shader state in the cache.
	auto result = GetBoundShaderStateCache().find(FBoundShaderStateLookupKey(VertexDeclaration, VertexShader, PixelShader, HullShader, DomainShader, GeometryShader));
	if (result != GetBoundShaderStateCache().end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
	//return GetBoundShaderStateCache().FindRef(
		//FBoundShaderStateLookupKey(VertexDeclaration,VertexShader,PixelShader,HullShader,DomainShader,GeometryShader)
		//);
}


void FCachedBoundShaderStateLink_Threadsafe::AddToCache()
{
	FScopeLock Lock(&BoundShaderStateCacheLock);
	GetBoundShaderStateCache_Threadsafe()[Key]=this;
}
void FCachedBoundShaderStateLink_Threadsafe::RemoveFromCache()
{
	FScopeLock Lock(&BoundShaderStateCacheLock);
	GetBoundShaderStateCache_Threadsafe().erase(Key);
}


FBoundShaderStateRHIRef GetCachedBoundShaderState_Threadsafe(
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FVertexShaderRHIParamRef VertexShader,
	FPixelShaderRHIParamRef PixelShader,
	FHullShaderRHIParamRef HullShader,
	FDomainShaderRHIParamRef DomainShader,
	FGeometryShaderRHIParamRef GeometryShader
	)
{
	FScopeLock Lock(&BoundShaderStateCacheLock);
	// Find the existing bound shader state in the cache.
	//FCachedBoundShaderStateLink_Threadsafe* CachedBoundShaderStateLink = GetBoundShaderStateCache_Threadsafe().FindRef(
	//	FBoundShaderStateLookupKey(VertexDeclaration,VertexShader,PixelShader,HullShader,DomainShader,GeometryShader)
	//	);
	auto result = GetBoundShaderStateCache_Threadsafe().find(FBoundShaderStateLookupKey(VertexDeclaration, VertexShader, PixelShader, HullShader, DomainShader, GeometryShader));
	//if(CachedBoundShaderStateLink)
	//{
	//	// If we've already created a bound shader state with these parameters, reuse it.
	//	return CachedBoundShaderStateLink->BoundShaderState;
	//}
	if (result != GetBoundShaderStateCache_Threadsafe().end())
	{
		return result->second->BoundShaderState;
	}
	return FBoundShaderStateRHIRef();
}

void EmptyCachedBoundShaderStates()
{
	GetBoundShaderStateCache().clear();
	GetBoundShaderStateCache_Threadsafe().clear();
}
