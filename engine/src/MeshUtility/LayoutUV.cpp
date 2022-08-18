// Copyright Epic Games, Inc. All Rights Reserved.


#include <atomic>
#include <algorithm>
#include <unordered_map>
#include <future>
#include <xatomic.h>
#include <map>
#include "MeshUtility/LayoutUV.h"
#include "MeshUtility/DisjointSet.h"
#include "MeshUtility/OverlappingCorners.h"
#include "MeshUtility/OverlappingCorners.h"
#include "Utility/SecureHash.h"
#include "MeshUtility/Allocator2D.h"
//#include "Modules/ModuleManager.h"

//IMPLEMENT_MODULE(FDefaultModuleImpl, MeshUtilitiesCommon)

//DEFINE_LOG_CATEGORY_STATIC(LogLayoutUV, Log, All);

#define CHART_JOINING	1

#define NEW_UVS_ARE_SAME THRESH_POINTS_ARE_SAME
#define LEGACY_UVS_ARE_SAME (1.0f / 1024.0f)
#define UVLAYOUT_THRESH_UVS_ARE_SAME (GetUVEqualityThreshold())

std::atomic<uint64> FLayoutUV::FindBestPackingCount(0);
std::atomic<uint64> FLayoutUV::FindBestPackingCycles(0);
std::atomic<uint64> FLayoutUV::FindBestPackingEfficiency(0);

FLayoutUV::FLayoutUV( IMeshView& InMeshView )
	: MeshView( InMeshView )
	, LayoutVersion( ELightmapUVVersion::Latest )
	, PackedTextureResolution(0)
{}


/** FIRST PASS: Given a Mesh, build the associated set of charts */
struct FLayoutUV::FChartFinder
{
	FChartFinder(IMeshView& InMeshView, ELightmapUVVersion InLayoutVersion);

	int32 FindCharts( const YOverlappingCorners& OverlappingCorners, std::vector< YVector2 >& TexCoords, std::vector< uint32 >& SortedTris, std::vector< FMeshChart >& Charts );

private:
	bool PositionsMatch( uint32 a, uint32 b ) const;
	bool NormalsMatch( uint32 a, uint32 b ) const;
	bool UVsMatch( uint32 a, uint32 b ) const;
	bool VertsMatch( uint32 a, uint32 b ) const;
	float TriangleUVArea( uint32 Tri ) const;
	void DisconnectChart( std::vector< FMeshChart >& Charts, FMeshChart& Chart, uint32 Side );
	float GetUVEqualityThreshold() const;

private:
	IMeshView& MeshView;
	ELightmapUVVersion LayoutVersion;

	int32 NextMeshChartId;
};


/** SECOND PASS: Given a set of charts, pack them in the UV space */
struct FLayoutUV::FChartPacker
{
	FChartPacker(IMeshView& InMeshView, ELightmapUVVersion InLayoutVersion, uint32 TextureResolution);

	bool FindBestPacking(const std::vector< YVector2 >& TexCoords, const std::vector< uint32 >& SortedTris, std::vector< FMeshChart >& AllCharts);

private:
	void ScaleCharts( std::vector< FMeshChart >& Charts, float UVScale );
	bool PackCharts( std::vector< FMeshChart >& Charts, float UVScale, const std::vector< YVector2 >& TexCoords, const std::vector< uint32 >& SortedTris, float& OutEfficiency, std::atomic<bool>& bAbort, bool bTrace);
	void OrientChart( FMeshChart& Chart, int32 Orientation );
	void RasterizeChart( const FMeshChart& Chart, const std::vector< YVector2 >& TexCoords, const std::vector< uint32 >& SortedTris, uint32 RectW, uint32 RectH, FAllocator2D& OutChartRaster );

private:
	IMeshView& MeshView;
	ELightmapUVVersion LayoutVersion;

	uint32 TextureResolution;
	float TotalUVArea;
};

FLayoutUV::FChartPacker::FChartPacker(IMeshView& InMeshView, ELightmapUVVersion InLayoutVersion, uint32 TextureResolution)
	: MeshView(InMeshView)
	, LayoutVersion(InLayoutVersion)
	, TextureResolution(TextureResolution)
	, TotalUVArea(0.0f)
{
}

int32 FLayoutUV::FindCharts(const YOverlappingCorners& OverlappingCorners)
{
	FChartFinder Finder(MeshView, LayoutVersion);
	return Finder.FindCharts(OverlappingCorners, MeshTexCoords, MeshSortedTris, MeshCharts);
}

FLayoutUV::FChartFinder::FChartFinder(IMeshView& InMeshView, ELightmapUVVersion InLayoutVersion)
	: MeshView(InMeshView)
	, LayoutVersion(InLayoutVersion)
	, NextMeshChartId( 0 )
{}

