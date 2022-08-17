#include "Engine/YRawMesh.h"
#include "Math/YVector.h"
#include <cassert>
#include "Engine/YArchive.h"
#include <unordered_set>
#include <algorithm>
#include "Utility/YStringFormat.h"
#include <set>
#include "Math/NumericLimits.h"
#include "Utility/mikktspace.h"
#include "Math/YColor.h"
#include "Utility/OverlappingCorners.h"
#include "Utility/NvTriStrip.h"
#include "Utility/nvtess.h"
YLODMesh::YLODMesh()
{

}

int YLODMesh::GetVertexPairEdge(int vertex_id0, int vertex_id1)
{
    //verte
    std::vector<int>& connect_edges = vertex_position[vertex_id0].edge_ids;
    for (int edge : connect_edges)
    {
        int vertex_maybe_0 = edges[edge].control_points_ids[0];
        int vertex_maybe_1 = edges[edge].control_points_ids[1];
        if ((vertex_maybe_0 == vertex_id0 && vertex_maybe_1 == vertex_id1) || (vertex_maybe_0 == vertex_id1 && vertex_maybe_1 == vertex_id0))
        {
            return edge;
        }
    }
    return INVALID_ID;
}

int YLODMesh::CreateEdge(int vertex_id_0, int vertex_id_1)
{
#if defined(DEBUG) || defined(_DEBUG)
    int exist_id = GetVertexPairEdge(vertex_id_0, vertex_id_1);
    assert(exist_id == INVALID_ID);
#endif
    YMeshEdge tmp_edge;
    tmp_edge.control_points_ids[0] = vertex_id_0;
    tmp_edge.control_points_ids[1] = vertex_id_1;
    int edge_id = (int)edges.size();
    edges.push_back(tmp_edge);
    vertex_position[vertex_id_0].edge_ids.push_back(edge_id);
    vertex_position[vertex_id_1].edge_ids.push_back(edge_id);
    return edge_id;
}

int YLODMesh::CreatePolygon(int polygon_group_id, std::vector<int> vertex_ins_ids, std::vector<int>& out_edges)
{
    out_edges.clear();
    // create triangle
    int polygon_id = (int)polygons.size();
    polygons.push_back(YMeshPolygon());

    YMeshPolygon& tmp_polygon = polygons[polygon_id];
    // polygon_group, both reference
    tmp_polygon.polygon_group_id = polygon_group_id;
    tmp_polygon.wedge_ids = vertex_ins_ids;
    polygon_groups[polygon_group_id].polygons.push_back(polygon_id);

    std::vector<int> vertex_ids;
    vertex_ids.reserve(vertex_ins_ids.size());

    //only support triangle,UE support polygon
    for (int i = 0; i < vertex_ins_ids.size(); ++i)
    {
        vertex_instances[vertex_ins_ids[i]].AddTriangleID(polygon_id);
        int i_next = (i + 1) % ((int)vertex_ins_ids.size());
        int vertex_id = vertex_instances[vertex_ins_ids[i]].control_point_id;
        int vertex_next_id = vertex_instances[vertex_ins_ids[i_next]].control_point_id;
        int edge_idex = GetVertexPairEdge(vertex_id, vertex_next_id);
        //create edges
        if (edge_idex == INVALID_ID)
        {
            edge_idex = CreateEdge(vertex_id, vertex_next_id);
            out_edges.push_back(edge_idex);
        }
        edges[edge_idex].AddTriangleID(polygon_id);
    }

    return polygon_id;
}



void YLODMesh::ComputeAABB()
{
    for (YMeshControlPoint& v : vertex_position)
    {
        aabb += v.position;
    }
}

YMeshEdge::YMeshEdge()
{
    control_points_ids[0] = INVALID_ID;
    control_points_ids[1] = INVALID_ID;
    edge_hardness = false;
    edge_crease_sharpness = 0.0;
}

void YMeshEdge::AddTriangleID(int triangle_id)
{
    auto find_reuslt = std::find(connected_triangles.begin(), connected_triangles.end(), triangle_id);
    if (find_reuslt == connected_triangles.end())
    {
        connected_triangles.push_back(triangle_id);
    }
}

