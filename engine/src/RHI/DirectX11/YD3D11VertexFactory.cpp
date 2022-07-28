#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "RHI/DirectX11/D3D11Shader.h"
#include "RHI/DirectX11/D3D11Texture.h"
#include "Math/YVector.h"
#include "Engine/YRawMesh.h"
#include "RHI/DirectX11/D3D11Device.h"
#include "Engine/YLog.h"
#include <fstream>
IVertexFactory::IVertexFactory() :
	is_alloc_resource_(false),
	is_gpu_skin_(false),
	is_morph_(false),
	has_custom_data_(false) {}

IVertexFactory::~IVertexFactory() {
}


void IVertexFactory::SetupStreams()
{

}

const std::vector<VertexStreamDescription>& IVertexFactory::GetVertexDescription() const { return vertex_descriptions_; }

std::vector<VertexStreamDescription>& IVertexFactory::GetVertexDescription() { return vertex_descriptions_; }

bool IVertexFactory::IsGPUSKin() const { return is_gpu_skin_; }

bool IVertexFactory::IsMorph() const { return is_morph_; }


bool IVertexFactory::HasCustomData() const { return has_custom_data_; }

void IVertexFactory::SetUseGPUSKin(bool use_gpu_skin) { is_gpu_skin_ = use_gpu_skin; }

void IVertexFactory::SetUseMorph(bool has_mporh) { is_morph_ = has_mporh; }

DXVertexFactory::DXVertexFactory() {}

DXVertexFactory::~DXVertexFactory() {}



void DXVertexFactory::SetInputLayout(const TComPtr<ID3D11InputLayout>& input_layout) { vertex_input_layout_ = input_layout; }

VertexStreamDescription::VertexStreamDescription(VertexAttribute in_vertex_attribe, const std::string& in_name, DataType in_type, int in_cpu_data_index, int in_com_num, int in_buffer_size, int in_slot, int in_stride,int in_offset, bool in_normalized, bool in_release, bool in_dynamic) :
	vertex_attribute(in_vertex_attribe), name(in_name), data_type(in_type), cpu_data_index(in_cpu_data_index), com_num(in_com_num), buffer_size(in_buffer_size),
	slot(in_slot), stride(in_stride),offset(in_offset), normalized(in_normalized), release(in_release), dynamic(in_dynamic)
{

}

VertexStreamDescription::VertexStreamDescription()
{

}
