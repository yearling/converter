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