YMeshVertexWedge::YMeshVertexWedge()
{
    uvs.resize(MAX_MESH_TEXTURE_COORDS, YVector2(0.0, 0.0));
}

void YMeshVertexWedge::AddTriangleID(int triangle_id)
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

YArchive& operator<<(YArchive& mem_file, YLODMesh& lod_mesh)
{
    mem_file << lod_mesh.LOD_index;
    mem_file << lod_mesh.sub_meshes;
    mem_file << lod_mesh.vertex_position;
    mem_file << lod_mesh.vertex_instances;
    mem_file << lod_mesh.polygons;
    mem_file << lod_mesh.edges;
    mem_file << lod_mesh.polygon_groups;
    //mem_file << lod_mesh.polygon_group_to_material_name;
    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YRawMesh& raw_mesh)
{
    mem_file << raw_mesh.mesh_name;
    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YMeshEdge& mesh_edge)
{
    mem_file << mesh_edge.control_points_ids[0];
    mem_file << mesh_edge.control_points_ids[1];
    mem_file << mesh_edge.connected_triangles;
    mem_file << mesh_edge.edge_hardness;
    mem_file << mesh_edge.edge_crease_sharpness;

    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YMeshPolygonGroup& mesh_polygon_group)
{
    mem_file << mesh_polygon_group.polygons;
    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YMeshPolygon& mesh_polygon)
{
    mem_file << mesh_polygon.polygon_group_id;
    mem_file << mesh_polygon.wedge_ids;
    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YMeshVertexWedge& mesh_vertex_instance)
{
    mem_file << mesh_vertex_instance.control_point_id;
    mem_file << mesh_vertex_instance.connected_triangles;
    mem_file << mesh_vertex_instance.normal;
    mem_file << mesh_vertex_instance.tangent;
    mem_file << mesh_vertex_instance.binormal_sign;
    mem_file << mesh_vertex_instance.color;
    mem_file << mesh_vertex_instance.uvs;

    return mem_file;
}

YArchive& operator<<(YArchive& mem_file, YMeshControlPoint& mesh_vertex)
{
    mem_file << mesh_vertex.wedge_ids;
    mem_file << mesh_vertex.edge_ids;
    mem_file << mesh_vertex.position;
    return mem_file;
}


ImportedRawMesh::ImportedRawMesh()
    :LOD_index(INVALID_ID) {

}

int ImportedRawMesh::GetVertexPairEdge(int vertex_id0, int vertex_id1) const
{
    //verte
    const std::vector<int>& connect_edges = control_points[vertex_id0].edge_ids;
    for (int edge : connect_edges)
    {
        int vertex_maybe_0 = edges[edge].control_points_ids[0];
        int vertex_maybe_1 = edges[edge].control_points_ids[1];
        if ((vertex_maybe_0 == vertex_id0 && vertex_maybe_1 == vertex_id1) || (vertex_maybe_0 == vertex_id1 && vertex_maybe_1 == vertex_id0))
        {
            return edge;
        }
    }
    return INVALID_ID;
}

