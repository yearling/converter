#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "RHI/DirectX11/D3D11Shader.h"
#include "RHI/DirectX11/D3D11Texture.h"
#include "Math/YVector.h"
#include "Engine/YRawMesh.h"
#include "RHI/DirectX11/D3D11Device.h"
#include "Engine/YLog.h"
#include <fstream>
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

bool IVertexFactory::ReleaseGPUResource() { return true; }

const std::vector<VertexStreamDescription>& IVertexFactory::GetVertexDescription() const { return vertex_descriptions_; }

std::vector<VertexStreamDescription>& IVertexFactory::GetVertexDescription() { return vertex_descriptions_; }

bool IVertexFactory::IsGPUSKin() const { return is_gpu_skin_; }

bool IVertexFactory::IsMorph() const { return is_morph_; }


bool IVertexFactory::HasCustomData() const { return has_custom_data_; }

void IVertexFactory::SetUseGPUSKin(bool use_gpu_skin)  { is_gpu_skin_ = use_gpu_skin; }

void IVertexFactory::SetUseMorph(bool has_mporh) { is_morph_ = has_mporh; }

DXVertexFactory::DXVertexFactory(){}

 DXVertexFactory::~DXVertexFactory() {}

bool DXVertexFactory::AllocGPUResource(YLODMesh* mesh)
{
    if (is_alloc_resource_)
    {
        return true;
    }
	// expand position buffer
	int polygon_group_index_offset = 0;
    polygon_group_offsets.clear();

	std::vector<YVector> position_buffer;
	std::vector<YVector> normal_buffer;
	std::vector<YVector2> uv_buffer;
    std::vector<int> index_buffer;
	int triagle_count = 0;
	for (YMeshPolygonGroup& polygon_group : mesh->polygon_groups)
	{
		for (int polygon_index : polygon_group.polygons)
		{
			triagle_count += 1;
		}
	}

	position_buffer.reserve(triagle_count * 3);
	normal_buffer.reserve(triagle_count * 3);
	uv_buffer.reserve(triagle_count * 2);
    index_buffer.reserve(triagle_count * 3);
    int index_value = 0;
	for (YMeshPolygonGroup& polygon_group : mesh->polygon_groups)
	{
		for (int polygon_index : polygon_group.polygons)
		{
			polygon_group_index_offset += 1;
			YMeshPolygon& polygon = mesh->polygons[polygon_index];
			YMeshVertexInstance& vertex_ins_0 = mesh->vertex_instances[polygon.vertex_instance_ids[0]];
			YMeshVertexInstance& vertex_ins_1 = mesh->vertex_instances[polygon.vertex_instance_ids[1]];
			YMeshVertexInstance& vertex_ins_2 = mesh->vertex_instances[polygon.vertex_instance_ids[2]];
			position_buffer.push_back(mesh->vertex_position[vertex_ins_0.vertex_id].position);
			position_buffer.push_back(mesh->vertex_position[vertex_ins_1.vertex_id].position);
			position_buffer.push_back(mesh->vertex_position[vertex_ins_2.vertex_id].position);
			normal_buffer.push_back(vertex_ins_0.vertex_instance_normal);
			normal_buffer.push_back(vertex_ins_1.vertex_instance_normal);
			normal_buffer.push_back(vertex_ins_2.vertex_instance_normal);
			uv_buffer.push_back(vertex_ins_0.vertex_instance_uvs[0]);
			uv_buffer.push_back(vertex_ins_1.vertex_instance_uvs[0]);
			uv_buffer.push_back(vertex_ins_2.vertex_instance_uvs[0]);
            index_buffer.push_back(index_value++);
            index_buffer.push_back(index_value++);
            index_buffer.push_back(index_value++);
		}
		polygon_group_offsets.push_back(polygon_group_index_offset);
	}
	assert(position_buffer.size() == triagle_count * 3);
	assert(normal_buffer.size() == triagle_count * 3);
	assert(uv_buffer.size() == triagle_count * 3);

    {
        TComPtr<ID3D11Buffer> d3d_vb;
        if (!g_device->CreateVertexBufferStatic((unsigned int)position_buffer.size() * sizeof(YVector), &position_buffer[0], d3d_vb)) {
            ERROR_INFO("Create vertex buffer failed!!");
            return false;
        }
        vertex_buffers_.push_back(d3d_vb);
    }

	{
		TComPtr<ID3D11Buffer> d3d_vb;
		if (!g_device->CreateVertexBufferStatic((unsigned int)normal_buffer.size() * sizeof(YVector), &normal_buffer[0], d3d_vb)) {
			ERROR_INFO("Create vertex buffer failed!!");
			return false;
		}
		vertex_buffers_.push_back(d3d_vb);
	}

	{
		TComPtr<ID3D11Buffer> d3d_vb;
		if (!g_device->CreateVertexBufferStatic((unsigned int)uv_buffer.size() * sizeof(YVector2), &uv_buffer[0], d3d_vb)) {
			ERROR_INFO("Create vertex buffer failed!!");
			return false;
		}
		vertex_buffers_.push_back(d3d_vb);
	}

    {
		if (!g_device->CreateIndexBuffer((unsigned int)index_buffer.size()*sizeof(int), &index_buffer[0], index_buffer_)) {
			ERROR_INFO("Create index buffer failed!!");
			return false;
		}
    }
    
    //desc
    std::vector<VertexStreamDescription> tmp_desc;
    VertexStreamDescription postion_desc(VertexAttribute::VA_POSITION,"position",DataType::Float32,0,3,0,-1,sizeof(YVector),false,false,false);
    VertexStreamDescription normal_desc(VertexAttribute::VA_NORMAL,"normal",DataType::Float32,1,3,0,-1,sizeof(YVector),false,false,false);
    VertexStreamDescription uv_desc(VertexAttribute::VA_UV0,"uv",DataType::Float32,2,2,0,-1,sizeof(YVector2),false,false,false);
    tmp_desc.push_back(postion_desc);
    tmp_desc.push_back(normal_desc);
    tmp_desc.push_back(uv_desc);
    SetVertexStreamDescriptions(std::move(tmp_desc));


    // vb
    {
        vertex_shader_ = std::make_unique<D3DVertexShader>();
        const std::string shader_path = "Shader/StaticMesh.hlsl";
        std::ifstream inFile;
        inFile.open(shader_path); //open the input file
        if (inFile.bad())
        {
            ERROR_INFO("open shader ", shader_path, " failed!");
            return false;
        }
        std::stringstream strStream;
        strStream << inFile.rdbuf(); //read the file
        std::string str = strStream.str();
        if (!vertex_shader_->CreateShaderFromSource(str, "VSMain", this))
        {
            return false;
        }
    }

    {
        pixel_shader_ = std::make_unique<D3DPixelShader>();
		const std::string shader_path = "Shader/StaticMesh.hlsl";
		std::ifstream inFile;
		inFile.open(shader_path); //open the input file
		if (inFile.bad())
		{
			ERROR_INFO("open shader ", shader_path, " failed!");
			return false;
		}
		std::stringstream strStream;
		strStream << inFile.rdbuf(); //read the file
		std::string str = strStream.str();
		if (!pixel_shader_->CreateShaderFromSource(str, "PSMain"))
		{
			return false;
		}
    }
    SetRenderState();
	is_alloc_resource_ = true;
    
    return true;
}