int32 FLayoutUV::FChartFinder::FindCharts( const YOverlappingCorners& OverlappingCorners, std::vector< YVector2 >& TexCoords, std::vector< uint32 >& SortedTris, std::vector< FMeshChart >& Charts )
{
	//double Begin = FPlatformTime::Seconds();

	uint32 NumIndexes = MeshView.GetNumIndices();
	uint32 NumTris = NumIndexes / 3;

	std::vector< int32 > TranslatedMatches;
	TranslatedMatches.resize( NumIndexes );
	TexCoords.resize( NumIndexes );
	for( uint32 i = 0; i < NumIndexes; i++ )
	{
		TranslatedMatches[i] = -1;
		TexCoords[i] = MeshView.GetInputTexcoord(i);
	}

	// Build disjoint set
	FDisjointSet DisjointSet( NumTris );
	for( uint32 i = 0; i < NumIndexes; i++ )
	{
		const std::vector<int32>& Overlapping = OverlappingCorners.FindIfOverlapping(i);
		for (int32 It : Overlapping)
		{
			uint32 j = It;
			if( j > i )
			{
				const uint32 TriI = i/3;
				const uint32 TriJ = j/3;

				bool bUnion = false;

#if CHART_JOINING
				bool bPositionMatch = PositionsMatch( i, j );
				if( bPositionMatch )
				{
					uint32 i1 = 3 * TriI + (i + 1) % 3;
					uint32 i2 = 3 * TriI + (i + 2) % 3;
					uint32 j1 = 3 * TriJ + (j + 1) % 3;
					uint32 j2 = 3 * TriJ + (j + 2) % 3;

					bool bEdgeMatch21 = PositionsMatch( i2, j1 );
					bool bEdgeMatch12 = PositionsMatch( i1, j2 );
					if( bEdgeMatch21 || bEdgeMatch12 )
					{
						uint32 ie = bEdgeMatch21 ? i2 : i1;
						uint32 je = bEdgeMatch21 ? j1 : j2;
							
						bool bUVMatch = UVsMatch( i, j ) && UVsMatch( ie, je );
						bool bUVWindingMatch = TriangleUVArea( TriI ) * TriangleUVArea( TriJ ) >= 0.0f;
						if( bUVMatch && bUVWindingMatch )
						{
							bUnion = true;
						}
						else if( NormalsMatch( i, j ) && NormalsMatch( ie, je ) )
						{
							// Chart edge
							YVector2 EdgeUVi = TexCoords[ie] - TexCoords[i];
							YVector2 EdgeUVj = TexCoords[je] - TexCoords[j];
							
							// Would these edges match if the charts were translated
							bool bTranslatedUVMatch = ( EdgeUVi - EdgeUVj ).IsNearlyZero(UVLAYOUT_THRESH_UVS_ARE_SAME);
							if( bTranslatedUVMatch )
							{
								// Note: may be mirrored
								
								// TODO should these be restricted to axis aligned edges?
								uint32 EdgeI = bEdgeMatch21 ? i2 : i;
								uint32 EdgeJ = bEdgeMatch21 ? j : j2;

								// Only allow one match per edge
								if( TranslatedMatches[ EdgeI ] < 0 &&
									TranslatedMatches[ EdgeJ ] < 0 )
								{
									TranslatedMatches[ EdgeI ] = EdgeJ;
									TranslatedMatches[ EdgeJ ] = EdgeI;
								}
							}
						}
					}
				}
#else
				if( VertsMatch( i, j ) )
				{
					// Edge must match as well (same winding)
					if( VertsMatch( 3 * TriI + (i - 1) % 3, 3 * TriJ + (j + 1) % 3 ) ||
						VertsMatch( 3 * TriI + (i + 1) % 3, 3 * TriJ + (j - 1) % 3 ) )
					{
						// Check for UV winding match too
						if( TriangleUVArea( TriI ) * TriangleUVArea( TriJ ) >= 0.0f )
						{
							bUnion = true;
						}
					}
				}
#endif

				if( bUnion )
				{
					// TODO solve spiral case by checking sets for UV overlap
					DisjointSet.Union( TriI, TriJ );
				}
			}
		}
	}

	// Sort tris by chart
	SortedTris.resize( NumTris );
	for( uint32 i = 0; i < NumTris; i++ )
	{
		// Flatten disjoint set path
		DisjointSet.Find(i);
		SortedTris[i] = i;
	}

	struct FCompareTris
	{
		FDisjointSet* DisjointSet;

		FCompareTris( FDisjointSet* InDisjointSet )
		: DisjointSet( InDisjointSet )
		{}

		FORCEINLINE bool operator()( uint32 A, uint32 B ) const
		{
			return (*DisjointSet)[A] < (*DisjointSet)[B];
		}
	};

	//Algo::IntroSort( SortedTris, FCompareTris( &DisjointSet ) );
    std::sort(SortedTris.begin(), SortedTris.end(), FCompareTris(&DisjointSet));
	std::unordered_map< uint32, int32 > DisjointSetToChartMap;

	// Build Charts
	for( uint32 Tri = 0; Tri < NumTris; )
	{
        int32 i = Charts.size();
        Charts.push_back(FMeshChart());
		FMeshChart& Chart = Charts[i];
		Chart.Id = NextMeshChartId++;

		Chart.MinUV = YVector2( FLT_MAX, FLT_MAX );
		Chart.MaxUV = YVector2( -FLT_MAX, -FLT_MAX );
		Chart.UVArea = 0.0f;
		Chart.WorldScale = YVector2(0.0f,0.0f);
		memset( Chart.Join, 0xff,4*sizeof(int32) );

		Chart.FirstTri = Tri;

		uint32 ChartID = DisjointSet[ SortedTris[ Tri ] ];
		DisjointSetToChartMap[ ChartID] =  i ;

		for( ; Tri < NumTris && DisjointSet[ SortedTris[ Tri ] ] == ChartID; Tri++ )
		{
			// Calculate chart bounds
			YVector		Positions[3];
			YVector2	UVs[3];
			for( int k = 0; k < 3; k++ )
			{
				uint32 Index = 3 * SortedTris[ Tri ] + k;

				Positions[k] = MeshView.GetPosition( Index );
				UVs[k] = TexCoords[ Index ];

				Chart.MinUV.x = YMath::Min( Chart.MinUV.x, UVs[k].x );
				Chart.MinUV.y = YMath::Min( Chart.MinUV.y, UVs[k].y );
				Chart.MaxUV.x = YMath::Max( Chart.MaxUV.x, UVs[k].x );
				Chart.MaxUV.y = YMath::Max( Chart.MaxUV.y, UVs[k].y );
			}

			YVector Edge1 = Positions[1] - Positions[0];
			YVector Edge2 = Positions[2] - Positions[0];
			float Area = 0.5f * ( Edge1 ^ Edge2 ).Size();

			YVector2 EdgeUV1 = UVs[1] - UVs[0];
			YVector2 EdgeUV2 = UVs[2] - UVs[0];
			float UVArea = 0.5f * YMath::Abs( EdgeUV1.x * EdgeUV2.y - EdgeUV1.y * EdgeUV2.x );

			YVector2 UVLength;
			UVLength.x = ( EdgeUV2.y * Edge1 - EdgeUV1.y * Edge2 ).Size();
			UVLength.y = (-EdgeUV2.x * Edge1 + EdgeUV1.x * Edge2 ).Size();
			
			Chart.WorldScale = Chart.WorldScale + UVLength;
			Chart.UVArea += UVArea;
		}
		
		Chart.LastTri = Tri;

#if !CHART_JOINING
		if (LayoutVersion >= ELightmapUVVersion::SmallChartPacking)
		{
			Chart.WorldScale /= YMath::Max(Chart.UVArea, 1e-8f);
		}
		else
		{
			if (Chart.UVArea > 1e-4f)
			{
				Chart.WorldScale /= Chart.UVArea;
			}
			else
			{
				Chart.WorldScale = YVector2::ZeroVector;
			}
		}		
#endif
	}

#if CHART_JOINING
	for( int32 i = 0; i < Charts.size(); i++ )
	{
		FMeshChart& Chart = Charts[i];

		for( uint32 Tri = Chart.FirstTri; Tri < Chart.LastTri; Tri++ )
		{
			for( int k = 0; k < 3; k++ )
			{
				uint32 Index = 3 * SortedTris[ Tri ] + k;

				if( TranslatedMatches[ Index ] >= 0 )
				{
					checkSlow( TranslatedMatches[ TranslatedMatches[ Index ] ] == Index );

					uint32 V0i = Index;
					uint32 V0j = TranslatedMatches[ Index ];

					uint32 TriI = V0i / 3;
					uint32 TriJ = V0j / 3;
					
					if( TriJ <= TriI )
					{
						// Only need to consider one direction
						continue;
					}
					
					uint32 V1i = 3 * TriI + (V0i + 1) % 3;
					uint32 V1j = 3 * TriJ + (V0j + 1) % 3;

					int32 ChartI = i;
					int32 ChartJ = DisjointSetToChartMap[ DisjointSet[ TriJ ] ];

					YVector2 UV0i = TexCoords[ V0i ];
					YVector2 UV1i = TexCoords[ V1i ];
					YVector2 UV0j = TexCoords[ V0j ];
					YVector2 UV1j = TexCoords[ V1j ];

					YVector2 EdgeUVi = UV1i - UV0i;
					YVector2 EdgeUVj = UV1j - UV0j;

					bool bMirrored = TriangleUVArea( TriI ) * TriangleUVArea( TriJ ) < 0.0f;
					
					YVector2 EdgeOffset0 = UV0i - UV1j;
					YVector2 EdgeOffset1 = UV1i - UV0j;
					checkSlow( ( EdgeOffset0 - EdgeOffset1 ).IsNearlyZero(UVLAYOUT_THRESH_UVS_ARE_SAME) );

					YVector2 Translation = EdgeOffset0;

					FMeshChart& ChartA = Charts[ ChartI ];
					FMeshChart& ChartB = Charts[ ChartJ ];

					for( uint32 Side = 0; Side < 4; Side++ )
					{
						// Join[] = { left, right, bottom, top }

						// FIXME
						if( bMirrored )
							continue;

						if( ChartA.Join[ Side ^ 0 ] != -1 ||
							ChartB.Join[ Side ^ 1 ] != -1 )
						{
							// Already joined with something else
							continue;
						}

						uint32 Sign = Side & 1;
						uint32 Axis = Side >> 1;

						bool bAxisAligned = YMath::Abs( EdgeUVi[ Axis ] ) < UVLAYOUT_THRESH_UVS_ARE_SAME;
						bool bBorderA = YMath::Abs( UV0i[ Axis ] - ( Sign ^ 0 ? Chart.MaxUV[ Axis ] : Chart.MinUV[ Axis ] ) ) < UVLAYOUT_THRESH_UVS_ARE_SAME;
						bool bBorderB = YMath::Abs( UV0j[ Axis ] - ( Sign ^ 1 ? Chart.MaxUV[ Axis ] : Chart.MinUV[ Axis ] ) ) < UVLAYOUT_THRESH_UVS_ARE_SAME;

						// FIXME mirrored
						if( !bAxisAligned || !bBorderA || !bBorderB )
						{
							// Edges weren't on matching rectangle borders
							continue;
						}

						YVector2 CenterA = 0.5f * ( ChartA.MinUV + ChartA.MaxUV );
						YVector2 CenterB = 0.5f * ( ChartB.MinUV + ChartB.MaxUV );

						YVector2 ExtentA = 0.5f * ( ChartA.MaxUV - ChartA.MinUV );
						YVector2 ExtentB = 0.5f * ( ChartB.MaxUV - ChartB.MinUV );

						// FIXME mirrored
						CenterB = CenterB + Translation;

						YVector2 CenterDiff = CenterA - CenterB;
						YVector2 ExtentDiff = ExtentA - ExtentB;
						YVector2 Separation = ExtentA + ExtentB + CenterDiff * ( Sign ? 1.0f : -1.0f );

						bool bCenterMatch = YMath::Abs( CenterDiff[ Axis ^ 1 ] ) < UVLAYOUT_THRESH_UVS_ARE_SAME;
						bool bExtentMatch = YMath::Abs( ExtentDiff[ Axis ^ 1 ] ) < UVLAYOUT_THRESH_UVS_ARE_SAME;
						bool bSeparate    = YMath::Abs( Separation[ Axis ^ 0 ] ) < UVLAYOUT_THRESH_UVS_ARE_SAME;
	
						if( !bCenterMatch || !bExtentMatch || !bSeparate )
						{
							// Rectangles don't match up after translation
							continue;
						}

						// Found a valid edge join
						ChartA.Join[ Side ^ 0 ] = ChartJ;
						ChartB.Join[ Side ^ 1 ] = ChartI;
						break;
					}
				}
			}
		}
	}

	std::vector< uint32 > JoinedSortedTris;
	JoinedSortedTris.reserve( NumTris );

	// Detect loops
	for( uint32 Axis = 0; Axis < 2; Axis++ )
	{
		uint32 Side = Axis << 1;

		for( int32 i = 0; i < Charts.size(); i++ )
		{
			int32 j = Charts[i].Join[ Side ^ 1 ];
			while( j != -1 )
			{
				int32 Next = Charts[j].Join[ Side ^ 1 ];
				if( Next == i )
				{
					// Break loop
					Charts[i].Join[ Side ^ 0 ] = -1;
					Charts[j].Join[ Side ^ 1 ] = -1;
					break;
				}
				j = Next;
			}
		}
	}

	// Join rows first, then columns
	for( uint32 Axis = 0; Axis < 2; Axis++ )
	{
		for( int32 i = 0; i < Charts.size(); i++ )
		{
			FMeshChart& ChartA = Charts[i];

			if( ChartA.FirstTri == ChartA.LastTri )
			{
				// Empty chart
				continue;
			}
			
			for( uint32 Side = 0; Side < 4; Side++ )
			{
				if( ChartA.Join[ Side ] != -1 )
				{
					FMeshChart& ChartB = Charts[ ChartA.Join[ Side ] ];

					check( ChartB.Join[ Side ^ 1 ] == i );
					check( ChartB.FirstTri != ChartB.LastTri );
				}
			}
		}

		NumTris = 0;

		for( int32 i = 0; i < Charts.size(); i++ )
		{
			FMeshChart& Chart = Charts[i];
			NumTris += Chart.LastTri - Chart.FirstTri;
		}
		check( NumTris == SortedTris.size() );

		NumTris = 0;
		for( int32 i = 0; i < Charts.size(); i++ )
		{
			FMeshChart& ChartA = Charts[i];

			if( ChartA.FirstTri == ChartA.LastTri )
			{
				// Empty chart
				continue;
			}

			uint32 Side = Axis << 1;

			// Find start (left, bottom)
			if( ChartA.Join[ Side ^ 0 ] == -1 )
			{
				// Add original tris
				NumTris += ChartA.LastTri - ChartA.FirstTri;

				// Continue joining until no more to the (right, top)
				int32 Next = ChartA.Join[ Side ^ 1 ];
				while( Next != -1 )
				{
					FMeshChart& ChartB = Charts[ Next ];

					NumTris += ChartB.LastTri - ChartB.FirstTri;
					Next = ChartB.Join[ Side ^ 1 ];
				}
			}
		}
		check( NumTris == SortedTris.size() );

#if 1
		NumTris = 0;
		for( int32 i = 0; i < Charts.size(); i++ )
		{
			FMeshChart& ChartA = Charts[i];

			if( ChartA.FirstTri == ChartA.LastTri )
			{
				// Empty chart
				continue;
			}

			// Join[] = { left, right, bottom, top }

			uint32 Side = Axis << 1;

			// Find start (left, bottom)
			if( ChartA.Join[ Side ^ 0 ] == -1 )
			{
				uint32 FirstTri = JoinedSortedTris.size();
			
				// Add original tris
				for( uint32 Tri = ChartA.FirstTri; Tri < ChartA.LastTri; Tri++ )
				{
					JoinedSortedTris.push_back( SortedTris[ Tri ] );
				}
				NumTris += ChartA.LastTri - ChartA.FirstTri;

				// Continue joining until no more to the (right, top)
				while( ChartA.Join[ Side ^ 1 ] != -1 )
				{
					FMeshChart& ChartB = Charts[ ChartA.Join[ Side ^ 1 ] ];

					check( ChartB.FirstTri != ChartB.LastTri );

					YVector2 Translation = ChartA.MinUV - ChartB.MinUV;
					Translation[ Axis ] += ChartA.MaxUV[ Axis ] - ChartA.MinUV[ Axis ];

					for( uint32 Tri = ChartB.FirstTri; Tri < ChartB.LastTri; Tri++ )
					{
						JoinedSortedTris.push_back( SortedTris[ Tri ] );
						for( int k = 0; k < 3; k++ )
						{
							TexCoords[ 3 * SortedTris[ Tri ] + k ] = TexCoords[3 * SortedTris[Tri] + k] +  Translation;
						}
					}
					NumTris += ChartB.LastTri - ChartB.FirstTri;

					ChartA.Join[ Side ^ 1 ] = ChartB.Join[ Side ^ 1 ];
					ChartA.MaxUV[ Axis ] += ChartB.MaxUV[ Axis ] - ChartB.MinUV[ Axis ];
					if( LayoutVersion >= ELightmapUVVersion::ChartJoiningLFix )
					{
						// Fixing joined chart MaxUV value to properly inflate non-joined axis extent
						ChartA.MaxUV[ Axis ^ 1 ] = YMath::Max( ChartA.MaxUV[ Axis ^ 1 ], ChartA.MinUV[ Axis ^ 1 ] + ( ChartB.MaxUV[ Axis ^ 1 ] - ChartB.MinUV[ Axis ^ 1 ] ) );
					}
					ChartA.WorldScale = ChartA.WorldScale+ ChartB.WorldScale;
					ChartA.UVArea += ChartB.UVArea;

					ChartB.FirstTri = 0;
					ChartB.LastTri = 0;
					ChartB.UVArea = 0.0f;

					DisconnectChart( Charts, ChartB, Side ^ 2 );
					DisconnectChart( Charts, ChartB, Side ^ 3 );
				}

				ChartA.FirstTri = FirstTri;
				ChartA.LastTri = JoinedSortedTris.size();
			}
			else
			{
				// Make sure a starting chart could connect to this
				FMeshChart& ChartB = Charts[ ChartA.Join[ Side ^ 0 ] ];
				check( ChartB.Join[ Side ^ 1 ] == i );
				check( ChartB.FirstTri != ChartB.LastTri );
			}
		}
		check( NumTris == SortedTris.size() );

		check( SortedTris.size() == JoinedSortedTris.size() );
		SortedTris.swap(JoinedSortedTris );
		JoinedSortedTris.clear();
#endif
	}
	
	// Clean out empty charts
	for( int32 i = 0; i < Charts.size(); )
	{
		if( Charts[i].FirstTri == Charts[i].LastTri )
		{
			Charts.erase(Charts.begin()+i);
		}
		else
		{ 
			i++;
		}	
	}

	for( int32 i = 0; i < Charts.size(); i++ )
	{
		FMeshChart& Chart = Charts[i];

		if (LayoutVersion >= ELightmapUVVersion::SmallChartPacking)
		{
			Chart.WorldScale = Chart.WorldScale/ YMath::Max(Chart.UVArea, 1e-8f);
		}
		else
		{
			if (Chart.UVArea > 1e-4f)
			{
				Chart.WorldScale = Chart.WorldScale / Chart.UVArea;
			}
			else
			{
				Chart.WorldScale = YVector2::zero_vector;
			}
		}
	}
#endif

	//double End = FPlatformTime::Seconds();

	//UE_LOG(LogLayoutUV, VeryVerbose, TEXT("FindCharts: %s"), *FPlatformTime::PrettyTime(End - Begin) );

	return Charts.size();
}

