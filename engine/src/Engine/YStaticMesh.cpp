#include "Engine/YStaticMesh.h"
#include <cassert>
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "Engine/YLog.h"
#include <fstream>

class YStaticMeshVertexFactory :public DXVertexFactory
{
public:
	YStaticMeshVertexFactory(YStaticMesh* mesh)
		:mesh_(mesh)
	{

	}
	void SetupStreams()override
	{
		if (vertex_input_layout_)
		{
			const TComPtr<ID3D11DeviceContext> dc = g_device->GetDC();
			dc->IASetInputLayout(vertex_input_layout_);
			for (VertexStreamDescription& desc : vertex_descriptions_) {
				ID3D11Buffer* buffer = mesh_->vertex_buffers_[desc.cpu_data_index];
				unsigned int stride = desc.stride;
				unsigned int offset = 0;
				if (desc.slot != -1) {
					dc->IASetVertexBuffers(desc.slot, 1, &buffer, &stride, &offset);
				}
			}
		}
	}
	void SetupVertexDescriptionPolicy()
	{
		vertex_descriptions_.clear();
		VertexStreamDescription postion_desc(VertexAttribute::VA_POSITION, "position", DataType::Float32, 0, 3, 0, -1, sizeof(YVector), false, false, false);
		VertexStreamDescription normal_desc(VertexAttribute::VA_NORMAL, "normal", DataType::Float32, 1, 3, 0, -1, sizeof(YVector), false, false, false);
		VertexStreamDescription uv_desc(VertexAttribute::VA_UV0, "uv", DataType::Float32, 2, 2, 0, -1, sizeof(YVector2), false, false, false);
		vertex_descriptions_.push_back(postion_desc);
		vertex_descriptions_.push_back(normal_desc);
		vertex_descriptions_.push_back(uv_desc);
	}
protected:
	YStaticMesh* mesh_ = nullptr;
};

void YStaticMesh::Render(CameraBase* camera)
{
	ID3D11Device* device = g_device->GetDevice();
	ID3D11DeviceContext* dc = g_device->GetDC();
	float BlendColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	dc->OMSetBlendState(bs_, BlendColor, 0xffffffff);
	dc->RSSetState(rs_);
	dc->OMSetDepthStencilState(ds_, 0);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//bind im
	vertex_factory_->SetupStreams();
	//bind ib
	dc->IASetIndexBuffer(index_buffer_, DXGI_FORMAT_R32_UINT, 0);
	vertex_shader_->BindResource("g_projection", camera->GetProjectionMatrix());
	vertex_shader_->BindResource("g_view", camera->GetViewMatrix());
	vertex_shader_->Update();
	pixel_shader_->Update();

	int triangle_total = 0;
	for (auto& polygon_group : raw_meshes[0]->polygon_groups)
	{
		int triangle_count =(int) polygon_group.polygons.size();
		dc->DrawIndexed(triangle_count * 3, triangle_total, 0);
		triangle_total += triangle_count * 3;
	}
}
YStaticMesh::YStaticMesh()
{

}

bool YStaticMesh::AllocGpuResource()
{
	if (allocated_gpu_resource)
	{
		return true;
	}
	if (raw_meshes.size() == 0)
	{
		return false;
	}
	std::unique_ptr< YStaticMeshVertexFactory > static_mesh_vertex_factory = std::make_unique<YStaticMeshVertexFactory>(this);
	static_mesh_vertex_factory->SetupVertexDescriptionPolicy();
	std::unique_ptr<YLODMesh>& lod_mesh = raw_meshes[0];
	//vb
		// expand position buffer
	int polygon_group_index_offset = 0;
	polygon_group_offsets.clear();

	std::vector<YVector> position_buffer;
	std::vector<YVector> normal_buffer;
	std::vector<YVector2> uv_buffer;
	std::vector<int> index_buffer;
	int triagle_count = 0;
	for (YMeshPolygonGroup& polygon_group : lod_mesh->polygon_groups)
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
	for (YMeshPolygonGroup& polygon_group : lod_mesh->polygon_groups)
	{
		for (int polygon_index : polygon_group.polygons)
		{
			polygon_group_index_offset += 1;
			YMeshPolygon& polygon = lod_mesh->polygons[polygon_index];
			YMeshVertexInstance& vertex_ins_0 = lod_mesh->vertex_instances[polygon.vertex_instance_ids[0]];
			YMeshVertexInstance& vertex_ins_1 = lod_mesh->vertex_instances[polygon.vertex_instance_ids[1]];
			YMeshVertexInstance& vertex_ins_2 = lod_mesh->vertex_instances[polygon.vertex_instance_ids[2]];
			position_buffer.push_back(lod_mesh->vertex_position[vertex_ins_0.vertex_id].position);
			position_buffer.push_back(lod_mesh->vertex_position[vertex_ins_1.vertex_id].position);
			position_buffer.push_back(lod_mesh->vertex_position[vertex_ins_2.vertex_id].position);
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
		if (!g_device->CreateIndexBuffer((unsigned int)index_buffer.size() * sizeof(int), &index_buffer[0], index_buffer_)) {
			ERROR_INFO("Create index buffer failed!!");
			return false;
		}
	}

	// vs
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
		if (!vertex_shader_->CreateShaderFromSource(str, "VSMain", static_mesh_vertex_factory.get()))
		{
			return false;
		}
	}
	//ps
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
	if (!bs_) {
		g_device->CreateBlendState(bs_, true);
	}

	if (!rs_) {
		g_device->CreateRasterStateNonCull(rs_);
	}

	if (!ds_) {
		g_device->CreateDepthStencileState(ds_, true);
		//device_->CreateDepthStencileStateNoWriteNoTest(ds_);
	}
	if (!sampler_state_) {
		sampler_state_ = g_device->GetSamplerState(SF_MIN_MAG_MIP_LINEAR, AM_WRAP);
	}
	vertex_factory_ = std::move(static_mesh_vertex_factory);
	allocated_gpu_resource = true;
	return true;
}

void YStaticMesh::ReleaseGPUReosurce()
{
	vertex_buffers_.clear();
	index_buffer_ = nullptr;
	bs_ = nullptr;
	rs_ = nullptr;
	ds_ = nullptr;
	sampler_state_ = nullptr;
	allocated_gpu_resource = false;
}
