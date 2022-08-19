#include "Engine/YRawMesh.h"
#include <cassert>
#include <unordered_set>
#include <algorithm>
#include <set>
#include "Utility/YStringFormat.h"
#include "Math/YVector.h"
#include "Engine/YArchive.h"
#include "Math/NumericLimits.h"
#include "MeshUtility/mikktspace.h"
#include "Math/YColor.h"
#include "MeshUtility/OverlappingCorners.h"
#include "MeshUtility/LayoutUV.h"
#include "Math/YMath.h"

void YMeshEdge::AddTriangleID(int triangle_id)
{
    auto find_reuslt = std::find(connected_triangles.begin(), connected_triangles.end(), triangle_id);
    if (find_reuslt == connected_triangles.end())
    {
        connected_triangles.push_back(triangle_id);
    }
}
YMeshWedge::YMeshWedge()
{
    uvs.resize(MAX_MESH_TEXTURE_COORDS, YVector2(0.0, 0.0));
}

bool YMeshWedge::operator==(const YMeshWedge& rhs)const
{
    if (control_point_id != rhs.control_point_id)
    {
        return false;
    }

    if (!position.Equals(rhs.position))
    {
        return false;
    }
    if (!normal.Equals(rhs.normal))
    {
        return false;
    }

    if (!tangent.Equals(rhs.tangent))
    {
        return false;
    }

    if (!bitangent.Equals(rhs.bitangent))
    {
        return false;
    }

    if (!YMath::IsNearlyEqual(binormal_sign, rhs.binormal_sign))
    {
        return false;
    }

    if (!color.Equals(rhs.color))
    {
        return false;
    }

    for (int i = 0; i < MAX_MESH_TEXTURE_COORDS; ++i)
    {
        if (!uvs[i].Equals(rhs.uvs[i]))
        {
            return false;
        }
    }
    return true;
}
void YMeshWedge::AddTriangleID(int triangle_id)
{
    auto find_reuslt = std::find(connected_triangles.begin(), connected_triangles.end(), triangle_id);
    if (find_reuslt == connected_triangles.end())
    {
        connected_triangles.push_back(triangle_id);
    }
}

void YMeshControlPoint::AddWedge(int index)
{
    auto find_reuslt = std::find(wedge_ids.begin(), wedge_ids.end(), index);
    if (find_reuslt == wedge_ids.end())
    {
        wedge_ids.push_back(index);
    }

}

ImportedRawMesh::ImportedRawMesh()
    :LOD_index(INVALID_ID) {

}

int ImportedRawMesh::GetVertexPairEdge(int control_point_id0, int control_id1) const
{
    //vertex
    const std::vector<int>& connect_edges = control_points[control_point_id0].edge_ids;
    for (int edge : connect_edges)
    {
        int vertex_maybe_0 = edges[edge].control_points_ids[0];
        int vertex_maybe_1 = edges[edge].control_points_ids[1];
        if ((vertex_maybe_0 == control_point_id0 && vertex_maybe_1 == control_id1) || (vertex_maybe_0 == control_id1 && vertex_maybe_1 == control_point_id0))
        {
            return edge;
        }
    }
    return INVALID_ID;
}

int ImportedRawMesh::CreateEdge(int control_point_id_0, int control_point_id_1)
{
#if defined(DEBUG) || defined(_DEBUG)
    int exist_id = GetVertexPairEdge(control_point_id_0, control_point_id_1);
    assert(exist_id == INVALID_ID);
#endif
    YMeshEdge tmp_edge;
    tmp_edge.control_points_ids[0] = control_point_id_0;
    tmp_edge.control_points_ids[1] = control_point_id_1;
    int edge_id = (int)edges.size();
    edges.push_back(tmp_edge);
    control_points[control_point_id_0].edge_ids.push_back(edge_id);
    control_points[control_point_id_1].edge_ids.push_back(edge_id);
    return edge_id;
}

int ImportedRawMesh::CreatePolygon(int polygon_group_id, std::vector<int> in_wedges, std::vector<int>& out_edges)
{
    out_edges.clear();
    // create triangle
    int polygon_id = (int)polygons.size();
    polygons.push_back(YMeshPolygon());

    YMeshPolygon& tmp_polygon = polygons[polygon_id];
    // polygon_group, both reference
    tmp_polygon.polygon_group_id = polygon_group_id;
    tmp_polygon.wedge_ids = in_wedges;
    polygon_groups[polygon_group_id].polygons.push_back(polygon_id);

    //only support triangle,UE support polygon
    for (int i = 0; i < in_wedges.size(); ++i)
    {
        wedges[in_wedges[i]].AddTriangleID(polygon_id);
        int i_next = (i + 1) % ((int)in_wedges.size());
        int control_point_id = wedges[in_wedges[i]].control_point_id;
        int next_control_point_id = wedges[in_wedges[i_next]].control_point_id;
        int edge_idex = GetVertexPairEdge(control_point_id, next_control_point_id);

        //create edges
        if (edge_idex == INVALID_ID)
        {
            assert("should not here, out side has created");
            edge_idex = CreateEdge(control_point_id, next_control_point_id);
            out_edges.push_back(edge_idex);
        }
        edges[edge_idex].AddTriangleID(polygon_id);
    }

    // calculate triangle aera and wedge corner angle for normal average
    YMeshWedge& wedge0 = wedges[tmp_polygon.wedge_ids[0]];
    YMeshWedge& wedge1 = wedges[tmp_polygon.wedge_ids[1]];
    YMeshWedge& wedge2 = wedges[tmp_polygon.wedge_ids[2]];
    YVector position0 = control_points[wedge0.control_point_id].position;
    YVector position1 = control_points[wedge1.control_point_id].position;
    YVector position2 = control_points[wedge2.control_point_id].position;
    tmp_polygon.face_aera = CalculateTriangleArea(position0, position1, position2);
    wedge0.corner_angle = ComputeTriangleCornerAngle(position0, position1, position2);
    wedge1.corner_angle = ComputeTriangleCornerAngle(position1, position0, position2);
    wedge2.corner_angle = ComputeTriangleCornerAngle(position2, position0, position1);
    return polygon_id;
}

