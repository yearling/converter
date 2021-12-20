#pragma once
#include "RHI/DirectX11/D3D11Device.h"
#include <string>
#include <vector>
#include "RHI/DirectX11/D3D11Shader.h"
#include "Engine/YCamera.h"

class D3DTextureSampler;
struct YLODMesh;
enum VertexAttribute
{
    VA_POSITION,
    VA_UV0,
    VA_UV1,
    VA_NORMAL,
    VA_TANGENT,
    VA_BITANGENT,
    VA_COLOR,
    VA_ATTRIBUTE0,
    VA_ATTRIBUTE1,
    VA_ATTRIBUTE2,
    VA_ATTRIBUTE3,
    VA_ATTRIBUTE4,
    VA_ATTRIBUTE5,
    VA_ATTRIBUTE6,
    VA_ATTRIBUTE7
};
enum DataType
{
    Float32,
    Uint8
};
struct VertexStreamDescription {
    VertexStreamDescription();
    VertexStreamDescription(VertexAttribute in_vertex_attribe,const std::string& in_name,DataType in_type, int in_cpu_data_index,int in_com_num,int in_buffer_size,int in_slot,int in_stride,bool in_normalized,bool in_release,bool in_dynamic);
    VertexAttribute vertex_attribute;
    std::string name;
    DataType data_type;
    int cpu_data_index;
    int com_num;
    int buffer_size;
    int slot;
    int stride;
    bool normalized:1;
    bool release:1;
    bool dynamic : 1;
};
class IVertexFactory {
  public:
    IVertexFactory();
    virtual ~IVertexFactory();
    virtual bool SetVertexStreamDescriptions(std::vector<VertexStreamDescription>&& descriptions);
    virtual bool AllocGPUResource(YLODMesh* mesh) = 0;
    virtual bool ReleaseGPUResource();
    virtual bool UpdateVertexStreamBuffer(VertexAttribute vertex_attribute, unsigned char* buffer_data, unsigned int buffer_in_byte)=0; 
    const std::vector<VertexStreamDescription>& GetVertexDescription() const;
    std::vector<VertexStreamDescription>& GetVertexDescription();
    virtual bool IsGPUSKin() const;
    virtual bool IsMorph() const;
    virtual void SetUseGPUSKin(bool use_gpu_skin) ;
    virtual void SetUseMorph(bool has_mporh) ;
    virtual bool HasCustomData() const;
    virtual bool SetupVertexStreams() = 0;
  protected:
    std::vector<VertexStreamDescription> vertex_descriptions_;
    bool is_alloc_resource_:1;
    bool is_gpu_skin_ : 1;
    bool is_morph_ : 1;
    bool has_custom_data_:1;
};

class DXVertexFactory:public IVertexFactory {
  public:
    DXVertexFactory();
    ~DXVertexFactory() override;
    bool AllocGPUResource(YLODMesh* mesh) override;
    bool ReleaseGPUResource() override;
    bool SetVertexStreamDescriptions(std::vector<VertexStreamDescription>&& descriptions) override;
    bool UpdateVertexStreamBuffer(VertexAttribute vertex_attribute, unsigned char* buffer_data, unsigned int buffer_in_byte) override; 
    virtual bool SetupVertexStreams() override;
    void SetInputLayout(const TComPtr<ID3D11InputLayout>& input_layout);
    void SetRenderState();
    void DrawCall(CameraBase* camera);
  private:
    std::vector<TComPtr<ID3D11Buffer>> vertex_buffers_;
    TComPtr<ID3D11Buffer> index_buffer_;
    TComPtr<ID3D11InputLayout> vertex_input_layout_;
    TComPtr<ID3D11BlendState> bs_;
    TComPtr<ID3D11DepthStencilState> ds_;
    TComPtr<ID3D11RasterizerState> rs_;
    D3DTextureSampler* sampler_state_ = {nullptr};
    std::vector<int> polygon_group_offsets;
    std::unique_ptr<D3DVertexShader> vertex_shader_;
    std::unique_ptr<D3DPixelShader> pixel_shader_;
};