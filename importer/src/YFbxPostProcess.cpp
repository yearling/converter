#include "YFbxPostProcess.h"
#include "MeshUtility/OverlappingCorners.h"
#include "MeshUtility/NvTriStrip.h"
#include "MeshUtility/nvtess.h"
#include <algorithm>
#include "Math/YColor.h"

PostProcessRenderMesh::PostProcessRenderMesh(ImportedRawMesh* raw_mesh, FbxImportParam* in_fbx_import_param)
    :raw_mesh_(raw_mesh){
    int wedege_count = (int)raw_mesh_->wedges.size();
    vertex_data_cache.reserve(wedege_count);
    section_indices.reserve(wedege_count);
    generate_reverse_index = in_fbx_import_param->generate_reverse_indices;
    generate_depth_only_index = in_fbx_import_param->generate_depth_only_indices;
    generate_reverse_depth_only_index = in_fbx_import_param->generate_reverse_depth_only_indices;
    generate_adjacent_index = in_fbx_import_param->generate_adjacency_indices;
}

void PostProcessRenderMesh::PostProcessPipeline()
{
    CompressVertex();
    OptimizeIndices();
    if (generate_adjacent_index)
    {
        BuildStaticAdjacencyIndexBuffer();
    }
    if (generate_reverse_index)
    {
        BuildReverseIndices();
    }

    if (generate_depth_only_index)
    {
        BuildDepthOnlyIndices();
        if (generate_reverse_depth_only_index)
        {
            BuildDepthOnlyInverseIndices();
        }
    }
    else if (generate_reverse_depth_only_index)
    {
        BuildDepthOnlyIndices();
        BuildDepthOnlyInverseIndices();
    }
}

namespace NvTriStripHelper
{
    /**
    * Orders a triangle list for better vertex cache coherency.
    *
    * *** WARNING: This is safe to call for multiple threads IF AND ONLY IF all
    * threads call SetListsOnly(true) and SetCacheSize(CACHESIZE_GEFORCE3). If
    * NvTriStrip is ever used with different settings the library will need
    * some modifications to be thread-safe. ***
    */
    void CacheOptimizeIndexBuffer(std::vector<uint32>& Indices)
    {
        PrimitiveGroup* PrimitiveGroups = NULL;
        uint32			NumPrimitiveGroups = 0;
        bool Is32Bit = true;

        SetListsOnly(true);
        SetCacheSize(CACHESIZE_GEFORCE3);

        GenerateStrips(Indices.data(), Indices.size(), &PrimitiveGroups, &NumPrimitiveGroups);

        Indices.clear();
        Indices.resize(PrimitiveGroups->numIndices, -1);

        if (Is32Bit)
        {
            memcpy(Indices.data(), PrimitiveGroups->indices, Indices.size() * sizeof(uint32));
        }
        else
        {
            for (uint32 I = 0; I < PrimitiveGroups->numIndices; ++I)
            {
                Indices[I] = (uint16)PrimitiveGroups->indices[I];
            }
        }
        delete[] PrimitiveGroups;
    }
}