void ImportedRawMesh::ComputeAABB()
{
    aabb.Init();
    for (YMeshControlPoint& v : control_points)
    {
        aabb += v.position;
    }
}

void ImportedRawMesh::CompressControlPoint()
{
    std::set<int> control_point_set;
    for (int i = 0; i < control_points.size(); ++i)
    {
        control_point_set.insert(i);
    }

    std::set<int> used_control_point_set;
    //因为有退化三角形，有些没引用到的三角形的control point 可以删掉
    //统计没有使用到的control point
    for (int polygon_group_id = 0; polygon_group_id < (int)polygon_groups.size(); ++polygon_group_id)
    {
        const YMeshPolygonGroup& polygon_group = polygon_groups[polygon_group_id];
        for (int polygon_id : polygon_group.polygons)
        {
            const YMeshPolygon& polygon = polygons[polygon_id];
            for (int wedge_index = 0; wedge_index < 3; ++wedge_index)
            {
                int wedge_id = polygon.wedge_ids[wedge_index];
                int contro_point_id = wedges[wedge_id].control_point_id;
                used_control_point_set.insert(contro_point_id);
            }
        }
    }
    if (control_point_set.size() != used_control_point_set.size())
    {
        assert(used_control_point_set.size() < control_point_set.size());
    }

    std::vector<int> diff_result(control_point_set.size());
    auto iter = std::set_difference(control_point_set.begin(), control_point_set.end(), used_control_point_set.begin(), used_control_point_set.end(), diff_result.begin());
    diff_result.resize(iter - diff_result.begin());

    auto func_in_diff_set = [&](int n) {
        return std::find(diff_result.begin(), diff_result.end(), n) != diff_result.end();
    };


    bool result = func_in_diff_set(2);
    std::vector<int> old_to_new;
    std::vector<int> new_to_old;
    old_to_new.resize(control_point_set.size(), -1);
    new_to_old.resize(used_control_point_set.size(), -1);
    std::vector<YMeshControlPoint> new_control_points;
    new_control_points.reserve(control_points.size());
    for (int i = 0; i < (int)control_points.size(); ++i)
    {
        if (func_in_diff_set(i))
        {
            old_to_new[i] = -1;
        }
        else
        {
            int new_position = new_control_points.size();
            new_control_points.push_back(control_points[i]);
            old_to_new[i] = new_position;
            new_to_old[new_position] = i;
        }
    }

    int before_size = (int)control_points.size();
    int after_size = (int)new_control_points.size();
    control_points.swap(new_control_points);

    for (YMeshWedge& wedge : wedges)
    {
        wedge.control_point_id = old_to_new[wedge.control_point_id];
    }

    for (YMeshEdge& edge : edges)
    {
        edge.control_points_ids[0] = old_to_new[edge.control_points_ids[0]];
        edge.control_points_ids[1] = old_to_new[edge.control_points_ids[1]];
    }

    WARNING_INFO(StringFormat("remove mesh's %s degenerate triangle, before is %d , after is %d, smaller %d", name.c_str(), before_size, after_size, before_size - after_size));
}


bool ImportedRawMesh::Valid() const
{
    for (int polygon_group_id = 0; polygon_group_id < (int)polygon_groups.size(); ++polygon_group_id)
    {
        const YMeshPolygonGroup& polygon_group = polygon_groups[polygon_group_id];
        for (int polygon_id : polygon_group.polygons)
        {
            const YMeshPolygon& polygon = polygons[polygon_id];
            if (polygon.wedge_ids.size() != 3)
            {
                WARNING_INFO("Imported RawMesh Valid: polygon only support triangle");
                return false;
            }
            int tmp_control_points[3];
            for (int wedge_index = 0; wedge_index < 3; ++wedge_index)
            {
                int wedge_id = polygon.wedge_ids[wedge_index];
                {
                    if (wedge_id >= wedges.size() || wedge_id <= INVALID_ID)
                    {
                        WARNING_INFO("Imported RawMesh Valid: Wrong wedge index");
                        return false;
                    }
                    int control_point_id = wedges[wedge_id].control_point_id;
                    if (control_point_id >= control_points.size() || control_point_id <= INVALID_ID)
                    {
                        WARNING_INFO("Imported RawMesh Valid: Wrong control point index");
                        return false;
                    }
                    tmp_control_points[wedge_index] = control_point_id;

                    const std::vector<int>& traingle_ids = wedges[wedge_id].connected_triangles;
                    if (std::find(traingle_ids.begin(), traingle_ids.end(), polygon_id) == traingle_ids.end())
                    {
                        WARNING_INFO("Imported RawMesh Valid: Wrong edge triangle relation");
                        return false;
                    }
                }
            }

            for (int i = 0; i < 3; ++i)
            {
                int i_next = (i + 1) % 3;
                int edge_index = GetVertexPairEdge(tmp_control_points[i], tmp_control_points[i_next]);
                if (edge_index <= INVALID_ID || edge_index >= edges.size())
                {
                    WARNING_INFO("Imported RawMesh Valid: Wrong edge index");
                    return false;
                }

                const std::vector<int>& traingle_ids = edges[edge_index].connected_triangles;
                if (std::find(traingle_ids.begin(), traingle_ids.end(), polygon_id) == traingle_ids.end())
                {
                    WARNING_INFO("Imported RawMesh Valid: Wrong edge triangle relation");
                    return false;
                }
            }
        }
    }

    //for (const YMeshControlPoint& control_point : control_points)
    for (int control_point_id = 0; control_point_id < (int)control_points.size(); ++control_point_id)
    {
        const YMeshControlPoint& control_point = control_points[control_point_id];
        for (int wedge_id : control_point.wedge_ids)
        {
            if (wedge_id >= wedges.size() || wedge_id <= INVALID_ID)
            {
                WARNING_INFO("Imported RawMesh Valid: Wrong wedge index");
                return false;
            }
            if (wedges[wedge_id].control_point_id != control_point_id)
            {
                WARNING_INFO("Imported RawMesh Valid: control_point_id not equal wedges's control_point id");
                return false;
            }
        }

        for (int edge_id : control_point.edge_ids)
        {
            if (edge_id <= INVALID_ID || edge_id >= edges.size())
            {
                WARNING_INFO("Imported RawMesh Valid: Wrong edge index");
                return false;
            }

            const YMeshEdge& edge = edges[edge_id];
            if ((edge.control_points_ids[0] != control_point_id) && (edge.control_points_ids[1] != control_point_id))
            {
                WARNING_INFO("Imported RawMesh Valid: edge reference control point wrong");
                return false;
            }

        }
    }
    return true;
}

