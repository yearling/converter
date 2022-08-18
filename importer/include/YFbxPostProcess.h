#pragma once
#include "Engine/YCommonHeader.h"
#include <vector>
#include "Math/YVector.h"
#include "Engine/YPackedNormal.h"
#include "Math/Vector2DHalf.h"
#include "Engine/YRawMesh.h"
#include "Engine/YStaticMesh.h"
#include "YFbxImporter.h"

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
    PostProcessRenderMesh(ImportedRawMesh* raw_mesh, FbxImportParam* in_fbx_import_param);
    //不在raw mesh中做是为了保存rawmesh的原始结构
    void PostProcessPipeline();
   
    std::unique_ptr< HiStaticVertexData> GenerateHiStaticVertexData();
    std::unique_ptr< DefaultStaticVertexData> GenerateMediumStaticVertexData();
protected:
    void CompressVertex();
    void OptimizeIndices();
    void BuildStaticAdjacencyIndexBuffer();
    void BuildReverseIndices();
    void BuildDepthOnlyIndices();
    void BuildDepthOnlyInverseIndices();

protected:
    friend class FStaticMeshNvRenderBuffer;
    ImportedRawMesh* raw_mesh_{ nullptr };
    std::vector<FullStaticVertexData> vertex_data_cache;
    std::vector<std::vector<uint32>> section_indices;
    std::vector<std::vector<uint32>> reversed_indices;
    std::vector<std::vector<uint32>> depth_only_indices;
    std::vector<std::vector<uint32>> depth_only_reversed_indices;
    std::vector<std::vector<uint32>> adjacency_section_indices;
    bool generate_reverse_index = true;
    bool generate_depth_only_index = true;
    bool generate_reverse_depth_only_index = true;
    bool generate_adjacent_index = true;
};
