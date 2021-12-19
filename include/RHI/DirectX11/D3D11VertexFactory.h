#pragma once
#include "RHI/DirectX11/D3D11Device.h"
#include <string>
#include <vector>
class RenderMesh;
struct ProgramRes;
class D3DTextureSampler;
struct VertexAttribute
{
    
};
enum DataType
{
    Float32,
    Uint8
};
struct VertexStreamDescription {
    VertexAttribute vertex_attribute;
    std::string name;
    DataType type;
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
    virtual bool AllocGPUResource(RenderMesh* mesh) = 0;
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
    DXVertexFactory(D3D11Device* device);
    ~DXVertexFactory() override;
    bool AllocGPUResource(RenderMesh* mesh) override;
    bool ReleaseGPUResource() override;
    bool SetVertexStreamDescriptions(std::vector<VertexStreamDescription>&& descriptions) override;
    bool UpdateVertexStreamBuffer(VertexAttribute vertex_attribute, unsigned char* buffer_data, unsigned int buffer_in_byte) override; 
    virtual bool SetupVertexStreams() override;
    void SetInputLayout(const TComPtr<ID3D11InputLayout>& input_layout);
    void SetRenderState();
    void DrawCall(ProgramRes* program_ptr,RenderMesh* mesh);
  private:
    std::vector<TComPtr<ID3D11Buffer>> vertex_buffers_;
    TComPtr<ID3D11Buffer> index_buffers_;
    TComPtr<ID3D11InputLayout> vertex_input_layout_;
    D3D11Device* device_;
    TComPtr<ID3D11BlendState> bs_;
    TComPtr<ID3D11DepthStencilState> ds_;
    TComPtr<ID3D11RasterizerState> rs_;
    D3DTextureSampler* sampler_state_ = {nullptr};
};