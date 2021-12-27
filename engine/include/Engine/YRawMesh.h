#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "YMath.h"
#include "math/YVector.h"

const int INVALID_ID = -1;
struct YMeshVertex
{
	/** All of vertex instances which reference this vertex (for split vertex support) */
	std::vector<int> vertex_instance_ids;
	/** The edges connected to this vertex */
	std::vector<int>  connect_edge_ids;

	YVector position;

	void AddVertexInstance(int index);
};

struct YMeshVertexInstance
{
	YMeshVertexInstance();
	/** The vertex this is instancing */
	int vertex_id = -1;

	/** List of connected triangles */
	std::vector<int> connected_triangles;

	YVector vertex_instance_normal{ 0.0,0.0,1.0 };
	YVector vertex_instance_tangent{ 0.0,0.0,1.0 };
	float vertex_instance_binormal_sign{ 0.0 };
	YVector4 vertex_instance_color{ 0.0,0.0,0.0,0.0 };
	std::vector<YVector2> vertex_instance_uvs;
	void AddTriangleID(int triangle_id);
};

struct YMeshPolygon
{
	int polygon_group_id = -1;
	std::vector<int> vertex_instance_ids;
};

struct YMeshPolygonGroup
{
	std::vector<int> polygons;
};

struct YMeshEdge
{

	YMeshEdge();
	/** IDs of the two editable mesh vertices that make up this edge.  The winding direction is not defined. */
	int VertexIDs[2];

	/** The triangles that share this edge */
	std::vector<int> connected_triangles;
	bool edge_hardness = false;
	float edge_crease_sharpness = 0;
	void AddTriangleID(int triangle_id);
};

struct YRawMesh
{
public:
	std::string mesh_name;
};

struct YLODMesh
{
public:
	YLODMesh();
	int LOD_index;
	std::vector<YRawMesh> sub_meshes;
	std::vector<YMeshVertex> vertex_position;
	//std::vector<YMeshVertex> vertices;
	std::vector<YMeshVertexInstance> vertex_instances;
	std::vector< YMeshPolygon> polygons;
	std::vector< YMeshEdge> edges;
	std::vector<YMeshPolygonGroup> polygon_groups;
	std::unordered_map<int, std::string> polygon_group_imported_material_slot_name;

	int GetVertexPairEdge(int vertex_id0, int vertex_id1);
	int CreateEdge(int vertex_id_0, int vertex_id_1);
	int CreatePolygon(int polygon_group_id, std::vector<int> vertex_ins_ids, std::vector<int>& out_edges);
};


