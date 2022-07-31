#include "Engine/YSkeletonMesh.h"
#include "Engine/YCanvasUtility.h"
#include "Engine/YCanvas.h"

#include "Render/YRenderInterface.h"
#include "Engine/YRenderScene.h"
#include "Engine/YPrimitiveElement.h"
#include "Math/YTransform.h"
class YSkeltonMeshVertexFactory :public DXVertexFactory
{
public:
    YSkeltonMeshVertexFactory(YSkeletonMesh* mesh)
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
                unsigned int offset = desc.offset;
                if (desc.slot != -1) {
                    dc->IASetVertexBuffers(desc.slot, 1, &buffer, &stride, &offset);
                }
            }
        }
    }
    void SetupVertexDescriptionPolicy()
    {
        vertex_descriptions_.clear();
        VertexStreamDescription postion_desc(VertexAttribute::VA_POSITION, "position", DataType::Float32, 0, 3, 0, -1, sizeof(YVector), 0, false, false, true);
        VertexStreamDescription normal_desc(VertexAttribute::VA_NORMAL, "normal", DataType::Float32, 1, 3, 0, -1, sizeof(YVector), 0, false, false, true);
        VertexStreamDescription uv_desc(VertexAttribute::VA_UV0, "uv", DataType::Float32, 2, 2, 0, -1, sizeof(YVector2), false, 0, false, true);
        VertexStreamDescription color_desc(VertexAttribute::VA_COLOR, "color", DataType::Float32, 3, 4, 0, -1, sizeof(YVector4), 0, false, false, true);
        vertex_descriptions_.push_back(postion_desc);
        vertex_descriptions_.push_back(normal_desc);
        vertex_descriptions_.push_back(uv_desc);
        vertex_descriptions_.push_back(color_desc);
    }
protected:
    YSkeletonMesh* mesh_ = nullptr;
};


class YGPUSkeltonMeshVertexFactory :public DXVertexFactory
{
public:
    YGPUSkeltonMeshVertexFactory(YSkeletonMesh* mesh)
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
        VertexStreamDescription postion_desc(VertexAttribute::VA_POSITION, "position", DataType::Float32, 0, 3, 0, -1, sizeof(YVector), 0, false, false, true);
        VertexStreamDescription normal_desc(VertexAttribute::VA_NORMAL, "normal", DataType::Float32, 1, 3, 0, -1, sizeof(YVector), 0, false, false, true);
        VertexStreamDescription uv_desc(VertexAttribute::VA_UV0, "uv", DataType::Float32, 2, 2, 0, -1, sizeof(YVector2), 0, false, false, true);
        VertexStreamDescription color_desc(VertexAttribute::VA_COLOR, "color", DataType::Float32, 3, 4, 0, -1, sizeof(YVector4), 0, false, false, true);
        VertexStreamDescription bone_weight_desc(VertexAttribute::VA_BONEWEIGHT, "weight", DataType::Float32, 4, 4, 0, -1, sizeof(YVector4) * 2, 0, false, false, true);
        VertexStreamDescription bone_weight_desc2(VertexAttribute::VA_BONEWEIGHTEXTRA, "weight_extra", DataType::Float32, 4, 4, 0, -1, sizeof(YVector4) * 2, sizeof(YVector4), false, false, true);
        VertexStreamDescription bone_id_desc(VertexAttribute::VA_BONEID, "boneid", DataType::Uint32, 5, 4, 0, -1, sizeof(YVector4) * 2, 0, false, false, true);
        VertexStreamDescription bone_id_desc2(VertexAttribute::VA_BONEIDEXTRA, "boneid_extra", DataType::Uint32, 5, 4, 0, -1, sizeof(YVector4) * 2, sizeof(YVector4), false, false, true);
        VertexStreamDescription morph_position_offset(VertexAttribute::VA_MORPH_POSITION_OFFSET, "morph_position_offset", DataType::Float32, 6, 3, 0, -1, sizeof(YVector), 0, false, false, true);
        vertex_descriptions_.push_back(postion_desc);
        vertex_descriptions_.push_back(normal_desc);
        vertex_descriptions_.push_back(uv_desc);
        vertex_descriptions_.push_back(color_desc);
        vertex_descriptions_.push_back(bone_weight_desc);
        vertex_descriptions_.push_back(bone_weight_desc2);
        vertex_descriptions_.push_back(bone_id_desc);
        vertex_descriptions_.push_back(bone_id_desc2);
        vertex_descriptions_.push_back(morph_position_offset);
    }
    static int GetGPUMatrixCount() { return gpu_matrix_count; }
