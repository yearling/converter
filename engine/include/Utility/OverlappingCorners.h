#pragma once
#include "Engine/YCommonHeader.h"
#include <vector>
#include "Math/YVector.h"
#include <unordered_set>
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