void ImportedRawMesh::Merge(ImportedRawMesh& other)
{
    // control point
    int self_control_point_size = (int)control_points.size();
    int self_wedge_size = (int)wedges.size();
    int self_polygon_size = (int)polygons.size();
    int self_edge_size = (int)edges.size();
    //control_points.insert(control_points.begin(), other.control_points.begin(), other.control_points.end());
    control_points.reserve(control_points.size() + other.control_points.size());
    for (YMeshControlPoint& other_control_point : other.control_points)
    {
        control_points.push_back(other_control_point);
        YMeshControlPoint& new_control_pont = control_points.back();
        for (int& wedge_id : new_control_pont.wedge_ids)
        {
            wedge_id += self_wedge_size;
        }
        for (int& edge_id : new_control_pont.edge_ids)
        {
            edge_id += self_edge_size;
        }
    }

    // wedge
    wedges.reserve(wedges.size() + other.wedges.size());
    for (YMeshWedge& other_wedge : other.wedges)
    {
        wedges.push_back(other_wedge);
        YMeshWedge& new_wedge = wedges.back();
        new_wedge.control_point_id += self_control_point_size;
        for (int& triangle_id : new_wedge.connected_triangles)
        {
            triangle_id += self_polygon_size;
        }
    }

    //edge

    edges.reserve(edges.size() + other.edges.size());
    for (YMeshEdge& other_edge : other.edges)
    {
        edges.push_back(other_edge);
        YMeshEdge& new_edge = edges.back();
        new_edge.control_points_ids[0] += self_control_point_size;
        new_edge.control_points_ids[1] += self_control_point_size;
        for (int& triangle_id : new_edge.connected_triangles)
        {
            triangle_id += self_polygon_size;
        }
    }

    // triangle
    polygons.reserve(polygons.size() + other.polygons.size());
    for (YMeshPolygon& polygon : other.polygons)
    {
        polygons.push_back(polygon);
        YMeshPolygon& new_polygon = polygons.back();
        for (int& wedge_id : new_polygon.wedge_ids)
        {
            wedge_id += self_wedge_size;
        }
    }

    //polygon group
    polygon_groups.reserve(polygon_groups.size() + other.polygon_groups.size());
    for (int other_polygon_group_index = 0; other_polygon_group_index < other.polygon_groups.size(); ++other_polygon_group_index)
    {
        YMeshPolygonGroup& other_polygon_group = other.polygon_groups[other_polygon_group_index];
        const int new_polygon_group_id = polygon_groups.size();
        polygon_groups.push_back(other_polygon_group);
        polygon_group_to_material.push_back(other.polygon_group_to_material[other_polygon_group_index]);
        YMeshPolygonGroup& new_ploygon_group = polygon_groups.back();
        for (int& polygon_id : new_ploygon_group.polygons)
        {
            polygon_id += self_polygon_size;
        }
    }

    ComputeAABB();
    CompressMaterials();
    assert(Valid());
}

void ImportedRawMesh::ComputeFaceNormalAndTangent(NormalCaculateMethod normal_method, TangentMethod tangent_method)
{
    if (normal_method == NormalCaculateMethod::ImportNormalAndTangnet)
    {
        return;
    }

    float AdjustedComparisonThreshold = YMath::Max(SMALL_NUMBER, MIN_flt);
    for (const YMeshPolygonGroup& polygon_group : polygon_groups)
    {
        for (int polygon_id : polygon_group.polygons)
        {
            bool bValidNTBs = true;
            // Calculate the tangent basis for the polygon, based on the average of all constituent triangles
            YVector Normal(YVector::zero_vector);
            YVector Tangent(YVector::zero_vector);
            YVector Binormal(YVector::zero_vector);

            YMeshPolygon& polygon = polygons[polygon_id];
            const std::vector<int>& wedge_ids = polygon.wedge_ids;
            int control_point_id0 = wedges[wedge_ids[0]].control_point_id;
            int control_point_id1 = wedges[wedge_ids[1]].control_point_id;
            int control_point_id2 = wedges[wedge_ids[2]].control_point_id;

            const YVector Position0 = control_points[control_point_id0].position;
            const YVector DPosition1 = control_points[control_point_id1].position - Position0;
            const YVector DPosition2 = control_points[control_point_id2].position - Position0;

            const YVector2 UV0 = wedges[wedge_ids[0]].uvs[0];
            const YVector2 DUV1 = wedges[wedge_ids[1]].uvs[0] - UV0;
            const YVector2 DUV2 = wedges[wedge_ids[2]].uvs[0] - UV0;

            // We have a left-handed coordinate system, but a counter-clockwise winding order
            // Hence normal calculation has to take the triangle vectors cross product in reverse.
            YVector TmpNormal = YVector::CrossProduct(DPosition2, DPosition1).GetSafeNormal(AdjustedComparisonThreshold);
            if (!TmpNormal.IsNearlyZero(SMALL_NUMBER))
            {
                YMatrix	ParameterToLocal(
                    YVector4(DPosition1, 0.0f),
                    YVector4(DPosition2, 0.0f),
                    YVector4(Position0, 0.0f),
                    YVector4(0.0, 0.0, 0.0, 1.0)
                );

                YMatrix ParameterToTexture(
                    YVector4(DUV1.x, DUV1.y, 0, 0),
                    YVector4(DUV2.x, DUV2.y, 0, 0),
                    YVector4(UV0.x, UV0.y, 1, 0),
                    YVector4(0, 0, 0, 1)
                );

                // Use InverseSlow to catch singular matrices.  Inverse can miss this sometimes.
                const YMatrix TextureToLocal = ParameterToTexture.Inverse() * ParameterToLocal;

                YVector TmpTangent = TextureToLocal.TransformVector(YVector(1, 0, 0)).GetSafeNormal();
                YVector TmpBinormal = TextureToLocal.TransformVector(YVector(0, 1, 0)).GetSafeNormal();
                YVector::CreateOrthonormalBasis(TmpTangent, TmpBinormal, TmpNormal);

                if (TmpTangent.IsNearlyZero() || TmpTangent.ContainsNaN()
                    || TmpBinormal.IsNearlyZero() || TmpBinormal.ContainsNaN())
                {
                    TmpTangent = YVector::zero_vector;
                    TmpBinormal = YVector::zero_vector;
                    bValidNTBs = false;
                }

                if (TmpNormal.IsNearlyZero() || TmpNormal.ContainsNaN())
                {
                    TmpNormal = YVector::zero_vector;
                    bValidNTBs = false;
                }

                Normal = TmpNormal;
                Tangent = TmpTangent;
                Binormal = TmpBinormal;
            }
            else
            {
                //This will force a recompute of the normals and tangents
                Normal = YVector::zero_vector;
                Tangent = YVector::zero_vector;
                Binormal = YVector::zero_vector;

                // The polygon is degenerated
                bValidNTBs = false;
            }
            if (!bValidNTBs)
            {
                //WARNING_INFO(StringFormat("triangle %d has bad NTB", polygon_id));
            }
            polygon.face_normal = Normal.GetSafeNormal();
            polygon.face_tangent = Tangent.GetSafeNormal();
            polygon.face_bitangent = Binormal.GetSafeNormal();

            float determinant = YMatrix(YVector4(polygon.face_tangent, 0.0), YVector4(polygon.face_bitangent, 0.0), YVector4(polygon.face_normal, 0.0), YVector4(0.0, 0.0, 0.0, 1.0)).Determinant();
            polygon.face_bitanget_sign = determinant < 0 ? -1.0 : 1.0;
        }
    }
}