void PostProcessRenderMesh::CompressVertex()
{
    LOG_INFO("begin compress vertex");
    vertex_data_cache.reserve(raw_mesh_->wedges.size());
    section_indices.resize(raw_mesh_->polygon_groups.size());
    for (int golygon_group_index = 0; golygon_group_index < raw_mesh_->polygon_groups.size(); ++golygon_group_index)
    {
        const YMeshPolygonGroup& polygon_group = raw_mesh_->polygon_groups[golygon_group_index];
        for (int triangle_id : polygon_group.polygons)
        {
            const YMeshPolygon& triangle = raw_mesh_->polygons[triangle_id];
            for (int i = 0; i < 3; ++i)
            {
                const YMeshWedge& wedge = raw_mesh_->wedges[triangle.wedge_ids[i]];
                FullStaticVertexData tmp;
                tmp.position = wedge.position;
                tmp.normal = wedge.normal;
                tmp.tangent = YVector4(wedge.tangent, wedge.binormal_sign);
                tmp.uv0 = wedge.uvs[0];
                tmp.uv1 = wedge.uvs[1];
                tmp.color = wedge.color;
                section_indices[golygon_group_index].push_back((int)vertex_data_cache.size());
                vertex_data_cache.push_back(tmp);
            }
        }
    }

    std::vector< FullStaticVertexData> compressed_vertex_data;
    compressed_vertex_data.reserve(vertex_data_cache.size());
    std::vector<int> map_old_to_new;
    map_old_to_new.resize(vertex_data_cache.size(), INDEX_NONE);

    std::vector<YVector> position_to_compare;
    position_to_compare.reserve(vertex_data_cache.size());
    for (FullStaticVertexData& data : vertex_data_cache)
    {
        position_to_compare.push_back(data.position);
    }

    std::vector<uint32> indices_to_compare;
    indices_to_compare.reserve(vertex_data_cache.size());
    for (std::vector<uint32>& section : section_indices)
    {
        for (int index : section)
        {
            indices_to_compare.push_back(index);
        }
    }
    YOverlappingCorners  acc_overlap_finding(position_to_compare, indices_to_compare, THRESH_POINTS_ARE_SAME);

    for (int i = 0; i < (int)indices_to_compare.size(); ++i)
    {
        int vertex_index = indices_to_compare[i];
        const FullStaticVertexData& data = vertex_data_cache[vertex_index];
        //查找是不是处理过了
        const std::vector<int>& result = acc_overlap_finding.FindIfOverlapping(i);
        if (map_old_to_new[vertex_index] == INDEX_NONE)
        {
            //注意，不包含自己
            int new_index = (int)compressed_vertex_data.size();
            compressed_vertex_data.push_back(data);
            for (int near_id : result)
            {
                int vertex_near_id = indices_to_compare[near_id];
                const FullStaticVertexData& new_vertex_data = vertex_data_cache[vertex_near_id];
                if (data == new_vertex_data)
                {
                    map_old_to_new[vertex_near_id] = new_index;
                }
            }
            map_old_to_new[vertex_index] = new_index;
        }

    }

    vertex_data_cache.swap(compressed_vertex_data);

    std::vector<std::vector<uint32>> compressed_section_index;
    compressed_section_index.resize(section_indices.size());
    for (int section_index = 0; section_index < section_indices.size(); ++section_index)
    {
        for (int index : section_indices[section_index])
        {
            compressed_section_index[section_index].push_back(map_old_to_new[index]);
        }
    }
    LOG_INFO("end compress vertex");
    //test
    section_indices.swap(compressed_section_index);
    LOG_INFO("compress vertex before is ", compressed_vertex_data.size(), "  after is ", vertex_data_cache.size(), "  diff is ", compressed_vertex_data.size() - vertex_data_cache.size());
}

void PostProcessRenderMesh::OptimizeIndices()
{
    //这万一比较慢，10w面以上就不要用了，ue里也是10w面
    int vertex_count = 0;
    for (std::vector<uint32>& section : section_indices)
    {
        vertex_count += section.size();
    }
    if (vertex_count > 300000)
    {
        return;
    }
    LOG_INFO("begin optimize indices");
    for (std::vector<uint32>& section_index : section_indices)
    {
        NvTriStripHelper::CacheOptimizeIndexBuffer(section_index);
    }
    //optimize vertex cache
    std::vector<FullStaticVertexData> vertex_data_cache_old;
    vertex_data_cache.swap(vertex_data_cache_old);
    vertex_data_cache.reserve(vertex_data_cache_old.size());
    int indices_count = 0;

    for (std::vector<uint32>& per_section : section_indices)
    {
        indices_count += (int)per_section.size();
    }
    std::vector<int> index_mapping_old_to_new;
    index_mapping_old_to_new.resize(indices_count, INDEX_NONE);

    for (int section_index = 0; section_index < section_indices.size(); ++section_index)
    {
        const std::vector<uint32>& per_section_index_old = section_indices[section_index];
        for (int index_index_old = 0; index_index_old < per_section_index_old.size(); ++index_index_old)
        {
            uint32 index_old = per_section_index_old[index_index_old];
            if (index_mapping_old_to_new[index_old] == INDEX_NONE)
            {
                int32 index_new = (int)vertex_data_cache.size();
                vertex_data_cache.push_back(vertex_data_cache_old[index_old]);
                index_mapping_old_to_new[index_old] = index_new;
            }
        }
    }

    for (std::vector<uint32>& sectoin : section_indices)
    {
        for (uint32& index : sectoin)
        {
            index = index_mapping_old_to_new[index];
        }
    }
    LOG_INFO("end optimize indices");
}



class FStaticMeshNvRenderBuffer : public nv::RenderBuffer
{
public:
    FStaticMeshNvRenderBuffer(PostProcessRenderMesh* in_post_process_render_mesh, int in_section_index);
    /** Retrieve the position and first texture coordinate of the specified index. */
    virtual nv::Vertex getVertex(unsigned int Index) const;

private:
    /** Copying is forbidden. */
    FStaticMeshNvRenderBuffer(const FStaticMeshNvRenderBuffer&);
    FStaticMeshNvRenderBuffer& operator=(const FStaticMeshNvRenderBuffer&);
    PostProcessRenderMesh* post_process_render_mesh_;
    int section_index_;
};

