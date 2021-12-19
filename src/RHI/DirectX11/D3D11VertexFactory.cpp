#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "RHI/DirectX11/D3D11Shader.h"
#include "RHI/DirectX11/D3D11Texture.h"
IVertexFactory::IVertexFactory():
	is_alloc_resource_(false),
	is_gpu_skin_(false),
	is_morph_(false),
	has_custom_data_(false){}

IVertexFactory::~IVertexFactory() {
    if (is_alloc_resource_) {
        ReleaseGPUResource();
    }
}

bool IVertexFactory::SetVertexStreamDescriptions(std::vector<VertexStreamDescription>&& descriptions) {
    vertex_descriptions_ = std::move(descriptions);
    return true;
}

bool IVertexFactory::AllocGPUResource(RenderMesh* mesh)
{ return is_alloc_resource_; }

bool IVertexFactory::ReleaseGPUResource() { return true; }

const std::vector<VertexStreamDescription>& IVertexFactory::GetVertexDescription() const { return vertex_descriptions_; }

std::vector<VertexStreamDescription>& IVertexFactory::GetVertexDescription() { return vertex_descriptions_; }

bool IVertexFactory::IsGPUSKin() const { return is_gpu_skin_; }

bool IVertexFactory::IsMorph() const { return is_morph_; }


bool IVertexFactory::HasCustomData() const { return has_custom_data_; }

void IVertexFactory::SetUseGPUSKin(bool use_gpu_skin)  { is_gpu_skin_ = use_gpu_skin; }

void IVertexFactory::SetUseMorph(bool has_mporh) { is_morph_ = has_mporh; }

DXVertexFactory::DXVertexFactory(D3D11Device* device):device_(device) {}

 DXVertexFactory::~DXVertexFactory() {}

bool DXVertexFactory::AllocGPUResource(RenderMesh* mesh) {
     if (IVertexFactory::AllocGPUResource(mesh)) {
         return true;
     }

     // alloc vb
#if 0
     vertex_buffers_.resize(NUM_ATTRIBUTES);
     for (VertexStreamDescription& vertex_desc : vertex_descriptions_) {
         VertexAttribute va = static_cast<VertexAttribute>(vertex_desc.cpu_data_index); 
         std::shared_ptr<SKwai::UnifiedBuffer>& vb_ptr = mesh->GetAttribute(va);
         if (vb_ptr) {
             TComPtr<ID3D11Buffer> d3d_vb;
             MappedData mapped_data = vb_ptr->GetCpuBuffer()->Lock(0);
             if (vertex_desc.dynamic) {
                 if (!device_->CreateVertexBufferDynamic(vb_ptr->GetElementSize() * vb_ptr->GetCount(), mapped_data.data, d3d_vb)) {
                     ERROR_INFO("Create vertex buffer failed!!");
                     return false;
                 }
             } else {
                 if (!device_->CreateVertexBufferStatic(vb_ptr->GetElementSize() * vb_ptr->GetCount(), mapped_data.data, d3d_vb)) {
                     ERROR_INFO("Create vertex buffer failed!!");
                     return false;
                 }
             }
             vb_ptr->GetCpuBuffer()->Unlock(vb_ptr->GetCpuBuffer()->GetSize());
             vertex_buffers_[vertex_desc.cpu_data_index] = d3d_vb;
         }
     }
     // alloc ib
     std::shared_ptr<SKwai::UnifiedBuffer> cpu_ib=mesh->GetIndexBuffer();
     MappedData cpu_ib_mapped_data = cpu_ib->GetCpuBuffer()->Lock(0);
     if (!device_->CreateIndexBuffer(cpu_ib->GetElementSize() * cpu_ib->GetCount(), cpu_ib_mapped_data.data, index_buffers_)) {
          ERROR_INFO("Create vertex buffer failed!!");
          return false;
     }
     cpu_ib->GetCpuBuffer()->Unlock(cpu_ib->GetCpuBuffer()->GetSize());
#endif

     is_alloc_resource_ = true;
     return true;
 }

bool DXVertexFactory::ReleaseGPUResource() {
     vertex_buffers_.clear();
    index_buffers_ = nullptr;
     is_alloc_resource_ = false;
    return true; 
}

bool DXVertexFactory::SetVertexStreamDescriptions(std::vector<VertexStreamDescription>&& descriptions) {
    IVertexFactory::SetVertexStreamDescriptions(std::move(descriptions));
    return true;
}

bool DXVertexFactory::UpdateVertexStreamBuffer(VertexAttribute vertex_attribute, unsigned char* buffer_data, unsigned int buffer_in_byte) {
    return true;
}

bool DXVertexFactory::SetupVertexStreams() { return true; }
  
void DXVertexFactory::SetInputLayout(const TComPtr<ID3D11InputLayout>& input_layout) { vertex_input_layout_ = input_layout; }

void DXVertexFactory::SetRenderState() {
    if (!bs_) {
        device_->CreateBlendState(bs_, true);
    }

    if (!rs_) {
        device_->CreateRasterStateNonCull(rs_);
    }

    if (!ds_) {
        device_->CreateDepthStencileState(ds_,true);
        //device_->CreateDepthStencileStateNoWriteNoTest(ds_);
    }
    if (!sampler_state_) {
        sampler_state_ = device_->GetSamplerState(SF_MIN_MAG_MIP_LINEAR,AM_WRAP);
    }
}

void DXVertexFactory::DrawCall(ProgramRes* program_ptr,RenderMesh* mesh) {
    ID3D11Device* device = device_->GetDevice();
    ID3D11DeviceContext* dc = device_->GetDC();
    float BlendColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    dc->OMSetBlendState(bs_, BlendColor, 0xffffffff);
    dc->RSSetState(rs_);
    dc->OMSetDepthStencilState(ds_, 0);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dc->ClearDepthStencilView(device_->GetMainDSV(), D3D11_CLEAR_DEPTH, 1, 0xFF); 
    //bind ib
    dc->IASetInputLayout(vertex_input_layout_);
    for (VertexStreamDescription& desc : vertex_descriptions_) {
        ID3D11Buffer* buffer = vertex_buffers_[desc.cpu_data_index];
        unsigned int stride = desc.stride;
        unsigned int offset = 0;
        if (desc.slot != -1) {
            dc->IASetVertexBuffers(desc.slot, 1, &buffer, &stride, &offset);
        }
    }
    //bind ib
    dc->IASetIndexBuffer(index_buffers_, DXGI_FORMAT_R16_UINT, 0);
    //dc->VSSetShader(program_ptr->d3d_vs->VertexShader, nullptr, 0);
    //program_ptr->d3d_ps->BindTextureSampler("samLinear", sampler_state_);
    //program_ptr->d3d_vs->Update();
    //program_ptr->d3d_ps->Update();
    //int index_count = mesh->GetIndexBuffer()->GetCount();
    //std::shared_ptr<SKwai::UnifiedBuffer> cpu_ib = mesh->GetIndexBuffer();
    //int triangle_count = cpu_ib->GetCount();
    //dc->DrawIndexed(triangle_count, 0, 0);
    //dc->OMSetRenderTargets(dc)
}