void ImportedRawMesh::ComputeUVSeam()
{
    for (int i = 0; i < (int)edges.size(); ++i)
    {
        edges[i].is_uv_seam = IsUVSeam(i);
    }
}

void ImportedRawMesh::CompressMaterials()
{
    std::unordered_map<std::shared_ptr<YImportedMaterial>, int > map_material_to_new_group_id;
    std::vector< YMeshPolygonGroup> new_polygon_groups;
    std::vector< std::shared_ptr<YImportedMaterial>> new_polygon_group_to_material;
    for (int polygon_group_index = 0; polygon_group_index < polygon_group_to_material.size(); ++polygon_group_index)
    {
        std::shared_ptr<YImportedMaterial> material = polygon_group_to_material[polygon_group_index];
        if (!map_material_to_new_group_id.count(material))
        {
            int new_polygon_group_id = new_polygon_groups.size();
            new_polygon_groups.push_back(YMeshPolygonGroup());
            new_polygon_group_to_material.push_back(material);
            map_material_to_new_group_id[material] = new_polygon_group_id;
        }
        int new_group_id = map_material_to_new_group_id[material];
        YMeshPolygonGroup& new_polygon_group = new_polygon_groups[new_group_id];
        new_polygon_group.polygons.insert(new_polygon_group.polygons.end(), polygon_groups[polygon_group_index].polygons.begin(), polygon_groups[polygon_group_index].polygons.end());
    }
    polygon_groups.swap(new_polygon_groups);
    polygon_group_to_material.swap(new_polygon_group_to_material);
}

struct FLayoutUVRawMeshView final : FLayoutUV::IMeshView
{
    ImportedRawMesh* RawMesh;
    const uint32 SrcChannel;
    const uint32 DstChannel;
    const bool bNormalsValid;

    FLayoutUVRawMeshView(ImportedRawMesh* InRawMesh, uint32 InSrcChannel, uint32 InDstChannel)
        : RawMesh(InRawMesh)
        , SrcChannel(InSrcChannel)
        , DstChannel(InDstChannel)
        , bNormalsValid(true)
    {
    }

    uint32     GetNumIndices() const override
    {
        return RawMesh->wedges.size();
    }
    YVector    GetPosition(uint32 Index) const override
    {
        return RawMesh->wedges[Index].position;
    }
    YVector    GetNormal(uint32 Index) const override
    {
        return RawMesh->wedges[Index].normal;
    }
    YVector2  GetInputTexcoord(uint32 Index) const override {
        return RawMesh->wedges[Index].uvs[SrcChannel];
    }

    void      InitOutputTexcoords(uint32 Num) override {
        for (YMeshWedge& wedge : RawMesh->wedges)
        {
            wedge.uvs.push_back(YVector2());
        }
    }
    void      SetOutputTexcoord(uint32 Index, const YVector2& Value) override
    {
        RawMesh->wedges[Index].uvs[DstChannel] = Value;
    }
};

void ImportedRawMesh::GenerateLightMapUV()
{
    LOG_INFO("Begin Generate light map uv");
    if (TellLightMapUVExist())
    {
        LOG_INFO("mesh has second uv, generate light map uv exit");
        return;
    }

    FLayoutUVRawMeshView raw_mesh_view(this, 0, 1);
    FLayoutUV packer(raw_mesh_view);
    packer.SetVersion(ELightmapUVVersion::OptimalSurfaceArea);
    std::vector<YVector> position_to_compare;
    position_to_compare.reserve(wedges.size());
    std::vector<uint32> indices_to_compare;
    indices_to_compare.reserve(wedges.size());
    for (int wedge_index = 0; wedge_index < wedges.size(); ++wedge_index)
    {
        YMeshWedge& data = wedges[wedge_index];
        position_to_compare.push_back(data.position);
        indices_to_compare.push_back(wedge_index);
    }
    YOverlappingCorners  overlapping_corners(position_to_compare, indices_to_compare, THRESH_POINTS_ARE_SAME);
    packer.FindCharts(overlapping_corners);
    int32 effective_min_lightmap_resolution = 1024;
    const int32 light_padding = 2;
    effective_min_lightmap_resolution -= light_padding;
    bool pack_success = packer.FindBestPacking(effective_min_lightmap_resolution);
    if (pack_success)
    {
        packer.CommitPackedUVs();
    }
    LOG_INFO("End Generate light map uv");
}