protected:
    YSkeletonMesh* mesh_ = nullptr;
    static const int gpu_matrix_count = 64;
};

YBone::YBone()
{
    bind_local_tranform_ = YTransform::identity;
    bind_local_matrix_ = bind_local_tranform_.ToMatrix();
    bone_id_ = -1;
    parent_id_ = -1;
}

YSkeleton::YSkeleton()
{
    root_bone_id_ = -1;
}

void YSkeleton::Update(double delta_time)
{
    if (root_bone_id_ != -1)
    {
        UpdateRecursive(delta_time, root_bone_id_);
    }
}

void YSkeleton::UpdateRecursive(double delta_time, int bone_id)
{
    YBone& cur_bone = bones_[bone_id];

    if (cur_bone.parent_id_ != -1)
    {
        int parent_bone_id = cur_bone.parent_id_;
        YBone& parent_bone = bones_[parent_bone_id];
        cur_bone.global_transform_ = cur_bone.local_transform_ * parent_bone.global_transform_;
        cur_bone.global_matrix_ = cur_bone.local_matrix_ * parent_bone.global_matrix_;
        cur_bone.inv_bind_global_matrix_mul_global_matrix = cur_bone.inv_bind_global_matrix_ * cur_bone.global_matrix_;
    }
    else
    {
        cur_bone.global_transform_ = cur_bone.local_transform_;
        cur_bone.global_matrix_ = cur_bone.local_matrix_;
        cur_bone.inv_bind_global_matrix_mul_global_matrix = cur_bone.inv_bind_global_matrix_ * cur_bone.global_matrix_;
    }

    for (int child_bone_id : cur_bone.children_)
    {
        UpdateRecursive(delta_time, child_bone_id);
    }
}

YSkeletonMesh::YSkeletonMesh()
{
    play_time = 0.0;
}

void YSkeletonMesh::Update(double delta_time)
{
    play_time += delta_time;
    if (animation_data_)
    {
        if (play_time > animation_data_->time_)
        {
            play_time = 0.0;
        }
        int current_frame = int(play_time * 30.0f);
        if (skeleton_)
        {
            for (YBone& bone : skeleton_->bones_)
            {
                AnimationSequenceTrack& track = animation_data_->sequence_track[bone.bone_name_];
                int key_size = track.pos_keys_.size();
                current_frame = current_frame % key_size;
                YVector pos = track.pos_keys_[current_frame];
                YQuat rot = track.rot_keys_[current_frame];
                YVector scale = track.scale_keys_[current_frame];
                YTransform releative_animation_transform_ = YTransform(pos, rot, scale);
                bone.local_transform_ = releative_animation_transform_ * bone.bind_local_tranform_;
                bone.local_matrix_ = bone.local_transform_.ToMatrix();
            }

            skeleton_->Update(delta_time);
            for (int i = 0; i < skeleton_->bones_.size(); ++i) {
                YBone& bone = skeleton_->bones_[i];
                YVector joint_center = bone.global_transform_.TransformPosition(YVector(0, 0, 0));
                g_Canvas->DrawCube(joint_center, YVector4(1.0f, 0.0f, 0.0f, 1.0f), 0.1f);
                if (bone.parent_id_ != -1) {
                    YVector parent_joint_center = skeleton_->bones_[bone.parent_id_].global_transform_.TransformPosition(YVector(0, 0, 0));
                    g_Canvas->DrawLine(parent_joint_center, joint_center, YVector4(0.0f, 1.0f, 0.0f, 1.0f));
                }
            }
        }
        {
            if (render_data_->morph_render_data.size() && animation_data_ && animation_data_->bs_sequence_track.size())
            {
                //cached_morph_offset.resize()
                std::fill(cached_morph_offset.begin(), cached_morph_offset.end(), YVector::zero_vector);
                if (animation_data_->bs_sequence_track.count(0))
                {
                    const BlendShapeSequneceTrack& bs_sequence_track = animation_data_->bs_sequence_track.at(0);

                    for (auto& key_value : bs_sequence_track.value_curve_)
                    {
                        const std::string channel_name = key_value.first;
                        const std::vector<float>& curve = bs_sequence_track.value_curve_.at(channel_name);
                        int key_size = curve.size();
                        int current_key = current_frame % key_size;
                        float weight = curve[current_key] / 100.0f;
                        if (!YMath::IsNearlyZero(weight))
                        {
                            if (render_data_->morph_render_data_compress.count(channel_name))
                            {
                                CompressMorphWedge compress_morph_wedge = render_data_->morph_render_data_compress.at(channel_name);
                                for (int compress_index = 0; compress_index < compress_morph_wedge.map_index.size(); ++compress_index)
                                {
                                    int orignal_vertex_index = compress_morph_wedge.map_index[compress_index];
                                    YVector pos_diff = compress_morph_wedge.position[compress_index];
                                    cached_morph_offset[orignal_vertex_index] = cached_morph_offset[orignal_vertex_index] + pos_diff * weight;

                                }
                            }
                        }
                    }
                }
            }
        }
        return;
        int mesh_index = 0;
        for (SkinMesh& mesh : skin_data_->meshes_)
        {
            bool has_bs = !mesh.bs_.target_shapes_.empty();

            if (has_bs)
            {
                mesh.bs_.cached_control_point = mesh.control_points_;
                if (animation_data_->bs_sequence_track.count(mesh_index))
                {
                    const BlendShapeSequneceTrack& bs_sequence_track = animation_data_->bs_sequence_track.at(mesh_index);
                    mesh.bs_.cached_control_point = mesh.control_points_;

                    for (auto& key_value : mesh.bs_.target_shapes_)
                    {
                        const std::vector<float>& curve = bs_sequence_track.value_curve_.at(key_value.first);
                        int key_size = curve.size();
                        int current_key = current_frame % key_size;
                        float weight = curve[current_key] / 100.0f;

                        for (int contorl_point_index = 0; contorl_point_index < mesh.control_points_.size(); contorl_point_index++)
                        {
                            YVector& des_pos = mesh.bs_.cached_control_point[contorl_point_index];

                            YVector dif_pos = weight * (key_value.second.control_points[contorl_point_index] - mesh.control_points_[contorl_point_index]);
                            des_pos = des_pos + dif_pos;
                        }
                    }
                }
            }
            mesh_index++;
        }
    }
    else
    {

        for (SkinMesh& mesh : skin_data_->meshes_)
        {
            bool has_bs = !mesh.bs_.target_shapes_.empty();

            if (has_bs)
            {
                mesh.bs_.cached_control_point = mesh.control_points_;
            }
        }
    }


}

