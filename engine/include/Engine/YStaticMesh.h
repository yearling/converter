#pragma once
#include <vector>
#include "Engine/YRawMesh.h"
#include <memory>
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "Engine/YCamera.h"
#include "YReferenceCount.h"
#include "SObject/STexture.h"

struct StaticVertexRenderData
{
    struct IndexOffsetAndTriangleCount
    {
        uint32 offset = -1;
        uint32 triangle_count = -1;
        uint32 min_vertex_index = -1;
        uint32 max_vertex_index = -1;
    };
    std::vector<YVector> position;
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
    bool use_32_indices = true;
    std::vector<IndexOffsetAndTriangleCount> sections;
    //void GenerateIndexBuffers(const std::vector<std::vector<uint32>>& section_indices);
    void GenerateIndexBuffers(std::vector<uint32>& indices_32, std::vector<uint16>& indices_16, const std::vector<std::vector<uint32>>& section_indices, bool genereate_section_info = false);

    enum VertexInfoType
    {
        Hi_precision,
        Medium,
        low
    };
    VertexInfoType vertex_info_type;
    virtual uint32 GetVertexInfoSize();
    virtual void* GetVertexInfoData();
};
struct HiSttaticVertexData :public StaticVertexRenderData
{
    HiSttaticVertexData()
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

struct MediumStaticVertexData :public StaticVertexRenderData
{
    MediumStaticVertexData()
    {
        vertex_info_type = StaticVertexRenderData::Medium;
    }
    struct MediumVertexInfo
    {
        YPackedNormal normal;
        YPackedNormal tangent;
        FVector2DHalf uv0;
        FVector2DHalf uv1;
    };
    std::vector<MediumVertexInfo> vertex_infos;
    uint32 GetVertexInfoSize() override;
    void* GetVertexInfoData() override;
};

class YStaticMesh
{
public:
	YStaticMesh();
	bool AllocGpuResource();
	void ReleaseGPUReosurce();
	void	Render(CameraBase* camera);
	void Render(class RenderParam* render_param);
	std::vector<YLODMesh> raw_meshes;

	bool SaveV0(const std::string& dir);
	bool LoadV0(const std::string& file_path);
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
};