void ImportedRawMesh::CompressWedges()
{
    std::vector<int> map_old_to_new;
    map_old_to_new.resize(wedges.size(), INDEX_NONE);

    std::vector<YVector> position_to_compare;
    position_to_compare.reserve(wedges.size());
    for (YMeshWedge& wedge : wedges)
    {
        position_to_compare.push_back(wedge.position);
    }

    std::vector<uint32> indices_to_compare;
    indices_to_compare.reserve(wedges.size());
    for (YMeshPolygonGroup& group : polygon_groups)
    {
        for (int polygon_index : group.polygons)
        {
            YMeshPolygon& polygon = polygons[polygon_index];
            indices_to_compare.push_back(polygon.wedge_ids[0]);
            indices_to_compare.push_back(polygon.wedge_ids[1]);
            indices_to_compare.push_back(polygon.wedge_ids[2]);
        }
    }
    YOverlappingCorners  acc_overlap_finding(position_to_compare, indices_to_compare, THRESH_POINTS_ARE_SAME);
    std::vector<YMeshWedge> compressed_wedges;
    compressed_wedges.reserve(wedges.size());
    for (int i = 0; i < (int)indices_to_compare.size(); ++i)
    {
        int wedge_index = indices_to_compare[i];
        const YMeshWedge& wedge = wedges[wedge_index];
        //查找是不是处理过了
        const std::vector<int>& result = acc_overlap_finding.FindIfOverlapping(i);
        if (map_old_to_new[wedge_index] == INDEX_NONE)
        {
            //注意，不包含自己
            int new_index = (int)compressed_wedges.size();
            compressed_wedges.push_back(wedge);
            for (int near_id : result)
            {
                int vertex_near_id = indices_to_compare[near_id];
                const YMeshWedge& same_compare_wedge = wedges[vertex_near_id];
                if (wedge == same_compare_wedge)
                {
                    map_old_to_new[vertex_near_id] = new_index;
                }
            }
            map_old_to_new[wedge_index] = new_index;
        }
    }
    wedges.swap(compressed_wedges);
    for (YMeshPolygonGroup& group : polygon_groups)
    {
        for (int polygon_index : group.polygons)
        {
            YMeshPolygon& polygon = polygons[polygon_index];
            polygon.wedge_ids[0] = map_old_to_new[polygon.wedge_ids[0]];
            polygon.wedge_ids[1] = map_old_to_new[polygon.wedge_ids[1]];
            polygon.wedge_ids[2] = map_old_to_new[polygon.wedge_ids[2]];
        }
    }

}

bool ImportedRawMesh::TellLightMapUVExist(int uv_light_map_index /*= 1*/) const
{
    for (const YMeshWedge& wedge : wedges)
    {
        for( int i=1;i< wedge.uvs.size();++i)
        {
            if(!wedge.uvs[i].IsNearlyZero())
            {
                return true;
            }
        }
    }
    return false;
}

void ImportedRawMesh::RecursiveGetTriangleGroupBySoftEdge(int triangle_id, std::set<int>& out_triangle_group, std::unordered_map<int, FlowFlagRawMesh>& around_triangle_ids, bool split_uv_seam) const
{
    if (around_triangle_ids[triangle_id].visited)
    {
        return;
    }
    around_triangle_ids[triangle_id].visited = true;
    out_triangle_group.insert(triangle_id);
    int cur_wedge_index = around_triangle_ids[triangle_id].wedge_id;
    const YMeshPolygon& polygon = polygons[triangle_id];
    int last_id[2] = { -1,-1 };
    if (polygon.wedge_ids[0] == cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[1];
        last_id[1] = polygon.wedge_ids[2];
    }
    else if (polygon.wedge_ids[1] == cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[0];
        last_id[1] = polygon.wedge_ids[2];
    }
    else if (polygon.wedge_ids[2] == cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[0];
        last_id[1] = polygon.wedge_ids[1];
    }

    const YMeshWedge& cur_wedge = wedges[cur_wedge_index];
    const YMeshWedge& wedge1 = wedges[last_id[0]];
    const YMeshWedge& wedge2 = wedges[last_id[1]];
    int edge_id01 = GetVertexPairEdge(cur_wedge.control_point_id, wedge1.control_point_id);
    assert(edge_id01 != -1);
    const YMeshEdge& edge_01 = edges[edge_id01];
    bool edge_01_soft = !edge_01.edge_hardness;
    if (split_uv_seam)
    {
        edge_01_soft = edge_01_soft && (!edge_01.is_uv_seam);
    }
    if (edge_01_soft)
    {
        int other_triangle_id = -1;
        //有可能在边界
        for (int i_sub_edge = 0; i_sub_edge < edge_01.connected_triangles.size(); ++i_sub_edge)
        {
            int triangle_same_edege = edge_01.connected_triangles[i_sub_edge];
            if (triangle_same_edege == triangle_id)
            {
                continue;
            }
            else
            {
                RecursiveGetTriangleGroupBySoftEdge(triangle_same_edege, out_triangle_group, around_triangle_ids, split_uv_seam);
            }
        }
    }


    int edge_id02 = GetVertexPairEdge(cur_wedge.control_point_id, wedge2.control_point_id);
    assert(edge_id02 != -1);
    const YMeshEdge& edge_02 = edges[edge_id02];
    bool edge_02_soft = !edge_02.edge_hardness;
    if (split_uv_seam)
    {
        edge_02_soft = edge_02_soft && (!edge_02.is_uv_seam);
    }
    if (edge_02_soft)
    {
        //有可能在边界
        for (int i_sub_edge = 0; i_sub_edge < edge_02.connected_triangles.size(); ++i_sub_edge)
        {
            int triangle_same_edege = edge_02.connected_triangles[i_sub_edge];
            if (triangle_same_edege == triangle_id)
            {
                continue;
            }
            else
            {
                RecursiveGetTriangleGroupBySoftEdge(triangle_same_edege, out_triangle_group, around_triangle_ids, split_uv_seam);
            }
        }
    }
}