void YSkeletonMesh::Render()
{

}

void YSkeletonMesh::Render(RenderParam* render_param)
{
    if (!allocated_gpu_resource)
    {
        return;
    }
    ID3D11Device* device = g_device->GetDevice();
    ID3D11DeviceContext* dc = g_device->GetDC();
    float BlendColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    dc->OMSetBlendState(blend_state_, BlendColor, 0xffffffff);
    dc->RSSetState(rs_);
    dc->OMSetDepthStencilState(ds_, 0);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //bind im
    vertex_factory_->SetupStreams();
    //bind ib
    dc->IASetIndexBuffer(index_buffer_, DXGI_FORMAT_R32_UINT, 0);
    vertex_shader_->BindResource("g_projection", render_param->camera_proxy->projection_matrix_);
    vertex_shader_->BindResource("g_view", render_param->camera_proxy->view_matrix_);
    YMatrix local_to_world = render_param->local_to_world_;
    vertex_shader_->BindResource("g_world", local_to_world);
    vertex_shader_->Update();
    if (render_param->dir_lights_proxy->size()) {
        YVector dir_light = -(*render_param->dir_lights_proxy)[0].light_dir;
        pixel_shader_->BindResource("light_dir", &dir_light.x, 3);
    }
    pixel_shader_->Update();

    //update morph
    if (render_data_->morph_render_data.size() && animation_data_ && animation_data_->bs_sequence_track.size())
    {
        assert(cached_morph_offset.size() == render_data_->position.size());
        {
            D3D11_MAPPED_SUBRESOURCE MappedSubresource;
            if (dc->Map(morph_position_offset_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource) != S_OK)
            {
                ERROR_INFO("update skin data error");
            }
            memcpy(MappedSubresource.pData, cached_morph_offset.data(), cached_morph_offset.size() * sizeof(YVector));
            dc->Unmap(morph_position_offset_buffer, 0);
        }
    }
    std::vector<YVector>& position_buffer = render_data_->position;
    std::vector<YVector>& normal_buffer = render_data_->normal;
    std::vector<YVector4> blend_matrix_current_section;
    blend_matrix_current_section.resize(YGPUSkeltonMeshVertexFactory::GetGPUMatrixCount() * 3);
    for (int section_index = 0; section_index < render_data_->sections.size(); ++section_index)
    {
        int section_begin = render_data_->sections[section_index];
        int tringle_count = render_data_->triangle_counts[section_index];
        const std::vector<int> bone_mapping = render_data_->bone_mapping[section_index];
        for (int i = 0; i < bone_mapping.size(); ++i)
        {
            int bone_real_id = bone_mapping[i];
            YBone& bone = skeleton_->bones_[bone_real_id];
            YMatrix& bone_matrix = bone.inv_bind_global_matrix_mul_global_matrix;
            blend_matrix_current_section[i * 3 + 0] = YVector4(bone_matrix.m[0][0], bone_matrix.m[1][0], bone_matrix.m[2][0], bone_matrix.m[3][0]);
            blend_matrix_current_section[i * 3 + 1] = YVector4(bone_matrix.m[0][1], bone_matrix.m[1][1], bone_matrix.m[2][1], bone_matrix.m[3][1]);
            blend_matrix_current_section[i * 3 + 2] = YVector4(bone_matrix.m[0][2], bone_matrix.m[1][2], bone_matrix.m[2][2], bone_matrix.m[3][2]);
        }

        {
            D3D11_MAPPED_SUBRESOURCE MappedSubresource;
            if (dc->Map(bone_matrix_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource) != S_OK)
            {
                ERROR_INFO("update skin data error");
            }
            memcpy(MappedSubresource.pData, blend_matrix_current_section.data(), blend_matrix_current_section.size() * sizeof(YVector4));
            dc->Unmap(bone_matrix_buffer_, 0);
        }

        vertex_shader_->BindSRV("BoneMatrices", bone_matrix_buffer_srv_);
        vertex_shader_->Update();
        dc->DrawIndexed(tringle_count * 3, section_begin, 0);
    }
}