bool DXVertexFactory::ReleaseGPUResource() {
     vertex_buffers_.clear();
    index_buffer_ = nullptr;
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
        g_device->CreateBlendState(bs_, true);
    }

    if (!rs_) {
        g_device->CreateRasterStateNonCull(rs_);
    }

    if (!ds_) {
        g_device->CreateDepthStencileState(ds_,true);
        //device_->CreateDepthStencileStateNoWriteNoTest(ds_);
    }
    if (!sampler_state_) {
        sampler_state_ = g_device->GetSamplerState(SF_MIN_MAG_MIP_LINEAR,AM_WRAP);
    }
}

void DXVertexFactory::DrawCall(CameraBase* camera) {
    ID3D11Device* device = g_device->GetDevice();
    ID3D11DeviceContext* dc = g_device->GetDC();
    float BlendColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    dc->OMSetBlendState(bs_, BlendColor, 0xffffffff);
    dc->RSSetState(rs_);
    dc->OMSetDepthStencilState(ds_, 0);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dc->ClearDepthStencilView(g_device->GetMainDSV(), D3D11_CLEAR_DEPTH, 1, 0xFF);
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
    dc->IASetIndexBuffer(index_buffer_, DXGI_FORMAT_R32_UINT, 0);
    vertex_shader_->BindResource("g_projection", camera->GetProjectionMatrix());
    vertex_shader_->BindResource("g_view", camera->GetViewMatrix());
    vertex_shader_->Update();
    pixel_shader_->Update();
    
    int triangle_total = 0;
    for (int triangle_count : polygon_group_offsets)
    {
        dc->DrawIndexed(triangle_count*3, triangle_total, 0);
        triangle_total += triangle_count * 3;
    }
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

VertexStreamDescription::VertexStreamDescription(VertexAttribute in_vertex_attribe, const std::string& in_name, DataType in_type, int in_cpu_data_index, int in_com_num, int in_buffer_size, int in_slot, int in_stride, bool in_normalized, bool in_release, bool in_dynamic):
vertex_attribute(in_vertex_attribe), name(in_name), data_type(in_type), cpu_data_index(in_cpu_data_index),com_num(in_com_num),buffer_size(in_buffer_size),
slot(in_slot),stride(in_stride),normalized(in_normalized),release(in_release),dynamic(in_dynamic)
{

}

VertexStreamDescription::VertexStreamDescription()
{

}
