#include "Engine/YRawMesh.h"
#include "Math/YVector.h"
#include <cassert>
YLODMesh::YLODMesh()
{

}

int YLODMesh::GetVertexPairEdge(int vertex_id0, int vertex_id1)
{
	//verte
	std::vector<int>& connect_edges = vertex_position[vertex_id0].connect_edge_ids;
	for (int edge : connect_edges)
	{
		int vertex_maybe_0 = edges[edge].VertexIDs[0];
		int vertex_maybe_1 = edges[edge].VertexIDs[1];
		if ((vertex_maybe_0 == vertex_id0 && vertex_maybe_1 == vertex_id1) || (vertex_maybe_0 == vertex_id1 && vertex_maybe_1 == vertex_id0))
		{
			return edge;
		}
	}
	return -1;
}

int YLODMesh::CreateEdge(int vertex_id_0, int vertex_id_1)
{
#if defined(DEBUG) || defined(_DEBUG)
	int exist_id = GetVertexPairEdge(vertex_id_0, vertex_id_1);
	assert(exist_id == -1);
#endif
	YMeshEdge tmp_edge;
	tmp_edge.VertexIDs[0] = vertex_id_0;
	tmp_edge.VertexIDs[1] = vertex_id_1;
	int edge_id = (int)edges.size();
	edges.push_back(tmp_edge);
	vertex_position[vertex_id_0].connect_edge_ids.push_back(edge_id);
	vertex_position[vertex_id_1].connect_edge_ids.push_back(edge_id);
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
	tmp_polygon.vertex_instance_ids = vertex_ins_ids;
	polygon_groups[polygon_group_id].polygons.push_back(polygon_id);

	std::vector<int> vertex_ids;
	vertex_ids.reserve(vertex_ins_ids.size());

	//only support triangle,UE support polygon
	for (int i = 0; i < vertex_ins_ids.size(); ++i)
	{
		vertex_instances[vertex_ins_ids[i]].AddTriangleID(polygon_id);
		int i_next = (i + 1) % ((int)vertex_ins_ids.size());
		int vertex_id = vertex_instances[vertex_ins_ids[i]].vertex_id;
		int vertex_next_id = vertex_instances[vertex_ins_ids[i_next]].vertex_id;
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
	for (YMeshVertex& v : vertex_position)
	{
		aabb += v.position;
	}
}

YMeshEdge::YMeshEdge()
{
	VertexIDs[0] = -1;
	VertexIDs[1] = -1;
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

YMeshVertexInstance::YMeshVertexInstance()
{
	vertex_instance_uvs.resize(MAX_MESH_TEXTURE_COORDS, YVector2(0.0, 0.0));
}

void YMeshVertexInstance::AddTriangleID(int triangle_id)
{
	auto find_reuslt = std::find(connected_triangles.begin(), connected_triangles.end(), triangle_id);
	if (find_reuslt == connected_triangles.end())
	{
		connected_triangles.push_back(triangle_id);
	}
}

void YMeshVertex::AddVertexInstance(int index)
{
	auto find_reuslt = std::find(vertex_instance_ids.begin(), vertex_instance_ids.end(), index);
	if (find_reuslt == vertex_instance_ids.end())
	{
		vertex_instance_ids.push_back(index);
	}

}

MemoryFile& operator<<(MemoryFile& mem_file, YLODMesh& lod_mesh)
{
	mem_file << lod_mesh.LOD_index;
	mem_file << lod_mesh.sub_meshes;
	mem_file << lod_mesh.vertex_position;
	mem_file << lod_mesh.vertex_instances;
	mem_file << lod_mesh.polygons;
	mem_file << lod_mesh.edges;
	mem_file << lod_mesh.polygon_groups;
	mem_file << lod_mesh.polygon_group_imported_material_slot_name;
	return mem_file;
}

MemoryFile& operator<<(MemoryFile& mem_file, YRawMesh& raw_mesh)
{
	mem_file << raw_mesh.mesh_name;
	return mem_file;
}

MemoryFile& operator<<(MemoryFile& mem_file,  YMeshEdge& mesh_edge)
{
	mem_file << mesh_edge.VertexIDs[0];
	mem_file << mesh_edge.VertexIDs[1];
	mem_file << mesh_edge.connected_triangles;
	mem_file << mesh_edge.edge_hardness;
	mem_file << mesh_edge.edge_crease_sharpness;

	return mem_file;
}

MemoryFile& operator<<(MemoryFile& mem_file,  YMeshPolygonGroup& mesh_polygon_group)
{
	mem_file << mesh_polygon_group.polygons;
	return mem_file;
}

MemoryFile& operator<<(MemoryFile& mem_file, YMeshPolygon& mesh_polygon)
{
	mem_file << mesh_polygon.polygon_group_id;
	mem_file << mesh_polygon.vertex_instance_ids;
	return mem_file;
}

MemoryFile& operator<<(MemoryFile& mem_file,  YMeshVertexInstance& mesh_vertex_instance)
{
	mem_file << mesh_vertex_instance.vertex_id;
	mem_file << mesh_vertex_instance.connected_triangles;
	mem_file << mesh_vertex_instance.vertex_instance_normal;
	mem_file << mesh_vertex_instance.vertex_instance_tangent;
	mem_file << mesh_vertex_instance.vertex_instance_binormal_sign;
	mem_file << mesh_vertex_instance.vertex_instance_color;
	mem_file << mesh_vertex_instance.vertex_instance_uvs;

	return mem_file;
}

MemoryFile& operator<<(MemoryFile& mem_file,  YMeshVertex& mesh_vertex)
{
	mem_file << mesh_vertex.vertex_instance_ids;
	mem_file << mesh_vertex.connect_edge_ids;
	mem_file << mesh_vertex.position;
	return mem_file;
}

