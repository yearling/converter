#pragma once
#include "Engine/YCommonHeader.h"
#include <vector>
#include "Math/YVector.h"
#include "Engine/YPackedNormal.h"
#include "Math/Vector2DHalf.h"
#include "Engine/YRawMesh.h"
#include "Engine/YStaticMesh.h"

struct FullStaticVertexData
{
    YVector position;
    YVector normal;
    YVector4 tangent;
    YVector2 uv0;
    YVector2 uv1;
    YVector4 color;
    bool operator ==(const FullStaticVertexData& other) const;
};

struct PostProcessRenderMesh
{
public:
    PostProcessRenderMesh(ImportedRawMesh* raw_mesh);
    //不在raw mesh中做是为了保存rawmesh的原始结构
    void PostProcessPipeline();
    void CompressVertex();
    void OptimizeIndices();
    void BuildStaticAdjacencyIndexBuffer();
    void BuildReverseIndices();
    void BuildDepthOnlyIndices();
    void BuildDepthOnlyInverseIndices();
    std::unique_ptr< HiSttaticVertexData> GenerateHiStaticVertexData();
    std::unique_ptr< MediumStaticVertexData> GenerateMediumStaticVertexData();
protected:
    friend class FStaticMeshNvRenderBuffer;
    ImportedRawMesh* raw_mesh_;
    std::vector<FullStaticVertexData> vertex_data_cache;
    std::vector<std::vector<uint32>> section_indices;
    std::vector<std::vector<uint32>> reversed_indices;
    std::vector<std::vector<uint32>> depth_only_indices;
    std::vector<std::vector<uint32>> depth_only_reversed_indices;
    std::vector<std::vector<uint32>> adjacency_section_indices;

};