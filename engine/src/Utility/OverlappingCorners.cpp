#include "Utility/OverlappingCorners.h"
#include <algorithm>
#include "Engine/YRawMesh.h"

YOverlappingCorners::YOverlappingCorners(const std::vector<YVector>& InVertices, const std::vector<uint32>& InIndices, float ComparisonThreshold)
{
    const int32 num_wedges = InIndices.size();
    // Create a list of Vertex Z /index pairs;
    std::vector<FIndexAndZ> vertex_index_and_Z;
    vertex_index_and_Z.reserve(num_wedges);

    for (int wedge_index = 0; wedge_index < num_wedges; ++wedge_index)
    {
        FIndexAndZ tmp(wedge_index, InVertices[InIndices[wedge_index]]);
        vertex_index_and_Z.push_back(tmp);
    }
    // Sort the vertices by z value
    std::sort(vertex_index_and_Z.begin(), vertex_index_and_Z.end(), FCompareIndexAndZ());
    Init(num_wedges);

    // Search for duplicates, quickly!
    for (int i = 0; i < vertex_index_and_Z.size(); ++i)
    {
        // only need to search forward, since we add pairs both ways
        for (int j = i+1; j < vertex_index_and_Z.size(); ++j)
        {
            if (YMath::Abs(vertex_index_and_Z[j].Z - vertex_index_and_Z[i].Z) > ComparisonThreshold)
            {
                break;
            }
            const YVector& position0 = InVertices[InIndices[vertex_index_and_Z[i].Index]];
            const YVector& position1 = InVertices[InIndices[vertex_index_and_Z[j].Index]];

            if (PointsEqual(position0, position1, ComparisonThreshold))
            {
                Add(vertex_index_and_Z[i].Index, vertex_index_and_Z[j].Index);
            }
        }
    }
    FinishAdding();
}

void YOverlappingCorners::Init(int32 NumIndices)
{
    Arrays.clear();
    Sets.clear();
    bFinishedAdding = false;
    IndexBelongsTo.resize(NumIndices, INDEX_NONE);
}

void YOverlappingCorners::Add(int32 Key, int32 Value)
{
    check(Key != Value);
    check(bFinishedAdding == false);
    int32 ContainerIndex = IndexBelongsTo[Key];
    if (ContainerIndex == INDEX_NONE)
    {
        ContainerIndex = Arrays.size();
        Arrays.push_back(std::vector<int32>());
        std::vector<int32>& Container = Arrays[ContainerIndex];
        Container.reserve(6);
        Container.push_back(Key);
        Container.push_back(Value);
        IndexBelongsTo[Key] = ContainerIndex;
        IndexBelongsTo[Value] = ContainerIndex;
    }
    else
    {
        IndexBelongsTo[Value] = ContainerIndex;

        std::vector<int32>& ArrayContainer = Arrays[ContainerIndex];
        if (ArrayContainer.size() == 1)
        {
            // Container is a set
            Sets[ArrayContainer.back()].insert(Value);
        }
        else
        {
            // Container is an array
            if (std::find(ArrayContainer.begin(), ArrayContainer.end(), Value) == ArrayContainer.end())
            {
                ArrayContainer.push_back(Value);
            }

            if (std::find(ArrayContainer.begin(), ArrayContainer.end(), Key) == ArrayContainer.end())
            {
                ArrayContainer.push_back(Key);
            }

            // Change container into set when one vertex is shared by large number of triangles
            if (ArrayContainer.size() > 12)
            {
                int32 SetIndex = Sets.size();
                Sets.push_back(std::unordered_set<int32>());
                std::unordered_set<int32>& Set = Sets[SetIndex];
                //Set.Append(ArrayContainer);
                Set.insert(ArrayContainer.begin(), ArrayContainer.end());

                // Having one element means we are using a set
                // An array will never have just 1 element normally because we add them as pairs
                ArrayContainer.clear();
                ArrayContainer.push_back(SetIndex);
            }
        }
    }
}

void YOverlappingCorners::FinishAdding()
{
    check(bFinishedAdding == false);

    for (std::vector<int32>& Array : Arrays)
    {
        // Turn sets back into arrays for easier iteration code
        // Also reduces peak memory later in the import process
        if (Array.size() == 1)
        {
            std::unordered_set<int32>& Set = Sets[Array.back()];
            Array.clear();
            Array.resize(Set.size());
            for (int32 i : Set)
            {
                Array.push_back(i);
            }
        }

        // Sort arrays now to avoid sort multiple times
        std::sort(Array.begin(),Array.end());

    }

    Sets.clear();

    bFinishedAdding = true;
}