#if UE_EDITOR && (UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG)
static TAutoConsoleVariable<FString> CVarLayoutUVTracePackingForInputHash(
	TEXT("LayoutUV.TracePackingForInputHash"),
	TEXT(""),
	TEXT("Activate tracing for the input hash specified in the value.\n"),
	ECVF_Default);
#endif

bool FLayoutUV::FChartPacker::FindBestPacking(const std::vector< YVector2 >& TexCoords, const std::vector< uint32 >& SortedTris, std::vector< FMeshChart >& Charts)
{
	if( (uint32)Charts.size() > TextureResolution * TextureResolution )
	{
		// More charts than texels
		return false;
	}
	
	TotalUVArea = 0.0f;
	for (const FMeshChart& Chart : Charts)
	{
		TotalUVArea += Chart.UVArea * Chart.WorldScale.x * Chart.WorldScale.y;
	}

	if( TotalUVArea <= 0.0f )
	{
		return false;
	}

	//uint64 StartCycles = YPlatformTime::Cycles64();
	//TRACE_CPUPROFILER_EVENT_SCOPE(FChartPacker::FindBestPacking)

	// Cleanup uninitialized values to get a stable input hash
	for (FMeshChart& Chart : Charts)
	{
		Chart.PackingBias   = YVector2::zero_vector;
		Chart.PackingScaleU = YVector2::zero_vector;
		Chart.PackingScaleV = YVector2::zero_vector;
		Chart.UVScale       = YVector2::zero_vector;
	}

	//FString InputHash = FMD5::HashBytes((uint8*)Charts.GetData(), Charts.Num() * Charts.GetTypeSize());

//#if UE_EDITOR && (UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG)
//	// When you need to find where an unexpected difference in output hash might come from
//	// after changing the algorithm. You can set this CVar to activate tracing for a particular
//	// input hash.
//	FString InputHashTrace = CVarLayoutUVTracePackingForInputHash.GetValueOnAnyThread().TrimStartAndEnd();
//	const bool bTrace = InputHashTrace.Len() > 0 && InputHash.StartsWith(InputHashTrace);
//#else
	const bool bTrace = false;
//#endif

	// Those might require tuning, changing them won't affect the outcome and will maintain backward compatibility
	const int32 MultithreadChartsCountThreshold       = 100*1000;
	const int32 MultithreadTextureResolutionThreshold = 1000;
	const int32 MultithreadAheadWorkCount             = 3;

	const float LinearSearchStart = 0.5f;
	const float LinearSearchStep  = 0.5f;
	const int32 BinarySearchSteps = 6;

	float UVScaleFail = TextureResolution * YMath::Sqrt( 1.0f / TotalUVArea );
	float UVScalePass = TextureResolution * YMath::Sqrt( LinearSearchStart / TotalUVArea );

	// Store successful charts packing to avoid redoing the final step
	std::vector<FMeshChart>         LastPassCharts;
	std::atomic<bool>              bAbort(false);

	struct FThreadContext
	{
		std::vector<FMeshChart> Charts;
		std::future<bool>      Result;
		float              Efficiency = 0.0f;
	};

	std::vector<FThreadContext> ThreadContexts;

	//bool bShouldUseMultipleThreads =
	//	 FApp::ShouldUseThreadingForPerformance() && 
	//	!bTrace &&
 //		 Charts.Num() >= MultithreadChartsCountThreshold &&
	//	 TextureResolution >= MultithreadTextureResolutionThreshold;
    bool bShouldUseMultipleThreads = false;
	if ( bShouldUseMultipleThreads )
	{
		// Do forward work only when multi-thread activated
		ThreadContexts.resize(MultithreadAheadWorkCount);
	}

	// Linear search for first fit
	float LastEfficiency = 0.0f;
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE(LinearSearch);

		while(!bAbort)
		{
			// Launch forward work in other threads
			for (int32 Index = 0; Index < ThreadContexts.size(); ++Index)
			{
				ThreadContexts[Index].Charts = Charts;
				float ThreadUVScale  = UVScalePass * YMath::Pow(LinearSearchStep, Index + 1);
				ThreadContexts[Index].Result =
					std::async(
                        std::launch::async,
						[this, &ThreadContexts, &SortedTris, &TexCoords, &bAbort, ThreadUVScale, Index]()
						{
							//TRACE_CPUPROFILER_EVENT_SCOPE(SearchStep);
							return PackCharts(ThreadContexts[Index].Charts, ThreadUVScale, TexCoords, SortedTris, ThreadContexts[Index].Efficiency, bAbort, false);
						}
					);
			}

			if (bTrace)
			{
				//UE_LOG(LogLayoutUV, Log, TEXT("[LAYOUTUV_TRACE] Scale %f"), UVScalePass);
			}

			// Process the first iteration in this thread
			bool bFit = false;
			{
				//TRACE_CPUPROFILER_EVENT_SCOPE(SearchStep);
				bFit = PackCharts(Charts, UVScalePass, TexCoords, SortedTris, LastEfficiency, bAbort, bTrace);
			}

			// Wait for the work sequentially and cancel everything once we have a first viable solution
			for (int32 Index = 0; Index < ThreadContexts.size() + 1; ++Index)
			{
				// The first result is not coming from a future
				bFit = Index == 0 ? bFit : ThreadContexts[Index - 1].Result.get();
				if (bFit && !bAbort)
				{
					// We got a success, cancel other searches
					bAbort = true;

					if (Index > 0)
					{
						Charts         = ThreadContexts[Index - 1].Charts;
						LastEfficiency = ThreadContexts[Index - 1].Efficiency;
					}

					LastPassCharts = Charts;
				}

				if (!bAbort)
				{
					UVScaleFail = UVScalePass;
					UVScalePass *= LinearSearchStep;
				}
			}
		}
	}

	// Binary search for best fit
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE(BinarySearch);

		bAbort = false;
		for( int32 i = 0; i < BinarySearchSteps; i++ )
		{
			//TRACE_CPUPROFILER_EVENT_SCOPE(SearchStep);

			float UVScale = 0.5f * ( UVScaleFail + UVScalePass );

			if (bTrace)
			{
				//UE_LOG(LogLayoutUV, Log, TEXT("[LAYOUTUV_TRACE] Scale %f"), UVScale);
			}

			float Efficiency = 0.0f;
			bool bFit = PackCharts(Charts, UVScale, TexCoords, SortedTris, Efficiency, bAbort, bTrace);
			if( bFit )
			{
				LastPassCharts = Charts;
				float EfficiencyGainPercent = 100.0f * YMath::Abs(Efficiency - LastEfficiency);
				LastEfficiency = Efficiency;

				// Early out when we're inside a 1% efficiency range
				if (LayoutVersion >= ELightmapUVVersion::Segments2D && EfficiencyGainPercent <= 1.0f)
				{
					break;
				}
			
				UVScalePass = UVScale;
			}
			else
			{
				UVScaleFail = UVScale;
			}
		}
	}

	if (LayoutVersion < ELightmapUVVersion::ScaleChartsOrderingFix)
	{
		// Early versions applied a sort that was determinist
		// but dependent on earlier sorts. Since we strive to maintain
		// backward compatibility of the UV layout to avoid screwing
		// with already backed static lighting, we must apply a final
		// scaling and packing that will reuse the last step's ordering
		// whether it was a failure or not.
		PackCharts(Charts, UVScalePass, TexCoords, SortedTris, LastEfficiency, bAbort, bTrace);
	}
	else
	{
		// In case the last step was a failure, restore from last known good computation
		Charts = LastPassCharts;
	}

	//FString OutputHash = FMD5::HashBytes((uint8*)Charts.GetData(), Charts.Num() * Charts.GetTypeSize());

	//// Increase verbosity level to use this for packing results validation when modifying code
	//UE_LOG(LogLayoutUV, Verbose, TEXT("FindBestPacking (Input Data MD5: %s, Output Data MD5: %s, LayoutVersion: %d, Efficiency: %0.2f %%)"), *InputHash, *OutputHash, LayoutVersion, LastEfficiency*100);

	//static TAtomic<uint64> Count(0);
	//static TAtomic<uint64> TotalCycles(0);
	//static TAtomic<uint64> Efficiency(0);

	//FindBestPackingCount++;
	//FindBestPackingEfficiency += LastEfficiency*100000;
	//FindBestPackingCycles += FPlatformTime::Cycles64() - StartCycles;

	return true;
}

