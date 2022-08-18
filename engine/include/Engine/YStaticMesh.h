#pragma once
#include <vector>
#include "Engine/YRawMesh.h"
#include <memory>
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "Engine/YCamera.h"
#include "YReferenceCount.h"
#include "SObject/STexture.h"
#include "Engine/EnumAsByte.h"

struct StaticVertexRenderData
{
    enum VertexInfoType
    {
        Hi_precision,
        Default,
        low
    };
    TEnumAsByte<VertexInfoType> vertex_info_type;

    struct IndexOffsetAndTriangleCount
    {
        uint32 offset = -1;
        uint32 triangle_count = -1;
        uint32 min_vertex_index = -1;
        uint32 max_vertex_index = -1;
    };
    std::vector<YVector> position;
    bool use_32_indices = true;
    bool has_reverse_index = false;
    bool has_depth_only_index = false;
    bool has_reverse_depth_only_index = false;
    bool has_adjacent_index = false;
    std::vector<uint32> indices_vertex_32;
    std::vector<uint32> indices_vertex_reversed_32;
    std::vector<uint32> indices_depth_only_32;
    std::vector<uint32> indices_depth_only_reversed_32;
    std::vector<uint32> indices_adjacent_32;
    std::vector<uint16> indices_vertex_16;
    std::vector<uint16> indices_vertex_reversed_16;
    std::vector<uint16> indices_depth_only_16;
    std::vector<uint16> indices_depth_only_reversed_16;
    std::vector<uint16> indices_adjacent_16;
    std::vector<IndexOffsetAndTriangleCount> sections;

    YBox aabb;
    void GenerateIndexBuffers(std::vector<uint32>& indices_32, std::vector<uint16>& indices_16, const std::vector<std::vector<uint32>>& section_indices, bool genereate_section_info = false);

  
    virtual uint32 GetVertexInfoSize();
    virtual void* GetVertexInfoData();
};

YArchive& operator<<(YArchive& mem_file, StaticVertexRenderData::IndexOffsetAndTriangleCount& index_info);
YArchive& operator<<(YArchive& mem_file, StaticVertexRenderData& static_vertex_render_data);

struct HiStaticVertexData :public StaticVertexRenderData
{
    HiStaticVertexData()
    {
        vertex_info_type = StaticVertexRenderData::Hi_precision;
    }
    struct HiVertexInfo
    {
        YVector4 normal;
        YVector4 tangent;
        YVector2 uv0;
        YVector2 uv1;
        int color;
    };
    std::vector<HiVertexInfo> vertex_infos;
    uint32 GetVertexInfoSize() override;
    void* GetVertexInfoData() override;
};
namespace std
{
    template<>
    struct is_pod<HiStaticVertexData::HiVertexInfo>
    {
        static constexpr bool  value = true;
    };
}
YArchive& operator<<(YArchive& mem_file, HiStaticVertexData& imported_raw_mesh);

struct DefaultStaticVertexData :public StaticVertexRenderData
{
    DefaultStaticVertexData()
    {
        vertex_info_type = StaticVertexRenderData::Default;
    }
    struct DefaultVertexInfo
    {
        YPackedNormal normal;
        YPackedNormal tangent;
        FVector2DHalf uv0;
        FVector2DHalf uv1;
    };
    std::vector<DefaultVertexInfo> vertex_infos;
    uint32 GetVertexInfoSize() override;
    void* GetVertexInfoData() override;
};
namespace std
{
    template<>
    struct is_pod<DefaultStaticVertexData::DefaultVertexInfo>
    {
        static constexpr bool  value = true;
    };
}
YArchive& operator<<(YArchive& mem_file, DefaultStaticVertexData& imported_raw_mesh);

class YStaticMesh
{
public:
    YStaticMesh();
    bool AllocGpuResource();
    void ReleaseGPUReosurce();
    void	Render(CameraBase* camera);
    void Render(class RenderParam* render_param);

    bool SaveV0(const std::string& dir);
    bool LoadV0(const std::string& file_path);
    bool SaveRuntimeData(const std::string& dir);
    bool LoadRuntimeData(const std::string& file_path);
    bool SaveEditorData(const std::string& dir);
    bool LoadEditorData(const std::string& file_path);
    std::vector<YLODMesh> raw_meshes;
    std::vector<std::unique_ptr<ImportedRawMesh>> imported_raw_meshes_;
    std::vector<std::unique_ptr< StaticVertexRenderData>> lod_render_data_;
public:
    friend class YStaticMeshVertexFactory;
    bool allocated_gpu_resource = false;
    std::vector<TComPtr<ID3D11Buffer>> vertex_buffers_;
    TComPtr<ID3D11Buffer> index_buffer_;
    TComPtr<ID3D11BlendState> bs_;
    TComPtr<ID3D11DepthStencilState> ds_;
    TComPtr<ID3D11RasterizerState> rs_;
    D3DTextureSampler* sampler_state_ = { nullptr };
    std::vector<int> polygon_group_offsets;
    std::unique_ptr<D3DVertexShader> vertex_shader_;
    std::unique_ptr<D3DPixelShader> pixel_shader_;
    std::unique_ptr<DXVertexFactory> vertex_factory_;
    std::string model_name;
    TRefCountPtr<STexture> diffuse_tex_;
    std::vector<std::shared_ptr<YImportedMaterial>> imported_materials_;
};
