#include "Engine/YRawMesh.h"
#include "Math/YVector.h"
#include <cassert>
#include "Engine/YArchive.h"
#include <unordered_set>
#include <algorithm>
#include "Utility/YStringFormat.h"
#include <set>
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

YArchive& operator<<(YArchive& mem_file,  YMeshEdge& mesh_edge)
{
	mem_file << mesh_edge.control_points_ids[0];
	mem_file << mesh_edge.control_points_ids[1];
	mem_file << mesh_edge.connected_triangles;
	mem_file << mesh_edge.edge_hardness;
	mem_file << mesh_edge.edge_crease_sharpness;

	return mem_file;
}

YArchive& operator<<(YArchive& mem_file,  YMeshPolygonGroup& mesh_polygon_group)
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

YArchive& operator<<(YArchive& mem_file,  YMeshVertexWedge& mesh_vertex_instance)
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

YArchive& operator<<(YArchive& mem_file,  YMeshControlPoint& mesh_vertex)
{
	mem_file << mesh_vertex.wedge_ids;
	mem_file << mesh_vertex.edge_ids;
	mem_file << mesh_vertex.position;
	return mem_file;
}


ImportedRawMesh::ImportedRawMesh()
:LOD_index(INVALID_ID){

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
            int tmp_control_points[3];
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


   bool result =  func_in_diff_set(2);
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

    WARNING_INFO(StringFormat("remove mesh's %s degenerate triangle, before is %d , after is %d, smaller %d",name.c_str(),before_size,after_size,before_size-after_size));
}


bool ImportedRawMesh::Valid() const
{
    for(int polygon_group_id = 0;polygon_group_id<(int)polygon_groups.size();++polygon_group_id)
    {
        if (polygon_group_to_material.count(polygon_group_id) == 0)
        {
            WARNING_INFO("Imported RawMesh Valid: polygon_group do not in polygon_group_to_materials");
            return false;
        }

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
    for(int control_point_id =0; control_point_id<(int)control_points.size();++control_point_id)
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
    for(int other_polygon_group_index=0; other_polygon_group_index< other.polygon_groups.size();++other_polygon_group_index)
    { 
        YMeshPolygonGroup& other_polygon_group = other.polygon_groups[other_polygon_group_index];
        const int new_polygon_group_id = polygon_groups.size();
        polygon_groups.push_back(other_polygon_group);
        YMeshPolygonGroup& new_ploygon_group = polygon_groups.back();
        for (int& polygon_id : new_ploygon_group.polygons)
        {
            polygon_id += self_polygon_size;
        }
        // polygon_group_to_material
        polygon_group_to_material[new_polygon_group_id] = other.polygon_group_to_material[other_polygon_group_index];
    }

    // edge_verttex_id_to_edge_id
    for (int new_edge_index = self_edge_size; new_edge_index < edges.size(); ++new_edge_index)
    {
        YMeshEdge& edge = edges[new_edge_index];
        uint64_t compacted_key = ((uint64_t)edge.control_points_ids[0]) << 32 | ((uint64_t)edge.control_points_ids[1]);
        uint64_t compacted_key2 = ((uint64_t)edge.control_points_ids[1]) << 32 | ((uint64_t)edge.control_points_ids[0]);
        assert(control_point_to_edge.count(compacted_key) == 0);
        assert(control_point_to_edge.count(compacted_key2) == 0);
        control_point_to_edge[compacted_key] = new_edge_index;
        control_point_to_edge[compacted_key2] = new_edge_index;
    }

    assert(Valid());
}