void FLayoutUV::ResetStats()
{
	FindBestPackingCount = 0;
	FindBestPackingEfficiency = 0;
	FindBestPackingCycles = 0;
}

void FLayoutUV::LogStats()
{
	//UE_LOG(LogLayoutUV, Log, TEXT("FindBestPacking (Total Time: %s, Avg Efficiency: %f)"), *FPlatformTime::PrettyTime(FPlatformTime::ToSeconds64(FindBestPackingCycles.Load())), double(FindBestPackingEfficiency.Load()) / (FindBestPackingCount.Load()*1000));
}

void FLayoutUV::FChartPacker::ScaleCharts( std::vector< FMeshChart >& Charts, float UVScale )
{
	for( int32 i = 0; i < Charts.size(); i++ )
	{
		FMeshChart& Chart = Charts[i];
		Chart.UVScale = Chart.WorldScale * UVScale;
	}
	
	if ( LayoutVersion >= ELightmapUVVersion::ScaleChartsOrderingFix )
	{
		// Unsort the charts to make sure ScaleCharts always return the same ordering
	/*	Algo::IntroSort( Charts, []( const FMeshChart& A, const FMeshChart& B )
		{
			return A.Id < B.Id;
		});*/
        std::sort(Charts.begin(), Charts.end(), [](const FMeshChart& A, const FMeshChart& B)
            {
                return A.Id < B.Id;
            });
	}

	// Scale charts such that they all fit and roughly total the same area as before
#if 1
	float UniformScale = 1.0f;
	for( int i = 0; i < 1000; i++ )
	{
		uint32 NumMaxedOut = 0;
		float ScaledUVArea = 0.0f;
		for( int32 ChartIndex = 0; ChartIndex < Charts.size(); ChartIndex++ )
		{
			FMeshChart& Chart = Charts[ChartIndex];

			YVector2 ChartSize	= Chart.MaxUV - Chart.MinUV;
			YVector2 ChartSizeScaled = ChartSize * Chart.UVScale * UniformScale;

			const float MaxChartEdge = TextureResolution - 1.0f;
			const float LongestChartEdge = YMath::Max( ChartSizeScaled.x, ChartSizeScaled.y );

			const float Epsilon = 0.01f;
			if( LongestChartEdge + Epsilon > MaxChartEdge )
			{
				// Rescale oversized charts to fit
				Chart.UVScale.x = MaxChartEdge / YMath::Max( ChartSize.x, ChartSize.y );
				Chart.UVScale.y = MaxChartEdge / YMath::Max( ChartSize.x, ChartSize.y );
				NumMaxedOut++;
			}
			else
			{
				Chart.UVScale.x *= UniformScale;
				Chart.UVScale.y *= UniformScale;
			}
			
			ScaledUVArea += Chart.UVArea * Chart.UVScale.x * Chart.UVScale.y;
		}

		if( NumMaxedOut == 0 )
		{
			// No charts maxed out so no need to rebalance
			break;
		}

		if( NumMaxedOut == Charts.size() )
		{
			// All charts are maxed out
			break;
		}

		// Scale up smaller charts to maintain expected total area
		// Want ScaledUVArea == TotalUVArea * UVScale^2
		float RebalanceScale = UVScale * YMath::Sqrt( TotalUVArea / ScaledUVArea );
		if( RebalanceScale < 1.01f )
		{
			// Stop if further rebalancing is minor
			break;
		}
		UniformScale = RebalanceScale;
	}
#endif

#if 1
	float NonuniformScale = 1.0f;
	for( int i = 0; i < 1000; i++ )
	{
		uint32 NumMaxedOut = 0;
		float ScaledUVArea = 0.0f;
		for( int32 ChartIndex = 0; ChartIndex < Charts.size(); ChartIndex++ )
		{
			FMeshChart& Chart = Charts[ChartIndex];

			for( int k = 0; k < 2; k++ )
			{
				const float MaximumChartSize = TextureResolution - 1.0f;
				const float ChartSize = Chart.MaxUV[k] - Chart.MinUV[k];
				const float ChartSizeScaled = ChartSize * Chart.UVScale[k] * NonuniformScale;

				const float Epsilon = 0.01f;
				if( ChartSizeScaled + Epsilon > MaximumChartSize )
				{
					// Scale oversized charts to max size
					Chart.UVScale[k] = MaximumChartSize / ChartSize;
					NumMaxedOut++;
				}
				else
				{
					Chart.UVScale[k] = Chart.UVScale[k]* NonuniformScale;
				}
			}

			ScaledUVArea += Chart.UVArea * Chart.UVScale.x * Chart.UVScale.y;
		}

		if( NumMaxedOut == 0 )
		{
			// No charts maxed out so no need to rebalance
			break;
		}

		if( NumMaxedOut == Charts.size() * 2 )
		{
			// All charts are maxed out in both dimensions
			break;
		}

		// Scale up smaller charts to maintain expected total area
		// Want ScaledUVArea == TotalUVArea * UVScale^2
		float RebalanceScale = UVScale * YMath::Sqrt( TotalUVArea / ScaledUVArea );
		if( RebalanceScale < 1.01f )
		{
			// Stop if further rebalancing is minor
			break;
		}
		NonuniformScale = RebalanceScale;
	}
#endif

	// Sort charts from largest to smallest
	struct FCompareCharts
	{
		FORCEINLINE bool operator()( const FMeshChart& A, const FMeshChart& B ) const
		{
			// Rect area
			YVector2 ChartRectA = ( A.MaxUV - A.MinUV ) * A.UVScale;
			YVector2 ChartRectB = ( B.MaxUV - B.MinUV ) * B.UVScale;
			return ChartRectA.x * ChartRectA.y > ChartRectB.x * ChartRectB.y;
		}
	};
	//Algo::IntroSort( Charts, FCompareCharts() );
    std::sort(Charts.begin(), Charts.end(), FCompareCharts());
}