FStaticMeshNvRenderBuffer::FStaticMeshNvRenderBuffer(PostProcessRenderMesh* in_post_process_render_mesh, int in_secton_index)
    :post_process_render_mesh_(in_post_process_render_mesh), section_index_(in_secton_index) {
    std::vector<uint32>& index_ref = post_process_render_mesh_->section_indices[section_index_];
    mIb = new nv::IndexBuffer((void*)index_ref.data(), nv::IBT_U32, index_ref.size(), false);
}

nv::Vertex FStaticMeshNvRenderBuffer::getVertex(unsigned int Index) const
{
    nv::Vertex Vertex;

    //check(Index < PositionVertexBuffer.GetNumVertices());
    assert(Index < post_process_render_mesh_->vertex_data_cache.size());

    const YVector& Position = post_process_render_mesh_->vertex_data_cache[Index].position;
    Vertex.pos.x = Position.x;
    Vertex.pos.y = Position.y;
    Vertex.pos.z = Position.z;

    const YVector2& UV = post_process_render_mesh_->vertex_data_cache[Index].uv0;
    Vertex.uv.x = UV.x;
    Vertex.uv.y = UV.y;

    return Vertex;
}


void PostProcessRenderMesh::BuildStaticAdjacencyIndexBuffer()
{
    LOG_INFO("begin build adjacency index");
    adjacency_section_indices.clear();
    for (int i = 0; i < section_indices.size(); ++i)
    {
        FStaticMeshNvRenderBuffer StaticMeshRenderBuffer(this, i);
        nv::IndexBuffer* PnAENIndexBuffer = nv::tess::buildTessellationBuffer(&StaticMeshRenderBuffer, nv::DBM_PnAenDominantCorner, true);
        check(PnAENIndexBuffer);
        const int32 IndexCount = (int32)PnAENIndexBuffer->getLength();
        std::vector<uint32> new_adj_indices;
        new_adj_indices.resize(IndexCount);
        for (int32 Index = 0; Index < IndexCount; ++Index)
        {
            new_adj_indices[Index] = (*PnAENIndexBuffer)[Index];
        }
        delete PnAENIndexBuffer;
        adjacency_section_indices.emplace_back(std::move(new_adj_indices));
    }
    LOG_INFO("end build adjacency index");
}


void PostProcessRenderMesh::BuildReverseIndices()
{
    LOG_INFO("begin build reversed index");
    reversed_indices.clear();
    reversed_indices.resize(section_indices.size());
    for (int i = 0; i < section_indices.size(); ++i)
    {
        std::vector<uint32>& per_sec = section_indices[i];
        for (int j = 0; j < per_sec.size() / 3; ++j)
        {
            reversed_indices[i].push_back(per_sec[j * 3 + 2]);
            reversed_indices[i].push_back(per_sec[j * 3 + 1]);
            reversed_indices[i].push_back(per_sec[j * 3 + 0]);
        }
    }
    LOG_INFO("end build reversed index");
}

void PostProcessRenderMesh::BuildDepthOnlyIndices()
{
    LOG_INFO("begin build depth only index");
    std::vector<int> map_old_to_new;
    map_old_to_new.resize(vertex_data_cache.size(), INDEX_NONE);

    std::vector<YVector> position_to_compare;
    position_to_compare.reserve(vertex_data_cache.size());
    for (FullStaticVertexData& data : vertex_data_cache)
    {
        position_to_compare.push_back(data.position);
    }

    std::vector<uint32> indices_to_compare;
    indices_to_compare.reserve(vertex_data_cache.size());
    for (std::vector<uint32>& section : section_indices)
    {
        for (int index : section)
        {
            indices_to_compare.push_back(index);
        }
    }
    YOverlappingCorners  acc_overlap_finding(position_to_compare, indices_to_compare, THRESH_POINTS_ARE_SAME);

    for (int i = 0; i < (int)indices_to_compare.size(); ++i)
    {
        int vertex_index = indices_to_compare[i];
        const FullStaticVertexData& data = vertex_data_cache[vertex_index];
        //查找是不是处理过了
        const std::vector<int>& result = acc_overlap_finding.FindIfOverlapping(i);
        if (map_old_to_new[vertex_index] == INDEX_NONE)
        {
            //注意，不包含自己
            std::vector<int> connected_corner;
            for (int near_id : result)
            {
                int vertex_near_id = indices_to_compare[near_id];
                const FullStaticVertexData& new_vertex_data = vertex_data_cache[vertex_near_id];
                if (data.position.Equals(new_vertex_data.position, THRESH_POINTS_ARE_SAME))
                {
                    connected_corner.push_back(vertex_near_id);
                }
            }
            connected_corner.push_back(vertex_index);
            std::sort(connected_corner.begin(), connected_corner.end());
            int small_index = connected_corner[0];
            for (int index_near : connected_corner)
            {
                map_old_to_new[index_near] = small_index;
            }
        }
    }


    depth_only_indices.clear();
    depth_only_indices.resize(section_indices.size());
    for (int section_index = 0; section_index < section_indices.size(); ++section_index)
    {
        for (int index : section_indices[section_index])
        {
            depth_only_indices[section_index].push_back(map_old_to_new[index]);
        }
    }

    if (indices_to_compare.size() < 50000 * 3)
    {
        for (std::vector<uint32>& section_index : section_indices)
        {
            NvTriStripHelper::CacheOptimizeIndexBuffer(section_index);
        }
    }
    LOG_INFO("end build depth only index");
}