bool YSkeletonMesh::AllocGpuResource()
{
    if (allocated_gpu_resource)
    {
        return true;
    }

    if (skin_data_->meshes_.size() == 0)
    {
        return false;
    }

    //std::unique_ptr< YSkeltonMeshVertexFactory > static_mesh_vertex_factory = std::make_unique<YSkeltonMeshVertexFactory>(this);
    //static_mesh_vertex_factory->SetupVertexDescriptionPolicy();
    std::unique_ptr<YGPUSkeltonMeshVertexFactory> gpu_vertex_factory = std::make_unique<YGPUSkeltonMeshVertexFactory>(this);
    gpu_vertex_factory->SetupVertexDescriptionPolicy();

    polygon_group_offsets.clear();

    std::vector<YVector>& position_buffer = render_data_->position;
    std::vector<YVector>& normal_buffer = render_data_->normal;
    std::vector<YVector4>& color_buffer = render_data_->color;
    std::vector<YVector2> uv_buffer;
    std::vector<int> index_buffer = render_data_->indices;
    std::vector<int> bone_id_buffer;
    std::vector<float> bone_weight_buffer;

    if (render_data_->morph_render_data.size())
    {
        cached_morph_offset.resize(position_buffer.size(), YVector::zero_vector);
    }

    {
        uv_buffer.reserve(render_data_->uv.size());
        for (std::array<YVector2, 2>&uvs : render_data_->uv)
        {
            uv_buffer.push_back(uvs[0]);
        }
    }

    for (std::array<float, 8>&weight_per_vertex : render_data_->weights)
    {
        bone_weight_buffer.insert(bone_weight_buffer.end(), weight_per_vertex.begin(), weight_per_vertex.end());
    }

    for (std::array<int, 8>&bone_id_vertex : render_data_->bone_id)
    {
        bone_id_buffer.insert(bone_id_buffer.end(), bone_id_vertex.begin(), bone_id_vertex.end());
    }

    {
        TComPtr<ID3D11Buffer> d3d_vb;
        if (!g_device->CreateVertexBufferDynamic((unsigned int)position_buffer.size() * sizeof(YVector), position_buffer.data(), d3d_vb)) {
            ERROR_INFO("Create vertex buffer failed!!");
            return false;
        }
        vertex_buffers_.push_back(d3d_vb);
    }

    {
        TComPtr<ID3D11Buffer> d3d_vb;
        if (!g_device->CreateVertexBufferStatic((unsigned int)normal_buffer.size() * sizeof(YVector), normal_buffer.data(), d3d_vb)) {
            ERROR_INFO("Create vertex buffer failed!!");
            return false;
        }
        vertex_buffers_.push_back(d3d_vb);
    }

    {
        TComPtr<ID3D11Buffer> d3d_vb;
        if (!g_device->CreateVertexBufferStatic((unsigned int)uv_buffer.size() * sizeof(YVector2), uv_buffer.data(), d3d_vb)) {
            ERROR_INFO("Create vertex buffer failed!!");
            return false;
        }
        vertex_buffers_.push_back(d3d_vb);
    }

    {
        TComPtr<ID3D11Buffer> d3d_vb;
        if (!g_device->CreateVertexBufferStatic((unsigned int)color_buffer.size() * sizeof(YVector4), color_buffer.data(), d3d_vb)) {
            ERROR_INFO("Create vertex buffer failed!!");
            return false;
        }
        vertex_buffers_.push_back(d3d_vb);
    }



    {
        if (!g_device->CreateIndexBuffer((unsigned int)index_buffer.size() * sizeof(int), index_buffer.data(), index_buffer_)) {
            ERROR_INFO("Create index buffer failed!!");
            return false;
        }
    }

    {
        if (!g_device->CreateBuffer(sizeof(YVector4) * 3 * YGPUSkeltonMeshVertexFactory::GetGPUMatrixCount(), nullptr, true, bone_matrix_buffer_))
        {
            ERROR_INFO("Create bone matrix buffer failed!!");
            return false;
        }

        if (!g_device->CreateSRVForBuffer(DXGI_FORMAT_R32G32B32A32_FLOAT, 3 * YGPUSkeltonMeshVertexFactory::GetGPUMatrixCount(), bone_matrix_buffer_, bone_matrix_buffer_srv_, ""))
        {
            ERROR_INFO("Create bone matrix buffer failed!!");
            return false;
        }
    }

    {
        TComPtr<ID3D11Buffer> d3d_vb;
        if (!g_device->CreateVertexBufferStatic((unsigned int)bone_weight_buffer.size() * sizeof(float), bone_weight_buffer.data(), d3d_vb))
        {
            ERROR_INFO("Create vertex buffer failed!!");
            return false;
        }
        vertex_buffers_.push_back(d3d_vb);
    }

    {
        TComPtr<ID3D11Buffer> d3d_vb;
        if (!g_device->CreateVertexBufferStatic((unsigned int)bone_id_buffer.size() * sizeof(int), bone_id_buffer.data(), d3d_vb))
        {
            ERROR_INFO("Create vertex buffer failed!!");
            return false;
        }
        vertex_buffers_.push_back(d3d_vb);
    }

    {
        if (!g_device->CreateVertexBufferDynamic((unsigned int)position_buffer.size() * sizeof(YVector), nullptr, morph_position_offset_buffer)) {
            ERROR_INFO("Create vertex buffer failed!!");
            return false;
        }
        vertex_buffers_.push_back(morph_position_offset_buffer);
    }
    // vs
    {
        vertex_shader_ = std::make_unique<D3DVertexShader>();
        const std::string shader_path = "Shader/GPUSKin.hlsl";
        YFile vertex_shader_source(shader_path, YFile::FileType(YFile::FileType::FT_Read | YFile::FileType::FT_TXT));
        std::unique_ptr<MemoryFile> mem_file = vertex_shader_source.ReadFile();
        if (!mem_file)
        {
            ERROR_INFO("open shader ", shader_path, " failed!");
            return false;
        }
        std::string str(mem_file->GetReadOnlyFileContent().begin(), mem_file->GetReadOnlyFileContent().end());
        if (!vertex_shader_->CreateShaderFromSource(str, "VSMain", gpu_vertex_factory.get()))
        {
            return false;
        }
    }
    //ps
    {
        pixel_shader_ = std::make_unique<D3DPixelShader>();
        const std::string shader_path = "Shader/GPUSKin.hlsl";

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
    if (!blend_state_) {
        g_device->CreateBlendState(blend_state_, true);
    }

    if (!rs_) {
        g_device->CreateRasterStateNonCull(rs_);
    }

    if (!ds_) {
        g_device->CreateDepthStencileState(ds_, true);
    }
    if (!sampler_state_) {
        sampler_state_ = g_device->GetSamplerState(SF_BiLinear, SA_Wrap);
    }
    vertex_factory_ = std::move(gpu_vertex_factory);
    allocated_gpu_resource = true;
    return true;
}

void YSkeletonMesh::ReleaseGPUReosurce()
{
    vertex_buffers_.clear();
    index_buffer_ = nullptr;
    rs_ = nullptr;
    ds_ = nullptr;
    sampler_state_ = nullptr;
    allocated_gpu_resource = false;
}

AnimationData::AnimationData()
{

}