void ImportedRawMesh::RemoveNearHaredEdge(int triangle_id, std::set<int>& out_triangle_group, const std::unordered_map<int, FlowFlagRawMesh>& around_triangle_ids) const
{
    int cur_wedge_index = around_triangle_ids.at(triangle_id).wedge_id;
    const YMeshPolygon& polygon = polygons[triangle_id];
    int last_id[2] = { -1,-1 };
    if (polygon.wedge_ids[0] == cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[1];
        last_id[1] = polygon.wedge_ids[2];
    }
    else if (polygon.wedge_ids[1] == cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[0];
        last_id[1] = polygon.wedge_ids[2];
    }
    else if (polygon.wedge_ids[2] == cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[0];
        last_id[1] = polygon.wedge_ids[1];
    }

    const YMeshWedge& cur_wedge = wedges[cur_wedge_index];
    const YMeshWedge& wedge1 = wedges[last_id[0]];
    const YMeshWedge& wedge2 = wedges[last_id[1]];
    int edge_id01 = GetVertexPairEdge(cur_wedge.control_point_id, wedge1.control_point_id);
    assert(edge_id01 != -1);
    const YMeshEdge& edge_01 = edges[edge_id01];
    if (edge_01.edge_hardness)
    {
        //有可能在边界
        for (int i_sub_edge = 0; i_sub_edge < edge_01.connected_triangles.size(); ++i_sub_edge)
        {
            int triangle_same_edege = edge_01.connected_triangles[i_sub_edge];
            if (triangle_same_edege == triangle_id)
            {
                continue;
            }
            else
            {
                if (out_triangle_group.count(triangle_same_edege))
                {
                    out_triangle_group.erase(triangle_same_edege);
                }
            }
        }
    }


    int edge_id02 = GetVertexPairEdge(cur_wedge.control_point_id, wedge2.control_point_id);
    assert(edge_id02 != -1);
    const YMeshEdge& edge_02 = edges[edge_id02];
    if (edge_02.edge_hardness)
    {
        //有可能在边界
        for (int i_sub_edge = 0; i_sub_edge < edge_02.connected_triangles.size(); ++i_sub_edge)
        {
            int triangle_same_edege = edge_02.connected_triangles[i_sub_edge];
            if (triangle_same_edege == triangle_id)
            {
                continue;
            }
            else
            {
                if (out_triangle_group.count(triangle_same_edege))
                {
                    out_triangle_group.erase(triangle_same_edege);
                }
            }
        }
    }
}

bool ImportedRawMesh::IsUVSeam(int edge_index) const
{
    if (edges.empty() || edge_index<0 || edge_index > edges.size())
    {
        assert(0);
        return false;
    }

    const YMeshEdge& edge = edges[edge_index];
    int cp_0 = edge.control_points_ids[0];
    int cp_1 = edge.control_points_ids[1];

    if (edge.connected_triangles.size() == 1)
    {
        return true;
    }

    int triangle_id0 = edge.connected_triangles[0];
    int triangle_id1 = edge.connected_triangles[1];

    auto contro_point_to_wedge_id = [this](int triangle_id, int control_point_id)
    {
        const YMeshPolygon& triangle = polygons[triangle_id];
        for (int wedge_id : triangle.wedge_ids)
        {
            const YMeshWedge& wedge = wedges[wedge_id];
            if (wedge.control_point_id == control_point_id)
            {
                return wedge_id;
            }
        }
        return -1;
    };

    int wedge_id_tri0_cp0 = contro_point_to_wedge_id(triangle_id0, cp_0);
    int wedge_id_tri1_cp0 = contro_point_to_wedge_id(triangle_id1, cp_0);
    int wedge_id_tri0_cp1 = contro_point_to_wedge_id(triangle_id0, cp_1);
    int wedge_id_tri1_cp1 = contro_point_to_wedge_id(triangle_id1, cp_1);

    YVector2 uv_tri0_cp0 = wedges[wedge_id_tri0_cp0].uvs[0];
    YVector2 uv_tri1_cp0 = wedges[wedge_id_tri1_cp0].uvs[0];
    YVector2 uv_tri0_cp1 = wedges[wedge_id_tri0_cp1].uvs[0];
    YVector2 uv_tri1_cp1 = wedges[wedge_id_tri1_cp1].uvs[0];

    if (uv_tri0_cp0.Equals(uv_tri1_cp0, THRESH_UV_SAME) && uv_tri0_cp1.Equals(uv_tri1_cp1, THRESH_UV_SAME))
    {
        return false;
    }
    return true;
}

float ImportedRawMesh::CalculateTriangleArea(const YVector& v0, const YVector& v1, const YVector& v2)
{
    return YVector::CrossProduct((v1 - v0), (v2 - v0)).Size() / 2.0f;
}

float ImportedRawMesh::ComputeTriangleCornerAngle(const YVector& v0, const YVector& v1, const YVector& v2)
{
    YVector E1 = (v1 - v0);
    YVector E2 = (v2 - v0);
    //Normalize both edges (unit vector) of the triangle so we get a dotProduct result that will be a valid acos input [-1, 1]
    if (!E1.Normalize() || !E2.Normalize())
    {
        //Return a null ratio if the polygon is degenerate
        return 0.0f;
    }
    float DotProduct = YVector::Dot(E1, E2);
    return YMath::Acos(DotProduct);
}

struct MikktHelper
{
    static int MikkGetNumFaces(const SMikkTSpaceContext* Context)
    {
        ImportedRawMesh* raw_mesh = (ImportedRawMesh*)(Context->m_pUserData);
        return (int)raw_mesh->polygons.size();
    }

    static int MikkGetNumVertsOfFace(const SMikkTSpaceContext* Context, const int FaceIdx)
    {
        // All of our meshes are triangles.
        //ImportedRawMesh* raw_mesh = (ImportedRawMesh*)(Context->m_pUserData);
        //return (int)raw_mesh->polygons[FaceIdx].wedge_ids.size();
        return 3;
    }

    static void MikkGetPosition(const SMikkTSpaceContext* Context, float Position[3], const int FaceIdx, const int VertIdx)
    {
        ImportedRawMesh* raw_mesh = (ImportedRawMesh*)(Context->m_pUserData);
        YVector conrol_point = raw_mesh->control_points[raw_mesh->wedges[raw_mesh->polygons[FaceIdx].wedge_ids[VertIdx]].control_point_id].position;
        Position[0] = conrol_point.x;
        Position[1] = conrol_point.y;
        Position[2] = conrol_point.z;
    }