// Hash function to use FMD5Hash in TMap
inline uint32 GetTypeHash(const FMD5Hash& Hash)
{
	uint32* HashAsInt32 = (uint32*)Hash.GetBytes();
	return HashAsInt32[0] ^ HashAsInt32[1] ^ HashAsInt32[2] ^ HashAsInt32[3];
}

bool FLayoutUV::FChartPacker::PackCharts(std::vector< FMeshChart >& Charts, float UVScale, const std::vector< YVector2 >& TexCoords, const std::vector< uint32 >& SortedTris, float& OutEfficiency, std::atomic<bool>& bAbort, bool bTrace)
{
	ScaleCharts( Charts, UVScale );
	//TRACE_CPUPROFILER_EVENT_SCOPE(FChartPacker::PackCharts)

	FAllocator2D BestChartRaster(FAllocator2D::EMode::UsedSegments, TextureResolution, TextureResolution, LayoutVersion);
	FAllocator2D ChartRaster    (FAllocator2D::EMode::UsedSegments, TextureResolution, TextureResolution, LayoutVersion);
	FAllocator2D LayoutRaster   (FAllocator2D::EMode::FreeSegments, TextureResolution, TextureResolution, LayoutVersion);

	uint64 RasterizeCycles = 0;
	uint64 FindCycles = 0;

	//double BeginPackCharts = FPlatformTime::Seconds();

	OutEfficiency = 0.0f;
	LayoutRaster.Clear();

	// Store the position where we found a spot for each unique raster
	// so we can skip whole sections we know won't work out.
	// This method is obviously more efficient with smaller charts
	// but helps tremendously as the number of charts goes up for
	// the same texture space. This helps counteract the slowdown
	// induced by having more parts to place in the grid and is
	// particularly useful for foliage.
	std::map<FMD5Hash, YVector2> BestStartPos;

	// Reduce Insights CPU tracing to once per batch
	const int32 BatchSize = 1024;
	for( int32 ChartIndex = 0; ChartIndex < Charts.size() && !bAbort.load(std::memory_order::memory_order_relaxed);)
	{
		//TRACE_CPUPROFILER_EVENT_SCOPE(ChartBatch);
		for( int32 BatchIndex = 0; BatchIndex < BatchSize && ChartIndex < Charts.size() && !bAbort.load(std::memory_order::memory_order_relaxed); ++ChartIndex, ++BatchIndex)
		{
			FMeshChart& Chart = Charts[ChartIndex];

			// Try different orientations and pick best
			int32				BestOrientation = -1;
			FAllocator2D::FRect	BestRect = { ~0u, ~0u, ~0u, ~0u };

			// Refactored BestRect comparison code in one place so this can be customized per version if needed
			std::function<bool (const FAllocator2D::FRect&)> IsBestRect;
			if ( LayoutVersion >= ELightmapUVVersion::OptimalSurfaceArea )
			{
				// This version focus on minimal surface area giving fairness to both horizontal and vertical chart placement
				// instead of only taking the pixel offset of the lower left corner into account.
				IsBestRect = 
					[&BestRect](const FAllocator2D::FRect& Rect)
					{
						return ((Rect.X+Rect.W) + (Rect.Y+Rect.H)) < ((BestRect.X+BestRect.W) + (BestRect.Y+BestRect.H));
					};
			}
			else
			{
				IsBestRect = 
					[this, &BestRect](const FAllocator2D::FRect& Rect)
					{
						return Rect.X + Rect.Y * TextureResolution < BestRect.X + BestRect.Y * TextureResolution;
					};
			}

			for( int32 Orientation = 0; Orientation < 8; Orientation++ )
			{
				// TODO If any dimension is less than 1 pixel shrink dimension to zero

				OrientChart( Chart, Orientation);
			
				YVector2 ChartSize = Chart.MaxUV - Chart.MinUV;
				ChartSize = ChartSize.x * Chart.PackingScaleU + ChartSize.y * Chart.PackingScaleV;

				// Only need half pixel dilate for rects
				FAllocator2D::FRect	Rect;
				Rect.X = 0;
				Rect.Y = 0;
				Rect.W = YMath::CeilToInt( YMath::Abs( ChartSize.x ) + 1.0f );
				Rect.H = YMath::CeilToInt( YMath::Abs( ChartSize.y ) + 1.0f );

				// Just in case lack of precision pushes it over
				Rect.W = YMath::Min( TextureResolution, Rect.W );
				Rect.H = YMath::Min( TextureResolution, Rect.H );

				const bool bRectPack = false;

				if( bRectPack )
				{
					if( LayoutRaster.Find( Rect ) )
					{
						if( IsBestRect(Rect) )
						{
							BestOrientation = Orientation;
							BestRect = Rect;
						}
					}
					else
					{
						continue;
					}
				}
				else
				{
					if ( LayoutVersion >= ELightmapUVVersion::Segments && Orientation % 4 == 1 )
					{
						ChartRaster.FlipX( Rect );
					}
					else if ( LayoutVersion >= ELightmapUVVersion::Segments && Orientation % 4 == 3 )
					{
						ChartRaster.FlipY( Rect );
					}
					else
					{
						//int32 BeginRasterize = FPlatformTime::Cycles();
						RasterizeChart( Chart, TexCoords, SortedTris, Rect.W , Rect.H, ChartRaster);
						//RasterizeCycles += FPlatformTime::Cycles() - BeginRasterize;
					}

					bool bFound = false;

					//uint32 BeginFind = FPlatformTime::Cycles();
					if ( LayoutVersion == ELightmapUVVersion::BitByBit )
					{
						bFound = LayoutRaster.FindBitByBit( Rect, ChartRaster );
					}
					else if ( LayoutVersion >= ELightmapUVVersion::Segments )
					{
						// Use the real raster size for optimal placement
						FAllocator2D::FRect RasterRect = Rect;
						RasterRect.W = ChartRaster.GetRasterWidth();
						RasterRect.H = ChartRaster.GetRasterHeight();

						// Nothing rasterized, returning 0,0 as fast as possible
						// since this is what the actual algorithm is doing but
						// we might have to flag the entire UV map as invalid since
						// charts are going to overlap
						if (RasterRect.H == 0 && RasterRect.W == 0)
						{
							Rect.X = 0;
							Rect.Y = 0;
							bFound = true;
						}
						else
						{
							FMD5Hash RasterMD5 = ChartRaster.GetRasterMD5();
							//YVector2* StartPos = BestStartPos.Find(RasterMD5);
                            auto StartPos = BestStartPos.find(RasterMD5);
							if (StartPos!=BestStartPos.end())
							{
								RasterRect.X = StartPos->second.x;
								RasterRect.Y = StartPos->second.y;
							}

							LayoutRaster.ResetStats();
							bFound = LayoutRaster.FindWithSegments(RasterRect, ChartRaster, IsBestRect);
							if (bFound)
							{
								// Store only the best possible position in the hash table so we can start from there for other identical charts
								BestStartPos[RasterMD5] = YVector2(RasterRect.X, RasterRect.Y);

								// Since the older version stops searching at Width - Rect.W instead of using the raster size,
								// it means a perfect rasterized square of 2,2 won't fit a 2,2 hole at the end of a row if Rect.W = 3.
								// Because of that, we have no choice to worsen our algorithm behavior for backward compatibility.

								// Once we know the best possible position, we'll continue our search from there with the original
								// rect value if it differs from the raster rect to ensure we get the same result as the old algorithm.
								if (LayoutVersion < ELightmapUVVersion::Segments2D && (Rect.X != RasterRect.X || Rect.Y != RasterRect.Y))
								{
									Rect.X = RasterRect.X;
									Rect.Y = RasterRect.Y;

									bFound = LayoutRaster.FindWithSegments(Rect, ChartRaster, IsBestRect);
								}
								else
								{
									// We can't copy W and H here as they might be different than what we got initially
									Rect.X = RasterRect.X;
									Rect.Y = RasterRect.Y;
								}
							}

							LayoutRaster.PublishStats(ChartIndex, Orientation, bFound, Rect, BestRect, RasterMD5, IsBestRect);
						}

					}
					//FindCycles += FPlatformTime::Cycles() - BeginFind;

					if (bTrace)
					{
						//UE_LOG(LogLayoutUV, Log, TEXT("[LAYOUTUV_TRACE] Chart %d Orientation %d Found = %d Rect = %d,%d,%d,%d\n"), ChartIndex, Orientation, bFound ? 1 : 0, Rect.X, Rect.Y, Rect.W, Rect.H);
					}

					if( bFound )
					{
						if( IsBestRect(Rect) )
						{
							BestChartRaster = ChartRaster;

							BestOrientation = Orientation;
							BestRect = Rect;

							if ( BestRect.X == 0 && BestRect.Y == 0 )
							{
								// BestRect can't be beat, stop here
								break;
							}
						}
					}
					else
					{
						continue;
					}
				}
			}

			if( BestOrientation >= 0 )
			{
				// Add chart to layout
				OrientChart( Chart, BestOrientation );

				LayoutRaster.Alloc( BestRect, BestChartRaster );

				Chart.PackingBias.x += BestRect.X;
				Chart.PackingBias.y += BestRect.Y;
			}
			else
			{
				if (bTrace)
				{
					//UE_LOG(LogLayoutUV, Log, TEXT("[LAYOUTUV_TRACE] Chart %d Found no orientation that fit\n"), ChartIndex);
				}
				// Found no orientation that fit
				return false;
			}
		}
	}

	if (bAbort)
	{
		return false;
	}

	const uint32 TotalTexels = TextureResolution * TextureResolution;
	const uint32 UsedTexels  = LayoutRaster.GetUsedTexels();

	OutEfficiency = float( UsedTexels ) / TotalTexels;
	//double EndPackCharts = FPlatformTime::Seconds();

	//UE_LOG(LogLayoutUV, VeryVerbose, TEXT("PackCharts: %s"),	*FPlatformTime::PrettyTime(EndPackCharts - BeginPackCharts));
	//UE_LOG(LogLayoutUV, VeryVerbose, TEXT("  Rasterize: %llu"),	RasterizeCycles);
	//UE_LOG(LogLayoutUV, VeryVerbose, TEXT("  Find: %llu"),		FindCycles);

	return true;
}