void PostProcessRenderMesh::BuildDepthOnlyInverseIndices()
{
    LOG_INFO("begin build depth only reversed index");
    depth_only_reversed_indices.clear();
    depth_only_reversed_indices.resize(depth_only_indices.size());
    for (int i = 0; i < depth_only_indices.size(); ++i)
    {
        std::vector<uint32>& per_sec = depth_only_indices[i];
        for (int j = 0; j < per_sec.size() / 3; ++j)
        {
            depth_only_reversed_indices[i].push_back(per_sec[j * 3 + 2]);
            depth_only_reversed_indices[i].push_back(per_sec[j * 3 + 1]);
            depth_only_reversed_indices[i].push_back(per_sec[j * 3 + 0]);
        }
    }
    LOG_INFO("end build depth only reversed index");
}

std::unique_ptr< HiStaticVertexData> PostProcessRenderMesh::GenerateHiStaticVertexData()
{
    PostProcessPipeline();
    std::unique_ptr<HiStaticVertexData> render_data = std::make_unique<HiStaticVertexData>();
    render_data->position.reserve(vertex_data_cache.size());
    render_data->vertex_infos.reserve(vertex_data_cache.size());
    for (FullStaticVertexData& full_vertex_data : vertex_data_cache)
    {
        render_data->position.push_back(full_vertex_data.position);
        HiStaticVertexData::HiVertexInfo tmp;
        tmp.normal = YVector4(full_vertex_data.normal, 0.0);
        tmp.tangent = full_vertex_data.tangent;
        tmp.uv0 = full_vertex_data.uv0;
        tmp.uv1 = full_vertex_data.uv1;
        tmp.color = FLinearColor(full_vertex_data.color).ToFColor(false).AlignmentDummy;
        render_data->vertex_infos.push_back(tmp);
    }

    render_data->GenerateIndexBuffers(render_data->indices_vertex_32, render_data->indices_vertex_16, section_indices, true);
    if (generate_reverse_index)
    {
        render_data->GenerateIndexBuffers(render_data->indices_vertex_reversed_32, render_data->indices_vertex_reversed_16, reversed_indices);
        render_data->has_reverse_index = true;
    }

    if (generate_depth_only_index)
    {
        render_data->GenerateIndexBuffers(render_data->indices_depth_only_32, render_data->indices_depth_only_16, depth_only_indices);
        render_data->has_depth_only_index = true;
    }

    if (generate_reverse_depth_only_index)
    {
        render_data->GenerateIndexBuffers(render_data->indices_depth_only_reversed_32, render_data->indices_depth_only_reversed_16, depth_only_reversed_indices);
        render_data->has_reverse_depth_only_index = true;
    }

    if (generate_adjacent_index)
    {
        render_data->GenerateIndexBuffers(render_data->indices_adjacent_32, render_data->indices_adjacent_16, adjacency_section_indices);
        render_data->has_adjacent_index = true;
    }
    render_data->aabb = raw_mesh_->aabb;
    return render_data;
}