    static void MikkGetNormal(const SMikkTSpaceContext* Context, float Normal[3], const int FaceIdx, const int VertIdx)
    {
        ImportedRawMesh* raw_mesh = (ImportedRawMesh*)(Context->m_pUserData);
        YVector normal = raw_mesh->wedges[raw_mesh->polygons[FaceIdx].wedge_ids[VertIdx]].normal;
        Normal[0] = normal.x;
        Normal[1] = normal.y;
        Normal[2] = normal.z;
    }

    static void MikkSetTSpaceBasic(const SMikkTSpaceContext* Context, const float Tangent[3], const float BitangentSign, const int FaceIdx, const int VertIdx)
    {
        ImportedRawMesh* raw_mesh = (ImportedRawMesh*)(Context->m_pUserData);
        YVector& tangent_in_mesh = raw_mesh->wedges[raw_mesh->polygons[FaceIdx].wedge_ids[VertIdx]].tangent;
        float& bitangent_sign_in_mesh = raw_mesh->wedges[raw_mesh->polygons[FaceIdx].wedge_ids[VertIdx]].binormal_sign;
        tangent_in_mesh = YVector(Tangent[0], Tangent[1], Tangent[2]);
        bitangent_sign_in_mesh = -BitangentSign;
    }

    static void MikkGetTexCoord(const SMikkTSpaceContext* Context, float UV[2], const int FaceIdx, const int VertIdx)
    {
        ImportedRawMesh* raw_mesh = (ImportedRawMesh*)(Context->m_pUserData);
        YVector2 uv = raw_mesh->wedges[raw_mesh->polygons[FaceIdx].wedge_ids[VertIdx]].uvs[0];
        UV[0] = uv.x;
        UV[1] = uv.y;
    }
};
void ImportedRawMesh::ComputeTangentSpaceMikktMethod(bool ignore_degenerate_triangle /*= true*/)
{
    // we can use mikktspace to calculate the tangents
    SMikkTSpaceInterface MikkTInterface;
    MikkTInterface.m_getNormal = MikktHelper::MikkGetNormal;
    MikkTInterface.m_getNumFaces = MikktHelper::MikkGetNumFaces;
    MikkTInterface.m_getNumVerticesOfFace = MikktHelper::MikkGetNumVertsOfFace;
    MikkTInterface.m_getPosition = MikktHelper::MikkGetPosition;
    MikkTInterface.m_getTexCoord = MikktHelper::MikkGetTexCoord;
    MikkTInterface.m_setTSpaceBasic = MikktHelper::MikkSetTSpaceBasic;
    MikkTInterface.m_setTSpace = nullptr;
    SMikkTSpaceContext MikkTContext;
    MikkTContext.m_pInterface = &MikkTInterface;
    MikkTContext.m_pUserData = (void*)(this);
    MikkTContext.m_bIgnoreDegenerates = ignore_degenerate_triangle;
    genTangSpaceDefault(&MikkTContext);
}

std::set<int> ImportedRawMesh::GetTriangleGroupBySoftEdge(int wedge_index, bool split_uv_seam) const
{
    const YMeshWedge& wedge = wedges[wedge_index];
    int triangle_id = wedge.connected_triangles[0];
    const YMeshPolygon& start_triangle = polygons[triangle_id];

    const YMeshControlPoint& control_point_search = control_points[wedge.control_point_id];
    std::unordered_map<int, FlowFlagRawMesh> around_triangle_ids;
    for (int wedge_id : control_point_search.wedge_ids)
    {
        FlowFlagRawMesh tmp_flag;
        tmp_flag.triangle_id = wedges[wedge_id].connected_triangles[0];
        tmp_flag.wedge_id = wedge_id;
        tmp_flag.visited = false;
        around_triangle_ids[tmp_flag.triangle_id] = tmp_flag;
    }
    std::set<int> first_group;
    RecursiveGetTriangleGroupBySoftEdge(triangle_id, first_group, around_triangle_ids, split_uv_seam);
    RemoveNearHaredEdge(triangle_id, first_group, around_triangle_ids);
    return first_group;

}


std::set<int> ImportedRawMesh::FilterTriangleGroupBySameTangentSign(int wedge_index, const std::set<int>& connected_triangles)
{
    YMeshWedge& wedge = wedges[wedge_index];
    int triangle_id = wedge.connected_triangles[0];
    YMeshPolygon& start_triangle = polygons[triangle_id];

    std::set<int> same_tangent_sign;
    for (int compare_id : connected_triangles)
    {
        YMeshPolygon& compare_triangle = polygons[compare_id];
        if (start_triangle.face_bitanget_sign == compare_triangle.face_bitanget_sign)
        {
            same_tangent_sign.insert(compare_id);
        }
    }

    return same_tangent_sign;
}