int ImportedRawMesh::CreateEdge(int vertex_id_0, int vertex_id_1)
{
#if defined(DEBUG) || defined(_DEBUG)
    int exist_id = GetVertexPairEdge(vertex_id_0, vertex_id_1);
    assert(exist_id == INVALID_ID);
#endif
    YMeshEdge tmp_edge;
    tmp_edge.control_points_ids[0] = vertex_id_0;
    tmp_edge.control_points_ids[1] = vertex_id_1;
    int edge_id = (int)edges.size();
    edges.push_back(tmp_edge);
    control_points[vertex_id_0].edge_ids.push_back(edge_id);
    control_points[vertex_id_1].edge_ids.push_back(edge_id);
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
    YMeshVertexWedge& wedge0 = wedges[tmp_polygon.wedge_ids[0]];
    YMeshVertexWedge& wedge1 = wedges[tmp_polygon.wedge_ids[1]];
    YMeshVertexWedge& wedge2 = wedges[tmp_polygon.wedge_ids[2]];
    YVector position0 = control_points[wedge0.control_point_id].position;
    YVector position1 = control_points[wedge1.control_point_id].position;
    YVector position2 = control_points[wedge2.control_point_id].position;
    tmp_polygon.aera = CalculateTriangleArea(position0, position1, position2);
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

    for (YMeshVertexWedge& wedge : wedges)
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
    for (YMeshVertexWedge& other_wedge : other.wedges)
    {
        wedges.push_back(other_wedge);
        YMeshVertexWedge& new_wedge = wedges.back();
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

void ImportedRawMesh::ComputeTriangleNormalAndTangent(NormalCaculateMethod normal_method, TangentMethod tangent_method)
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
                WARNING_INFO(StringFormat("triangle %d has bad NTB", polygon_id));
            }
            polygon.normal = Normal.GetSafeNormal();
            polygon.tangent = Tangent.GetSafeNormal();
            polygon.bitangent = Binormal.GetSafeNormal();

            float determinant = YMatrix(YVector4(polygon.tangent, 0.0), YVector4(polygon.bitangent, 0.0), YVector4(polygon.normal, 0.0), YVector4(0.0, 0.0, 0.0, 1.0)).Determinant();
            polygon.bitanget_sign = determinant < 0 ? -1.0 : 1.0;
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
    std::unordered_map<std::shared_ptr<YFbxMaterial>, int > map_material_to_new_group_id;
    std::vector< YMeshPolygonGroup> new_polygon_groups;
    std::vector< std::shared_ptr<YFbxMaterial>> new_polygon_group_to_material;
    for (int polygon_group_index = 0; polygon_group_index < polygon_group_to_material.size(); ++polygon_group_index)
    {
        std::shared_ptr<YFbxMaterial> material = polygon_group_to_material[polygon_group_index];
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

void ImportedRawMesh::RecursiveFindGroup(int triangle_id, std::set<int>& out_triangle_group, std::unordered_map<int, FlowFlagRawMesh>& around_triangle_ids, bool split_uv_seam)
{
    if (around_triangle_ids[triangle_id].visited)
    {
        return;
    }
    around_triangle_ids[triangle_id].visited = true;
    out_triangle_group.insert(triangle_id);
    int cur_wedge_index = around_triangle_ids[triangle_id].wedge_id;
    YMeshPolygon& polygon = polygons[triangle_id];
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
    else if (polygon.wedge_ids[2] = cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[0];
        last_id[1] = polygon.wedge_ids[1];
    }

    YMeshVertexWedge& cur_wedge = wedges[cur_wedge_index];
    YMeshVertexWedge& wedge1 = wedges[last_id[0]];
    YMeshVertexWedge& wedge2 = wedges[last_id[1]];
    int edge_id01 = GetVertexPairEdge(cur_wedge.control_point_id, wedge1.control_point_id);
    assert(edge_id01 != -1);
    YMeshEdge& edge_01 = edges[edge_id01];
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
                RecursiveFindGroup(triangle_same_edege, out_triangle_group, around_triangle_ids, split_uv_seam);
            }
        }
    }


    int edge_id02 = GetVertexPairEdge(cur_wedge.control_point_id, wedge2.control_point_id);
    assert(edge_id02 != -1);
    YMeshEdge& edge_02 = edges[edge_id02];
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
                RecursiveFindGroup(triangle_same_edege, out_triangle_group, around_triangle_ids, split_uv_seam);
            }
        }
    }
}

