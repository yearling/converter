#pragma once
#include <vector>
#include <unordered_set>
#include "Engine/YCommonHeader.h"
#include "Math/YVector.h"
#include "Math/YMath.h"
#include "Math/NumericLimits.h"

/** Helper struct for building acceleration structures. */
struct FIndexAndZ
{
    float Z = -MIN_flt;
    int32 Index = INVALID_ID;

    /** Default constructor. */
    FIndexAndZ() {}

    /** Initialization constructor. */
    FIndexAndZ(int32 InIndex, YVector V)
    {
        Z = 0.30f * V.x + 0.33f * V.y + 0.37f * V.z;
        Index = InIndex;
    }
};

/** Sorting function for vertex Z/index pairs. */
struct FCompareIndexAndZ
{
    FORCEINLINE bool operator()(FIndexAndZ const& A, FIndexAndZ const& B) const { return A.Z < B.Z; }
};


/**
* Container to hold overlapping corners. For a vertex, lists all the overlapping vertices
*/
struct YOverlappingCorners
{
    YOverlappingCorners() {}

    YOverlappingCorners(const std::vector<YVector>& InVertices, const std::vector<uint32>& InIndices, float ComparisonThreshold = THRESH_POINTS_ARE_SAME);

    /* Resets, pre-allocates memory, marks all indices as not overlapping in preperation for calls to Add() */
    void Init(int32 NumIndices);

    /* Add overlapping indices pair */
    void Add(int32 Key, int32 Value);

    /* Sorts arrays, converts sets to arrays for sorting and to allow simple iterating code, prevents additional adding */
    void FinishAdding();

    /* Estimate memory allocated */
    //uint32 GetAllocatedSize(void) const;

    /**
    * @return array of sorted overlapping indices including input 'Key', empty array for indices that have no overlaps.
    */
    const std::vector<int32>& FindIfOverlapping(int32 Key) const
    {
        check(bFinishedAdding);
        int32 ContainerIndex = IndexBelongsTo[Key];
        return (ContainerIndex != INDEX_NONE) ? Arrays[ContainerIndex] : EmptyArray;
    }

private:
    std::vector<int32> IndexBelongsTo;
    std::vector< std::vector<int32> > Arrays;
    std::vector< std::unordered_set<int32> > Sets;
    std::vector<int32> EmptyArray;
    bool bFinishedAdding = false;
};