void FLayoutUV::FChartPacker::OrientChart( FMeshChart& Chart, int32 Orientation )
{
	switch( Orientation )
	{
	case 0:
		// 0 degrees
		Chart.PackingScaleU = YVector2( Chart.UVScale.x, 0 );
		Chart.PackingScaleV = YVector2( 0, Chart.UVScale.y );
		Chart.PackingBias = -Chart.MinUV.x * Chart.PackingScaleU - Chart.MinUV.y * Chart.PackingScaleV + 0.5f;
		break;
	case 1:
		// 0 degrees, flip x
		Chart.PackingScaleU = YVector2( -Chart.UVScale.x, 0 );
		Chart.PackingScaleV = YVector2( 0, Chart.UVScale.y );
		Chart.PackingBias = -Chart.MaxUV.x * Chart.PackingScaleU - Chart.MinUV.y * Chart.PackingScaleV + 0.5f;
		break;
	case 2:
		// 90 degrees
		Chart.PackingScaleU = YVector2( 0, -Chart.UVScale.x );
		Chart.PackingScaleV = YVector2( Chart.UVScale.y, 0 );
		Chart.PackingBias = -Chart.MaxUV.x * Chart.PackingScaleU - Chart.MinUV.y * Chart.PackingScaleV + 0.5f;
		break;
	case 3:
		// 90 degrees, flip x
		Chart.PackingScaleU = YVector2( 0, Chart.UVScale.x );
		Chart.PackingScaleV = YVector2( Chart.UVScale.y, 0 );
		Chart.PackingBias = -Chart.MinUV.x * Chart.PackingScaleU - Chart.MinUV.y * Chart.PackingScaleV + 0.5f;
		break;
	case 4:
		// 180 degrees
		Chart.PackingScaleU = YVector2( -Chart.UVScale.x, 0 );
		Chart.PackingScaleV = YVector2( 0, -Chart.UVScale.y );
		Chart.PackingBias = -Chart.MaxUV.x * Chart.PackingScaleU - Chart.MaxUV.y * Chart.PackingScaleV + 0.5f;
		break;
	case 5:
		// 180 degrees, flip x
		Chart.PackingScaleU = YVector2( Chart.UVScale.x, 0 );
		Chart.PackingScaleV = YVector2( 0, -Chart.UVScale.y );
		Chart.PackingBias = -Chart.MinUV.x * Chart.PackingScaleU - Chart.MaxUV.y * Chart.PackingScaleV + 0.5f;
		break;
	case 6:
		// 270 degrees
		Chart.PackingScaleU = YVector2( 0, Chart.UVScale.x );
		Chart.PackingScaleV = YVector2( -Chart.UVScale.y, 0 );
		Chart.PackingBias = -Chart.MinUV.x * Chart.PackingScaleU - Chart.MaxUV.y * Chart.PackingScaleV + 0.5f;
		break;
	case 7:
		// 270 degrees, flip x
		Chart.PackingScaleU = YVector2( 0, -Chart.UVScale.x );
		Chart.PackingScaleV = YVector2( -Chart.UVScale.y, 0 );
		Chart.PackingBias = -Chart.MaxUV.x * Chart.PackingScaleU - Chart.MaxUV.y * Chart.PackingScaleV + 0.5f;
		break;
	}
}