void ImportedRawMesh::RemoveNearHaredEdge(int triangle_id, std::set<int>& out_triangle_group, const std::unordered_map<int, FlowFlagRawMesh>& around_triangle_ids)
{
    int cur_wedge_index = around_triangle_ids.at(triangle_id).wedge_id;
    YMeshPolygon& polygon = polygons[triangle_id];
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
    else if (polygon.wedge_ids[2] = cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[0];
        last_id[1] = polygon.wedge_ids[1];
    }

    YMeshVertexWedge& cur_wedge = wedges[cur_wedge_index];
    YMeshVertexWedge& wedge1 = wedges[last_id[0]];
    YMeshVertexWedge& wedge2 = wedges[last_id[1]];
    int edge_id01 = GetVertexPairEdge(cur_wedge.control_point_id, wedge1.control_point_id);
    assert(edge_id01 != -1);
    YMeshEdge& edge_01 = edges[edge_id01];
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
    YMeshEdge& edge_02 = edges[edge_id02];
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

bool ImportedRawMesh::IsUVSeam(int edge_index)
{
    if (edges.empty() || edge_index<0 || edge_index > edges.size())
    {
        assert(0);
        return false;
    }

    YMeshEdge& edge = edges[edge_index];
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
        YMeshPolygon& triangle = polygons[triangle_id];
        for (int wedge_id : triangle.wedge_ids)
        {
            YMeshVertexWedge& wedge = wedges[wedge_id];
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

std::set<int> ImportedRawMesh::GetSplitTriangleGroupBySoftEdge(int wedge_index, bool split_uv_seam)
{
    YMeshVertexWedge& wedge = wedges[wedge_index];
    int triangle_id = wedge.connected_triangles[0];
    YMeshPolygon& start_triangle = polygons[triangle_id];

    YMeshControlPoint& control_point_search = control_points[wedge.control_point_id];
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
    RecursiveFindGroup(triangle_id, first_group, around_triangle_ids, split_uv_seam);
    RemoveNearHaredEdge(triangle_id, first_group, around_triangle_ids);
    return first_group;

}


std::set<int> ImportedRawMesh::GetSplitTriangleGroupBySoftEdgeSameTangentSign(int wedge_index, const std::set<int>& connected_triangles)
{
    YMeshVertexWedge& wedge = wedges[wedge_index];
    int triangle_id = wedge.connected_triangles[0];
    YMeshPolygon& start_triangle = polygons[triangle_id];

    std::set<int> same_tangent_sign;
    for (int compare_id : connected_triangles)
    {
        YMeshPolygon& compare_triangle = polygons[compare_id];
        if (start_triangle.bitanget_sign == compare_triangle.bitanget_sign)
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

    for (YMeshVertexWedge& wedge : wedges)
    {
        if (!wedge.tangent.Equals(YVector(1.0, 0.0, 0.0)))
        {
            tangent_valid = true;
            break;
        }
    }
    if (tangent_valid)
    {
        for (YMeshVertexWedge& wedge : wedges)
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
    ComputeTriangleNormalAndTangent(normal_method, tangent_method);
    for (int wedge_index = 0; wedge_index < wedges.size(); ++wedge_index)
    {
        YMeshVertexWedge& cur_wedge = wedges[wedge_index];
        std::set<int> connect_normal_triangles = GetSplitTriangleGroupBySoftEdge(wedge_index, false);
        std::set<int> connect_tangent_triangle = GetSplitTriangleGroupBySoftEdge(wedge_index, true);
        connect_tangent_triangle = GetSplitTriangleGroupBySoftEdgeSameTangentSign(wedge_index, connect_tangent_triangle);
        YVector normal = YVector::zero_vector;
        YVector tangent = YVector::zero_vector;
        YVector bitangent = YVector::zero_vector;
        for (int connect_triangle_id : connect_normal_triangles)
        {
            YMeshPolygon& polygon = polygons[connect_triangle_id];
            if ((!polygon.normal.IsNearlyZero(SMALL_NUMBER)) && !(polygon.normal.ContainsNaN()))
            {
                //normal = normal + polygon.normal*polygon.aera * cur_wedge.corner_angle;
                if (normal_method != ImportNormal)
                {
                    normal = normal + polygon.normal * cur_wedge.corner_angle;
                }
            }
            if (connect_tangent_triangle.count(connect_triangle_id))
            {
                if ((!polygon.tangent.IsNearlyZero(SMALL_NUMBER)) && !(polygon.tangent.ContainsNaN()))
                {
                    tangent = tangent + polygon.tangent;
                }

                if ((!polygon.bitangent.IsNearlyZero(SMALL_NUMBER)) && !(polygon.bitangent.ContainsNaN()))
                {
                    bitangent = bitangent + polygon.bitangent;
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
            normal = polygon.normal;
        }
        {
            if (tangent.IsNearlyZero(KINDA_SMALL_NUMBER))
            {
                tangent = polygon.tangent;
            }
        }


        if (!normal.IsNearlyZero(SMALL_NUMBER) && !tangent.IsNearlyZero(SMALL_NUMBER))
        {
            bitangent = YVector::CrossProduct(normal, tangent).GetSafeNormal() * polygon.bitanget_sign;
        }
        if (bitangent.IsNearlyZero())
        {
            bitangent = polygon.bitangent;
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
            YMeshVertexWedge& cur_wedge = wedges[wedge_index];
            cur_wedge.bitangent = YVector::CrossProduct(cur_wedge.normal, cur_wedge.tangent).GetSafeNormal() * cur_wedge.binormal_sign;
        }
    }
}

PostProcessRenderMesh::PostProcessRenderMesh(ImportedRawMesh* raw_mesh)
    :raw_mesh_(raw_mesh)
{
    int wedege_count = (int)raw_mesh_->wedges.size();
    vertex_data_cache.reserve(wedege_count);
    section_indices.reserve(wedege_count);

}

void PostProcessRenderMesh::PostProcessPipeline()
{
    CompressVertex();
    OptimizeIndices();
    BuildStaticAdjacencyIndexBuffer();
    BuildInverseIndices();
    BuildDepthOnlyIndices();
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
                const YMeshVertexWedge& wedge = raw_mesh_->wedges[triangle.wedge_ids[i]];
                FullStaticVertexData tmp;
                tmp.position = wedge.position;
                tmp.normal = wedge.normal;
                tmp.tangent = YVector4(wedge.tangent, wedge.binormal_sign);
                tmp.uv0 = wedge.uvs[0];
                //todo 
                tmp.uv1 = wedge.uvs[0];
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

namespace NvTriStripHelper
{
    /**
    * Converts 16 bit indices to 32 bit prior to passing them into the real GenerateStrips util method
    */
    //void GenerateStrips(
    //    const uint8* Indices,
    //    bool Is32Bit,
    //    const uint32 NumIndices,
    //    PrimitiveGroup** PrimGroups,
    //    uint32* NumGroups
    //)
    //{
    //    if (Is32Bit)
    //    {
    //        GenerateStrips((uint32*)Indices, NumIndices, PrimGroups, NumGroups);
    //    }
    //    else
    //    {
    //        // convert to 32 bit
    //        uint32 Idx;
    //        TArray<uint32> NewIndices;
    //        NewIndices.AddUninitialized(NumIndices);
    //        for (Idx = 0; Idx < NumIndices; ++Idx)
    //        {
    //            NewIndices[Idx] = ((uint16*)Indices)[Idx];
    //        }
    //        GenerateStrips(NewIndices.GetData(), NumIndices, PrimGroups, NumGroups);
    //    }

    //}

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
        Indices.resize(PrimitiveGroups->numIndices,-1);

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
void PostProcessRenderMesh::OptimizeIndices()
{
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
        indices_count += per_section.size();
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
                int32 index_new = vertex_data_cache.size();
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
}


class FStaticMeshNvRenderBuffer : public nv::RenderBuffer
{
public:
    FStaticMeshNvRenderBuffer(PostProcessRenderMesh* in_post_process_render_mesh,int in_section_index);
    /** Retrieve the position and first texture coordinate of the specified index. */
    virtual nv::Vertex getVertex(unsigned int Index) const;

private:
    /** Copying is forbidden. */
    FStaticMeshNvRenderBuffer(const FStaticMeshNvRenderBuffer&);
    FStaticMeshNvRenderBuffer& operator=(const FStaticMeshNvRenderBuffer&);
    PostProcessRenderMesh* post_process_render_mesh_;
    int section_index_;
};

FStaticMeshNvRenderBuffer::FStaticMeshNvRenderBuffer(PostProcessRenderMesh* in_post_process_render_mesh,int in_secton_index)
:post_process_render_mesh_(in_post_process_render_mesh), section_index_(in_secton_index){
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

    const YVector2 UV = post_process_render_mesh_->vertex_data_cache[Index].uv0;
    Vertex.uv.x = UV.x;
    Vertex.uv.y = UV.y;
 
    return Vertex;
}


void PostProcessRenderMesh::BuildStaticAdjacencyIndexBuffer()
{
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
}


void PostProcessRenderMesh::BuildInverseIndices()
{
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
}

void PostProcessRenderMesh::BuildDepthOnlyIndices()
{
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
    YOverlappingCorners  acc_overlap_finding(position_to_compare, indices_to_compare, THRESH_POINTS_ARE_SAME*4.0);

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
                if (data.position.Equals(new_vertex_data.position,THRESH_POINTS_ARE_SAME))
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
}

void PostProcessRenderMesh::BuildDepthOnlyInverseIndices()
{
    depth_only_reversed_indices.clear();
    depth_only_reversed_indices.resize(depth_only_indices.size());
    for (int i = 0; i < depth_only_indices.size(); ++i)
    {
        std::vector<uint32>& per_sec = depth_only_indices[i];
        for (int j = 0; j < per_sec.size() / 3; ++j)
        {
            reversed_indices[i].push_back(per_sec[j * 3 + 2]);
            reversed_indices[i].push_back(per_sec[j * 3 + 1]);
            reversed_indices[i].push_back(per_sec[j * 3 + 0]);
        }
    }
}

std::unique_ptr< HiSttaticVertexData> PostProcessRenderMesh::GenerateHiStaticVertexData()
{
    PostProcessPipeline();
    std::unique_ptr<HiSttaticVertexData> render_data = std::make_unique<HiSttaticVertexData>();
    render_data->position.reserve(vertex_data_cache.size());
    render_data->vertex_infos.reserve(vertex_data_cache.size());
    for (FullStaticVertexData& full_vertex_data : vertex_data_cache)
    {
        render_data->position.push_back(full_vertex_data.position);
        HiSttaticVertexData::HiVertexInfo tmp;
        tmp.normal = YVector4(full_vertex_data.normal, 0.0);
        tmp.tangent = full_vertex_data.tangent;
        tmp.uv0 = full_vertex_data.uv0;
        tmp.uv1 = full_vertex_data.uv1;
        tmp.color = FLinearColor(full_vertex_data.color).ToFColor(false).AlignmentDummy;
        render_data->vertex_infos.push_back(tmp);
    }

    render_data->GenerateIndexBuffers(section_indices);
    return render_data;
}

std::unique_ptr< MediumStaticVertexData> PostProcessRenderMesh::GenerateMediumStaticVertexData()
{
    PostProcessPipeline();
    std::unique_ptr<MediumStaticVertexData> render_data = std::make_unique<MediumStaticVertexData>();
    render_data->position.reserve(vertex_data_cache.size());
    render_data->vertex_infos.reserve(vertex_data_cache.size());
    for (FullStaticVertexData& full_vertex_data : vertex_data_cache)
    {
        render_data->position.push_back(full_vertex_data.position);
        MediumStaticVertexData::MediumVertexInfo tmp;
        tmp.normal = YVector4(full_vertex_data.normal, 0.0);
        tmp.tangent = full_vertex_data.tangent;
        tmp.uv0 = full_vertex_data.uv0;
        tmp.uv1 = full_vertex_data.uv1;
        render_data->vertex_infos.push_back(tmp);
    }

    render_data->GenerateIndexBuffers(section_indices);
    return render_data;
}

void StaticVertexRenderData::GenerateIndexBuffers(const std::vector<std::vector<uint32>>& section_indices)
{
    int section_offset = 0;
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
        triangle_count += indices_per_sec.size();
    }
    if (use_32_indices)
    {
        indices_32.reserve(triangle_count * 3);
    }
    else
    {
        indices_16.reserve(triangle_count * 3);
    }

    sections.resize(section_indices.size());

    for (int section_index = 0; section_index < section_indices.size(); ++section_index)
    {
        sections[section_index].offset = section_offset;
        sections[section_index].triangle_count = section_indices[section_index].size() / 3;

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
            section_offset++;
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

uint32 HiSttaticVertexData::GetVertexInfoSize()
{
    return sizeof(HiSttaticVertexData::HiVertexInfo) * vertex_infos.size();
}

void* HiSttaticVertexData::GetVertexInfoData()
{
    return vertex_infos.data();
}

uint32 MediumStaticVertexData::GetVertexInfoSize()
{
    return sizeof(MediumStaticVertexData::MediumVertexInfo) * vertex_infos.size();
}

void* MediumStaticVertexData::GetVertexInfoData()
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