void ImportedRawMesh::ComputeWedgeNormalAndTangent(NormalCaculateMethod normal_method, TangentMethod tangent_method, bool ignore_degenerate_triangle)
{
    bool tangent_valid = false;
    bool btn_valid = true;

    for (YMeshWedge& wedge : wedges)
    {
        if (!wedge.tangent.Equals(YVector(1.0, 0.0, 0.0)))
        {
            tangent_valid = true;
            break;
        }
    }
    if (tangent_valid)
    {
        for (YMeshWedge& wedge : wedges)
        {
            if (wedge.tangent.IsNearlyZero() ||
                wedge.bitangent.IsNearlyZero() ||
                wedge.normal.IsNearlyZero())
            {
                btn_valid = false;
                break;
            }

            YVector test_normal = YVector::CrossProduct(wedge.tangent, wedge.bitangent).GetSafeNormal();
            float samilirity = YMath::Abs(YVector::Dot(test_normal, wedge.normal));
            if (!YMath::IsNearlyEqual(samilirity, 1.0f, SMALL_NUMBER))
            {
                btn_valid = false;
                break;
            }
        }
    }
    else
    {
        btn_valid = false;
    }

    if (tangent_valid && btn_valid && normal_method == ImportNormalAndTangnet)
    {
        return;
    }

    if (!tangent_valid && normal_method == ImportNormalAndTangnet)
    {
        normal_method = ImportNormal;
    }

    ComputeUVSeam();
    ComputeFaceNormalAndTangent(normal_method, tangent_method);
    for (int wedge_index = 0; wedge_index < wedges.size(); ++wedge_index)
    {
        YMeshWedge& cur_wedge = wedges[wedge_index];
        std::set<int> connect_normal_triangles = GetTriangleGroupBySoftEdge(wedge_index, false);
        std::set<int> connect_tangent_triangle = GetTriangleGroupBySoftEdge(wedge_index, true);
        connect_tangent_triangle = FilterTriangleGroupBySameTangentSign(wedge_index, connect_tangent_triangle);
        YVector normal = YVector::zero_vector;
        YVector tangent = YVector::zero_vector;
        YVector bitangent = YVector::zero_vector;
        for (int connect_triangle_id : connect_normal_triangles)
        {
            YMeshPolygon& polygon = polygons[connect_triangle_id];
            if ((!polygon.face_normal.IsNearlyZero(SMALL_NUMBER)) && !(polygon.face_normal.ContainsNaN()))
            {
                //normal = normal + polygon.normal*polygon.aera * cur_wedge.corner_angle;
                if (normal_method != ImportNormal)
                {
                    normal = normal + polygon.face_normal * cur_wedge.corner_angle;
                }
            }
            if (connect_tangent_triangle.count(connect_triangle_id))
            {
                if ((!polygon.face_tangent.IsNearlyZero(SMALL_NUMBER)) && !(polygon.face_tangent.ContainsNaN()))
                {
                    tangent = tangent + polygon.face_tangent;
                }

                if ((!polygon.face_bitangent.IsNearlyZero(SMALL_NUMBER)) && !(polygon.face_bitangent.ContainsNaN()))
                {
                    bitangent = bitangent + polygon.face_bitangent;
                }
            }
        }

        if (normal_method == Caculate)
        {
            normal = normal.GetSafeNormal();
        }
        else
        {
            normal = cur_wedge.normal.GetSafeNormal();
        }
        tangent = tangent.GetSafeNormal();
        bitangent = bitangent.GetSafeNormal();
        const YMeshPolygon& polygon = polygons[wedges[wedge_index].connected_triangles[0]];
        if (normal.IsNearlyZero(SMALL_NUMBER))
        {
            normal = polygon.face_normal;
        }
        {
            if (tangent.IsNearlyZero(KINDA_SMALL_NUMBER))
            {
                tangent = polygon.face_tangent;
            }
        }


        if (!normal.IsNearlyZero(SMALL_NUMBER) && !tangent.IsNearlyZero(SMALL_NUMBER))
        {
            bitangent = YVector::CrossProduct(normal, tangent).GetSafeNormal() * polygon.face_bitanget_sign;
        }
        if (bitangent.IsNearlyZero())
        {
            bitangent = polygon.face_bitangent;
        }
        YVector::CreateGramSchmidtOrthogonalization(tangent, bitangent, normal);
        float determinant = YMatrix(YVector4(tangent, 0.0), YVector4(bitangent, 0.0), YVector4(normal, 0.0), YVector4(0.0, 0.0, 0.0, 1.0)).Determinant();
        cur_wedge.normal = normal;
        cur_wedge.tangent = tangent;
        cur_wedge.bitangent = bitangent;
        cur_wedge.binormal_sign = determinant < 0 ? -1.0 : 1.0;
    }

    if (tangent_method == Mikkt)
    {
        ComputeTangentSpaceMikktMethod(ignore_degenerate_triangle);
        for (int wedge_index = 0; wedge_index < wedges.size(); ++wedge_index)
        {
            YMeshWedge& cur_wedge = wedges[wedge_index];
            cur_wedge.bitangent = YVector::CrossProduct(cur_wedge.normal, cur_wedge.tangent).GetSafeNormal() * cur_wedge.binormal_sign;
        }
    }
}
const int ImportedRawMesh::version = 1;
YArchive& operator<<(YArchive& mem_file, ImportedRawMesh& imported_raw_mesh)
{
    int version_value = ImportedRawMesh::version;
    mem_file << version_value;
    mem_file << imported_raw_mesh.LOD_index;
    mem_file << imported_raw_mesh.name;
    mem_file << imported_raw_mesh.control_points;
    mem_file << imported_raw_mesh.edges;
    mem_file << imported_raw_mesh.wedges;
    mem_file << imported_raw_mesh.polygons;
    mem_file << imported_raw_mesh.polygon_groups;
    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YMeshControlPoint& control_point)
{
    mem_file << control_point.position;
    mem_file << control_point.wedge_ids;
    mem_file << control_point.edge_ids;
    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YMeshEdge& mesh_edge)
{
    mem_file << mesh_edge.control_points_ids[0];
    mem_file << mesh_edge.control_points_ids[1];
    mem_file << mesh_edge.connected_triangles;
    mem_file << mesh_edge.edge_hardness;
    mem_file << mesh_edge.edge_crease_sharpness;
    mem_file << mesh_edge.is_uv_seam;

    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YMeshPolygon& mesh_polygon)
{
    mem_file << mesh_polygon.polygon_group_id;
    mem_file << mesh_polygon.wedge_ids;
    mem_file << mesh_polygon.face_normal;
    mem_file << mesh_polygon.face_tangent;
    mem_file << mesh_polygon.face_bitangent;
    mem_file << mesh_polygon.face_bitanget_sign;
    mem_file << mesh_polygon.face_aera;
    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YMeshPolygonGroup& mesh_polygon_group)
{
    mem_file << mesh_polygon_group.polygons;
    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YMeshWedge& wedge)
{
    mem_file << wedge.control_point_id;
    mem_file << wedge.position;
    mem_file << wedge.normal;
    mem_file << wedge.tangent;
    mem_file << wedge.bitangent;
    mem_file << wedge.binormal_sign;
    mem_file << wedge.color;
    mem_file << wedge.uvs;
    mem_file << wedge.connected_triangles;

    return mem_file;
}

