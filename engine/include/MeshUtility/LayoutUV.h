// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MeshUtility/Allocator2D.h"
#include "Math/YVector.h"
#include <atomic>

struct FMeshChart
{
	uint32		FirstTri;
	uint32		LastTri;
	
	YVector2	MinUV;
	YVector2	MaxUV;
	
	float		UVArea;
	YVector2	UVScale;
	YVector2	WorldScale;
	
	YVector2	PackingScaleU;
	YVector2	PackingScaleV;
	YVector2	PackingBias;

	int32		Join[4];

	int32		Id; // Store a unique id so that we can come back to the initial Charts ordering when necessary
};

struct YOverlappingCorners;

class  FLayoutUV
{
public:

	/**
	 * Abstract triangle mesh view interface that may be used by any module without introducing
	 * a dependency on a concrete mesh type (and thus potentially circular module references).
	 * This abstraction results in a performance penalty due to virtual dispatch,
	 * however it is expected to be insignificant compared to the rest of work done by FLayoutUV
	 * and cache misses due to indexed vertex data access.
	*/
	struct IMeshView
	{
		virtual ~IMeshView() {}

		virtual uint32      GetNumIndices() const = 0;
		virtual YVector     GetPosition(uint32 Index) const = 0;
		virtual YVector     GetNormal(uint32 Index) const = 0;
		virtual YVector2   GetInputTexcoord(uint32 Index) const = 0;

		virtual void        InitOutputTexcoords(uint32 Num) = 0;
		virtual void        SetOutputTexcoord(uint32 Index, const YVector2& Value) = 0;
	};

	FLayoutUV( IMeshView& InMeshView );
	void SetVersion( ELightmapUVVersion Version ) { LayoutVersion = Version; }
	int32 FindCharts( const YOverlappingCorners& OverlappingCorners );
	bool FindBestPacking( uint32 InTextureResolution );
	void CommitPackedUVs();

	static void LogStats();
	static void ResetStats();
private:
	IMeshView& MeshView;
	ELightmapUVVersion LayoutVersion;

	std::vector< YVector2 > MeshTexCoords;
	std::vector< uint32 > MeshSortedTris;
	std::vector< FMeshChart > MeshCharts;
	uint32 PackedTextureResolution;

	struct FChartFinder;
	struct FChartPacker;

	static std::atomic<uint64> FindBestPackingCount;
	static std::atomic<uint64> FindBestPackingCycles;
	static std::atomic<uint64> FindBestPackingEfficiency;
};