std::unique_ptr< DefaultStaticVertexData> PostProcessRenderMesh::GenerateMediumStaticVertexData()
{
    PostProcessPipeline();
    std::unique_ptr<DefaultStaticVertexData> render_data = std::make_unique<DefaultStaticVertexData>();
    render_data->position.reserve(vertex_data_cache.size());
    render_data->vertex_infos.reserve(vertex_data_cache.size());
    for (FullStaticVertexData& full_vertex_data : vertex_data_cache)
    {
        render_data->position.push_back(full_vertex_data.position);
        DefaultStaticVertexData::DefaultVertexInfo tmp;
        tmp.normal = YVector4(full_vertex_data.normal, 0.0);
        tmp.tangent = full_vertex_data.tangent;
        tmp.uv0 = full_vertex_data.uv0;
        tmp.uv1 = full_vertex_data.uv1;
        render_data->vertex_infos.push_back(tmp);
    }

    render_data->GenerateIndexBuffers(render_data->indices_vertex_32, render_data->indices_vertex_16, section_indices, true);
    if (generate_reverse_index)
    {
        render_data->GenerateIndexBuffers(render_data->indices_vertex_reversed_32, render_data->indices_vertex_reversed_16, reversed_indices);
        render_data->has_reverse_index = true;
    }

    if (generate_depth_only_index)
    {
        render_data->GenerateIndexBuffers(render_data->indices_depth_only_32, render_data->indices_depth_only_16, depth_only_indices);
        render_data->has_depth_only_index = true;
    }

    if (generate_reverse_depth_only_index)
    {
        render_data->GenerateIndexBuffers(render_data->indices_depth_only_reversed_32, render_data->indices_depth_only_reversed_16, depth_only_reversed_indices);
        render_data->has_reverse_depth_only_index = true;
    }

    if (generate_adjacent_index)
    {
        render_data->GenerateIndexBuffers(render_data->indices_adjacent_32, render_data->indices_adjacent_16, adjacency_section_indices);
        render_data->has_adjacent_index = true;
    }
    render_data->aabb = raw_mesh_->aabb;
    return render_data;
}

void StaticVertexRenderData::GenerateIndexBuffers(std::vector<uint32>& indices_32, std::vector<uint16>& indices_16, const std::vector<std::vector<uint32>>& section_indices, bool genereate_section_info)
{
    if (position.size() > MAX_uint16)
    {
        use_32_indices = true;
    }
    else
    {
        use_32_indices = false;
    }

    int triangle_count = 0;
    for (const std::vector<uint32>& indices_per_sec : section_indices)
    {
        triangle_count += (int)indices_per_sec.size();
    }
    if (use_32_indices)
    {
        indices_32.reserve(triangle_count * 3);
    }
    else
    {
        indices_16.reserve(triangle_count * 3);
    }



    for (int section_index = 0; section_index < section_indices.size(); ++section_index)
    {
        for (int index : section_indices[section_index])
        {
            if (use_32_indices)
            {
                indices_32.push_back((uint32)index);
            }
            else
            {
                indices_16.push_back((uint16)index);
            }
        }
    }

    if (genereate_section_info)
    {
        int section_offset = 0;
        sections.resize(section_indices.size());
        for (int section_index = 0; section_index < section_indices.size(); ++section_index)
        {
            sections[section_index].offset = section_offset;
            sections[section_index].triangle_count = section_indices[section_index].size() / 3;
            int min_index = MAX_int32;
            int max_index = MIN_int32;
            for (int index : section_indices[section_index])
            {
                min_index = YMath::Min(min_index, index);
                max_index = YMath::Max(max_index, index);
            }
            sections[section_index].min_vertex_index = min_index;
            sections[section_index].max_vertex_index = max_index;
            section_offset += section_indices[section_index].size();
        }
    }
}

uint32 StaticVertexRenderData::GetVertexInfoSize()
{
    return 0;
}

void* StaticVertexRenderData::GetVertexInfoData()
{
    return nullptr;
}

uint32 HiStaticVertexData::GetVertexInfoSize()
{
    return sizeof(HiStaticVertexData::HiVertexInfo) * vertex_infos.size();
}

void* HiStaticVertexData::GetVertexInfoData()
{
    return vertex_infos.data();
}

uint32 DefaultStaticVertexData::GetVertexInfoSize()
{
    return sizeof(DefaultStaticVertexData::DefaultVertexInfo) * vertex_infos.size();
}

void* DefaultStaticVertexData::GetVertexInfoData()
{
    return vertex_infos.data();
}

bool FullStaticVertexData::operator==(const FullStaticVertexData& other) const
{
    if (!position.Equals(other.position))
    {
        return false;
    }
    if (!normal.Equals(other.normal))
    {
        return false;
    }

    if (!tangent.Equals(other.tangent))
    {
        return false;
    }

    if (!uv0.Equals(other.uv0))
    {
        return false;
    }

    if (!uv1.Equals(other.uv1))
    {
        return false;
    }

    if (!color.Equals(other.color))
    {
        return false;
    }

    return true;
}