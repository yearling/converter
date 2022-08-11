#include "Engine/YStaticMesh.h"
#include <cassert>
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "Engine/YLog.h"
#include <fstream>
#include "Engine/YFile.h"
#include "Utility/YPath.h"
#include "SObject/SObject.h"
#include "Utility/YJsonHelper.h"
#include "Render/YRenderInterface.h"
#include "Engine/YRenderScene.h"
#include "Engine/YPrimitiveElement.h"
#include "Engine/YCanvas.h"

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
		VertexStreamDescription postion_desc(VertexAttribute::VA_POSITION, "position", DataType::Float32, 0, 3, 0, -1, sizeof(YVector),0, false, false, false);
		VertexStreamDescription normal_desc(VertexAttribute::VA_NORMAL, "normal", DataType::Float32, 1, 3, 0, -1, sizeof(YVector), 0,false, false, false);
		VertexStreamDescription tanget_desc(VertexAttribute::VA_NORMAL, "tangent", DataType::Float32, 2, 3, 0, -1, sizeof(YVector), 0,false, false, false);
		VertexStreamDescription uv_desc(VertexAttribute::VA_UV0, "uv", DataType::Float32, 3, 2, 0, -1, sizeof(YVector2), 0,false, false, false);
		vertex_descriptions_.push_back(postion_desc);
		vertex_descriptions_.push_back(normal_desc);
		vertex_descriptions_.push_back(tanget_desc);
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
	for (auto& polygon_group : raw_meshes[0].polygon_groups)
	{
		int triangle_count = (int)polygon_group.polygons.size();
		dc->DrawIndexed(triangle_count * 3, triangle_total, 0);
		triangle_total += triangle_count * 3;
	}
}