// Max of 2048x2048 due to precision
// Dilate in 28.4 fixed point. Half pixel dilation is conservative rasterization.
// Dilation same as Minkowski sum of triangle and square.
template< int32 Dilate >
void RasterizeTriangle( FAllocator2D& Shader, const YVector2 Points[3], int32 ScissorWidth, int32 ScissorHeight )
{
	const YVector2 HalfPixel( 0.5f, 0.5f );
	YVector2 p0 = Points[0] - HalfPixel;
	YVector2 p1 = Points[1] - HalfPixel;
	YVector2 p2 = Points[2] - HalfPixel;

	// Correct winding
	float Facing = ( p0.x - p1.x ) * ( p2.y - p0.y ) - ( p0.y - p1.y ) * ( p2.x - p0.x );
	if( Facing < 0.0f )
	{
		std::swap( p0, p2 );
	}

	// 28.4 fixed point
	const int32 X0 = (int32)( 16.0f * p0.x + 0.5f );
	const int32 X1 = (int32)( 16.0f * p1.x + 0.5f );
	const int32 X2 = (int32)( 16.0f * p2.x + 0.5f );
	
	const int32 Y0 = (int32)( 16.0f * p0.y + 0.5f );
	const int32 Y1 = (int32)( 16.0f * p1.y + 0.5f );
	const int32 Y2 = (int32)( 16.0f * p2.y + 0.5f );

	// Bounding rect
	int32 MinX = ( YMath::Min3( X0, X1, X2 ) - Dilate + 15 ) / 16;
	int32 MaxX = ( YMath::Max3( X0, X1, X2 ) + Dilate + 15 ) / 16;
	int32 MinY = ( YMath::Min3( Y0, Y1, Y2 ) - Dilate + 15 ) / 16;
	int32 MaxY = ( YMath::Max3( Y0, Y1, Y2 ) + Dilate + 15 ) / 16;

	// Clip to image
	MinX = YMath::Clamp( MinX, 0, ScissorWidth );
	MaxX = YMath::Clamp( MaxX, 0, ScissorWidth );
	MinY = YMath::Clamp( MinY, 0, ScissorHeight );
	MaxY = YMath::Clamp( MaxY, 0, ScissorHeight );

	// Deltas
	const int32 DX01 = X0 - X1;
	const int32 DX12 = X1 - X2;
	const int32 DX20 = X2 - X0;

	const int32 DY01 = Y0 - Y1;
	const int32 DY12 = Y1 - Y2;
	const int32 DY20 = Y2 - Y0;

	// Half-edge constants
	int32 C0 = DY01 * X0 - DX01 * Y0;
	int32 C1 = DY12 * X1 - DX12 * Y1;
	int32 C2 = DY20 * X2 - DX20 * Y2;

	// Correct for fill convention
	C0 += ( DY01 < 0 || ( DY01 == 0 && DX01 > 0 ) ) ? 0 : -1;
	C1 += ( DY12 < 0 || ( DY12 == 0 && DX12 > 0 ) ) ? 0 : -1;
	C2 += ( DY20 < 0 || ( DY20 == 0 && DX20 > 0 ) ) ? 0 : -1;

	// Dilate edges
	C0 += ( abs(DX01) + abs(DY01) ) * Dilate;
	C1 += ( abs(DX12) + abs(DY12) ) * Dilate;
	C2 += ( abs(DX20) + abs(DY20) ) * Dilate;

	for( int32 y = MinY; y < MaxY; y++ )
	{
		for( int32 x = MinX; x < MaxX; x++ )
		{
			// same as Edge1 >= 0 && Edge2 >= 0 && Edge3 >= 0
			int32 IsInside;
			IsInside  = C0 + (DX01 * y - DY01 * x) * 16;
			IsInside |= C1 + (DX12 * y - DY12 * x) * 16;
			IsInside |= C2 + (DX20 * y - DY20 * x) * 16;

			if( IsInside >= 0 )
			{
				Shader.SetBit( x, y );
			}
		}
	}
}

void FLayoutUV::FChartPacker::RasterizeChart( const FMeshChart& Chart, const std::vector< YVector2 >& TexCoords, const std::vector< uint32 >& SortedTris, uint32 RectW, uint32 RectH, FAllocator2D& OutChartRaster )
{
	// Bilinear footprint is -1 to 1 pixels. If packed geometrically, only a half pixel dilation
	// would be needed to guarantee all charts were at least 1 pixel away, safe for bilinear filtering.
	// Unfortunately, with pixel packing a full 1 pixel dilation is required unless chart edges exactly
	// align with pixel centers.

	OutChartRaster.Clear();

	for( uint32 Tri = Chart.FirstTri; Tri < Chart.LastTri; Tri++ )
	{
		YVector2 Points[3];
		for ( int k = 0; k < 3; k++ )
		{
			const YVector2& UV = TexCoords[ 3 * SortedTris[ Tri ] + k ];
			Points[k] =  Chart.PackingScaleU* UV.x  +  Chart.PackingScaleV* UV.y  + Chart.PackingBias;
		}

		RasterizeTriangle< 16 >( OutChartRaster, Points, RectW, RectH );
	}

	if ( LayoutVersion >= ELightmapUVVersion::Segments )
	{
		OutChartRaster.CreateUsedSegments();
	}
}

bool FLayoutUV::FindBestPacking(uint32 InTextureResolution)
{
	FChartPacker Packer(MeshView, LayoutVersion, InTextureResolution);
	bool bPackingFound = Packer.FindBestPacking(MeshTexCoords, MeshSortedTris, MeshCharts);
	PackedTextureResolution = bPackingFound ? InTextureResolution : 0;
	return bPackingFound;
}

void FLayoutUV::CommitPackedUVs()
{
	if (PackedTextureResolution == 0)
	{
		return;
	}

	//TRACE_CPUPROFILER_EVENT_SCOPE(FLayoutUV::CommitPackedUVs)

	// Alloc new UV channel
	MeshView.InitOutputTexcoords(MeshTexCoords.size());

	// Commit chart UVs
	for( int32 i = 0; i < MeshCharts.size(); i++ )
	{
		FMeshChart& Chart = MeshCharts[i];

		Chart.PackingScaleU = Chart.PackingScaleU/ PackedTextureResolution;
		Chart.PackingScaleV = Chart.PackingScaleV / PackedTextureResolution;
		Chart.PackingBias = Chart.PackingBias / PackedTextureResolution;

		for( uint32 Tri = Chart.FirstTri; Tri < Chart.LastTri; Tri++ )
		{
			for( int k = 0; k < 3; k++ )
			{
				uint32 Index = 3 * MeshSortedTris[ Tri ] + k;
				const YVector2& UV = MeshTexCoords[ Index ];
				YVector2 TransformedUV =Chart.PackingScaleU * UV.x  +  Chart.PackingScaleV * UV.y  + Chart.PackingBias;
				MeshView.SetOutputTexcoord(Index, TransformedUV);
			}
		}
	}
}

inline bool FLayoutUV::FChartFinder::PositionsMatch( uint32 a, uint32 b ) const
{
	return ( MeshView.GetPosition(a) - MeshView.GetPosition(b) ).IsNearlyZero( THRESH_POINTS_ARE_SAME );
}

inline bool FLayoutUV::FChartFinder::NormalsMatch( uint32 a, uint32 b ) const
{
	return ( MeshView.GetNormal(a) - MeshView.GetNormal(b) ).IsNearlyZero( THRESH_NORMALS_ARE_SAME );
}

inline bool FLayoutUV::FChartFinder::UVsMatch( uint32 a, uint32 b ) const
{
	return ( MeshView.GetInputTexcoord(a) - MeshView.GetInputTexcoord(b) ).IsNearlyZero(UVLAYOUT_THRESH_UVS_ARE_SAME);
}

inline bool FLayoutUV::FChartFinder::VertsMatch( uint32 a, uint32 b ) const
{
	return PositionsMatch( a, b ) && UVsMatch( a, b );
}

// Signed UV area
inline float FLayoutUV::FChartFinder::TriangleUVArea( uint32 Tri ) const
{
	YVector2 UVs[3];
	for( int k = 0; k < 3; k++ )
	{
		UVs[k] = MeshView.GetInputTexcoord(3 * Tri + k);
	}

	YVector2 EdgeUV1 = UVs[1] - UVs[0];
	YVector2 EdgeUV2 = UVs[2] - UVs[0];
	return 0.5f * ( EdgeUV1.x * EdgeUV2.y - EdgeUV1.y * EdgeUV2.x );
}

inline void FLayoutUV::FChartFinder::DisconnectChart( std::vector< FMeshChart >& Charts, FMeshChart& Chart, uint32 Side )
{
	if( Chart.Join[ Side ] != -1 )
	{
		Charts[ Chart.Join[ Side ] ].Join[ Side ^ 1 ] = -1;
		Chart.Join[ Side ] = -1;
	}
}

inline float FLayoutUV::FChartFinder::GetUVEqualityThreshold() const
{
	return LayoutVersion >= ELightmapUVVersion::SmallChartPacking ? NEW_UVS_ARE_SAME : LEGACY_UVS_ARE_SAME;
}