void YStaticMesh::Render(RenderParam* render_param)
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
	vertex_shader_->BindResource("g_projection", render_param->camera_proxy->projection_matrix_);
	vertex_shader_->BindResource("g_view", render_param->camera_proxy->view_matrix_);
	vertex_shader_->BindResource("g_world", render_param->local_to_world_);
	vertex_shader_->Update();
	if (render_param->dir_lights_proxy->size()) {
		YVector dir_light = -(*render_param->dir_lights_proxy)[0].light_dir;
		pixel_shader_->BindResource("light_dir", &dir_light.x, 3);
	}
    pixel_shader_->BindResource("show_normal", 1.0f);
	pixel_shader_->Update();

	int triangle_total = 0;
	for (auto& polygon_group : raw_meshes[0].polygon_groups)
	{
		int triangle_count = (int)polygon_group.polygons.size();
		dc->DrawIndexed(triangle_count * 3, triangle_total, 0);
		triangle_total += triangle_count * 3;
	}
    YLODMesh& lod_mesh = raw_meshes[0];
    const bool draw_hard_edge = true;
    if (draw_hard_edge)
    {
        for (YMeshPolygonGroup& polygon_group : lod_mesh.polygon_groups)
        {
            for (int polygon_index : polygon_group.polygons)
            {
                YMeshPolygon& polygon = lod_mesh.polygons[polygon_index];
                YMeshVertexWedge& vertex_ins_0 = lod_mesh.vertex_instances[polygon.wedge_ids[0]];
                YMeshVertexWedge& vertex_ins_1 = lod_mesh.vertex_instances[polygon.wedge_ids[1]];
                YMeshVertexWedge& vertex_ins_2 = lod_mesh.vertex_instances[polygon.wedge_ids[2]];
                int control_point[3] = { vertex_ins_0.control_point_id, vertex_ins_1.control_point_id, vertex_ins_2.control_point_id };
                int edge0_index = lod_mesh.GetVertexPairEdge(control_point[0], control_point[1]);
                int edge1_index = lod_mesh.GetVertexPairEdge(control_point[1], control_point[2]);
                int edge2_index = lod_mesh.GetVertexPairEdge(control_point[2], control_point[0]);

                bool edge0_hard = lod_mesh.edges[edge0_index].edge_hardness;
                bool edge1_hard = lod_mesh.edges[edge1_index].edge_hardness;
                bool edge2_hard = lod_mesh.edges[edge2_index].edge_hardness;

                YVector control_point0 = lod_mesh.vertex_position[control_point[0]].position;
                YVector control_point1 = lod_mesh.vertex_position[control_point[1]].position;
                YVector control_point2 = lod_mesh.vertex_position[control_point[2]].position;
                if (edge0_hard)
                {
                    g_Canvas->DrawLine(control_point0, control_point1, edge0_hard ? YVector4(0.0, 0.0, 0.0, 1.0) : YVector4(0.0, 0.0, 1.0, 1.0));
                }
                if (edge1_hard)
                {
                    g_Canvas->DrawLine(control_point1, control_point2, edge1_hard ? YVector4(0.0, 0.0, 0.0, 1.0) : YVector4(0.0, 0.0, 1.0, 1.0));
                }

                if (edge2_hard)
                {
                    g_Canvas->DrawLine(control_point2, control_point0, edge2_hard ? YVector4(0.0, 0.0, 0.0, 1.0) : YVector4(0.0, 0.0, 1.0, 1.0));
                }
            }
        }
    }
    const bool draw_normal_tangent_bitangent = false;
    if (draw_normal_tangent_bitangent)
    {
        for (YMeshPolygonGroup& polygon_group : lod_mesh.polygon_groups)
        {
            for (int polygon_index : polygon_group.polygons)
            {
                YMeshPolygon& polygon = lod_mesh.polygons[polygon_index];
                for (int i = 0; i < 3; ++i)
                {
                    YMeshVertexWedge& vertex_ins_0 = lod_mesh.vertex_instances[polygon.wedge_ids[i]];
                    YVector control_point0 = lod_mesh.vertex_position[vertex_ins_0.control_point_id].position;
                    YVector normal = vertex_ins_0.normal.GetSafeNormal();
                    YVector tangent = vertex_ins_0.tangent.GetSafeNormal();
                    YVector bitangent = vertex_ins_0.bitangent.GetSafeNormal();
                    YMatrix trans_tangent_to_local =
                        YMatrix(
                            YVector4(tangent, 0.0),
                            YVector4(bitangent, 0.0),
                            YVector4(normal, 0.0),
                            YVector4(control_point0, 1.0)
                        );
                    float scale = 0.3;
                    YVector normal_end_pose = YVector(0.0, 0.0, 1.0) * scale;
                    normal_end_pose = trans_tangent_to_local.TransformPosition(normal_end_pose);
                    g_Canvas->DrawLine(control_point0, normal_end_pose, YVector4(0.0, 0.0, 1.0, 1.0));
                    YVector tangent_end_pose = YVector(1.0, 0.0, 0.0) * scale;
                    tangent_end_pose = trans_tangent_to_local.TransformPosition(tangent_end_pose);
                    g_Canvas->DrawLine(control_point0, tangent_end_pose, YVector4(1.0, 0.0, 0.0, 1.0));
                    YVector binormal_end_pose = YVector(0.0, 1.0, 0.0) * scale;
                    binormal_end_pose = trans_tangent_to_local.TransformPosition(binormal_end_pose);
                    g_Canvas->DrawLine(control_point0, binormal_end_pose, YVector4(0.0, 1.0, 0.0, 1.0));
                }
              
            }
        }
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
	YLODMesh& lod_mesh = raw_meshes[0];
	//vb
		// expand position buffer
	int polygon_group_index_offset = 0;
	polygon_group_offsets.clear();

	std::vector<YVector> position_buffer;
	std::vector<YVector> normal_buffer;
	std::vector<YVector> tangent_buffer;
	std::vector<YVector2> uv_buffer;
	std::vector<int> index_buffer;
	int triagle_count = 0;
	for (YMeshPolygonGroup& polygon_group : lod_mesh.polygon_groups)
	{
		for (int polygon_index : polygon_group.polygons)
		{
			triagle_count += 1;
		}
	}

	position_buffer.reserve(triagle_count * 3);
	normal_buffer.reserve(triagle_count * 3);
    tangent_buffer.reserve(triagle_count * 3);
	uv_buffer.reserve(triagle_count * 2);
	index_buffer.reserve(triagle_count * 3);
	int index_value = 0;
	for (YMeshPolygonGroup& polygon_group : lod_mesh.polygon_groups)
	{
		for (int polygon_index : polygon_group.polygons)
		{
			polygon_group_index_offset += 1;
			YMeshPolygon& polygon = lod_mesh.polygons[polygon_index];
			YMeshVertexWedge& vertex_ins_0 = lod_mesh.vertex_instances[polygon.wedge_ids[0]];
			YMeshVertexWedge& vertex_ins_1 = lod_mesh.vertex_instances[polygon.wedge_ids[1]];
			YMeshVertexWedge& vertex_ins_2 = lod_mesh.vertex_instances[polygon.wedge_ids[2]];
			position_buffer.push_back(lod_mesh.vertex_position[vertex_ins_0.control_point_id].position);
			position_buffer.push_back(lod_mesh.vertex_position[vertex_ins_1.control_point_id].position);
			position_buffer.push_back(lod_mesh.vertex_position[vertex_ins_2.control_point_id].position);
			normal_buffer.push_back(vertex_ins_0.normal);
			normal_buffer.push_back(vertex_ins_1.normal);
			normal_buffer.push_back(vertex_ins_2.normal);
            tangent_buffer.push_back(vertex_ins_0.tangent);
            tangent_buffer.push_back(vertex_ins_1.tangent);
            tangent_buffer.push_back(vertex_ins_2.tangent);
			uv_buffer.push_back(vertex_ins_0.uvs[0]);
			uv_buffer.push_back(vertex_ins_1.uvs[0]);
			uv_buffer.push_back(vertex_ins_2.uvs[0]);
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
        if (!g_device->CreateVertexBufferStatic((unsigned int)tangent_buffer.size() * sizeof(YVector), &tangent_buffer[0], d3d_vb)) {
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
		YFile vertex_shader_source(shader_path, YFile::FileType(YFile::FileType::FT_Read | YFile::FileType::FT_TXT));
		std::unique_ptr<MemoryFile> mem_file = vertex_shader_source.ReadFile();
		if (!mem_file)
		{
			ERROR_INFO("open shader ", shader_path, " failed!");
			return false;
		}
		std::string str(mem_file->GetReadOnlyFileContent().begin(), mem_file->GetReadOnlyFileContent().end());
		if (!vertex_shader_->CreateShaderFromSource(str, "VSMain", static_mesh_vertex_factory.get()))
		{
			return false;
		}
	}
	//ps
	{
		pixel_shader_ = std::make_unique<D3DPixelShader>();
		const std::string shader_path = "Shader/StaticMesh.hlsl";

		YFile vertex_shader_source(shader_path, YFile::FileType(YFile::FileType::FT_Read | YFile::FileType::FT_TXT));
		std::unique_ptr<MemoryFile> mem_file = vertex_shader_source.ReadFile();
		if (!mem_file)
		{
			ERROR_INFO("open shader ", shader_path, " failed!");
			return false;
		}
		std::string str(mem_file->GetReadOnlyFileContent().begin(), mem_file->GetReadOnlyFileContent().end());
		if (!pixel_shader_->CreateShaderFromSource(str, "PSMain"))
		{
			return false;
		}
	}
	if (!bs_) {
		g_device->CreateBlendState(bs_, true);
	}

	if (!rs_) {
		g_device->CreateRasterState(rs_);
	}

	if (!ds_) {
		g_device->CreateDepthStencileState(ds_, true);
		//device_->CreateDepthStencileStateNoWriteNoTest(ds_);
	}
	if (!sampler_state_) {
		sampler_state_ = g_device->GetSamplerState(SF_BiLinear, SA_Wrap);
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


bool YStaticMesh::SaveV0(const std::string& dir)
{
	MemoryFile mem_file(MemoryFile::FT_Write);
	int version = 0;
	mem_file << version;
	mem_file << raw_meshes;


	std::string file_name = YPath::PathCombine(dir, model_name + ".yasset");
	YFile file_to_write(file_name, YFile::FileType(YFile::FileType::FT_Write | YFile::FileType::FT_BINARY));
	if (!file_to_write.WriteFile(&mem_file, true))
	{
		ERROR_INFO("static mesh ", model_name, " save failed!");
		return false;
	}
	return true;
}

bool YStaticMesh::LoadV0(const std::string& file_path)
{
	// read model.json
	model_name = file_path;
	std::string parent_dir_path = YPath::GetPath(file_path);
	std::string static_mesh_asset_path = file_path + SObject::asset_extension_with_dot;
	YFile file_to_read(static_mesh_asset_path, YFile::FileType(YFile::FileType::FT_Read | YFile::FileType::FT_BINARY));
	std::unique_ptr<MemoryFile> mem_file = file_to_read.ReadFile();
	if (mem_file)
	{
		int version = 0;
		(*mem_file) << version;
		(*mem_file) << raw_meshes;

		for (auto& mesh : raw_meshes)
		{
			mesh.ComputeAABB();
		}
		return true;
	}
	else
	{
		ERROR_INFO("static mesh load ", static_mesh_asset_path, " failed!");
		return false;
	}
}
