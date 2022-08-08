#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "YMath.h"
#include "math/YVector.h"
#include "YFile.h"
#include "Math/YBox.h"
#include "YArchive.h"

const int INVALID_ID = -1;

struct YFbxMaterial
{
    struct ParamTexture
    {
        std::string param_name;
        std::string texture_path;
        int uv_index;
        bool is_normal_map;
    };
public:
    YFbxMaterial() {};
    std::string name;
    std::unordered_map<std::string, ParamTexture> param_textures;

};


struct YMeshControlPoint
{
	/** All of vertex instances which reference this vertex (for split vertex support) */
	std::vector<int> wedge_ids;
	/** The edges connected to this vertex */
	std::vector<int>  edge_ids;

	YVector position;

	void AddWedge(int index);
};

struct YMeshVertexWedge
{
	YMeshVertexWedge();
	/** The vertex this is instancing */
	int control_point_id = INVALID_ID;
    YVector position;
	/** List of connected triangles */
	std::vector<int> connected_triangles;

	YVector normal{ 0.0,0.0,1.0 };
	YVector tangent{ 1.0,0.0,0.0 };
	YVector bitangent{ 0.0,1.0,0.0 };
	float binormal_sign{ 0.0 };
	YVector4 color{ 0.0,0.0,0.0,0.0 };
	std::vector<YVector2> uvs;
	void AddTriangleID(int triangle_id);
};

struct YMeshPolygon
{
	int polygon_group_id = INVALID_ID;
	std::vector<int> wedge_ids;
};

struct YMeshPolygonGroup
{
	std::vector<int> polygons;
};

struct YMeshEdge
{

	YMeshEdge();
	/** IDs of the two editable mesh vertices that make up this edge.  The winding direction is not defined. */
    int control_points_ids[2] = { INVALID_ID,INVALID_ID };

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
	std::vector<YMeshControlPoint> vertex_position;
	//std::vector<YMeshVertex> vertices;
	std::vector<YMeshVertexWedge> vertex_instances;
	std::vector< YMeshPolygon> polygons;
	std::vector< YMeshEdge> edges;
	std::vector<YMeshPolygonGroup> polygon_groups;
	std::unordered_map<int,  std::shared_ptr<YFbxMaterial>> polygon_group_to_material_name;
	std::unordered_map<int,  std::shared_ptr<YFbxMaterial>> polygon_group_to_material;
    std::unordered_map<uint64_t, int> edged_vertex_id_to_edge_id;
	YBox aabb;
	int GetVertexPairEdge(int vertex_id0, int vertex_id1);
	int CreateEdge(int vertex_id_0, int vertex_id_1);
	int CreatePolygon(int polygon_group_id, std::vector<int> vertex_ins_ids, std::vector<int>& out_edges);
	void ComputeAABB();
};

struct YStaticMeshSection
{
    int offset=-1;
    int triangle_count=-1;
};

struct ImportedRawMesh
{
public:
    ImportedRawMesh();
    int LOD_index;
    std::string name;
    // topo
    std::vector<YMeshControlPoint> control_points;
    std::vector< YMeshEdge> edges;
    std::vector< YMeshPolygon> polygons;
    std::vector<YMeshPolygonGroup> polygon_groups;
    // info
    std::vector<YMeshVertexWedge> wedges;
    //material
    std::unordered_map<int, std::shared_ptr<YFbxMaterial>> polygon_group_to_material;
    std::unordered_map<int, int> material_to_polygon_group;

    //speed up structure
    std::unordered_map<uint64_t, int> control_point_to_edge;

    YBox aabb;
    int GetVertexPairEdge(int vertex_id0, int vertex_id1)const ;
    int CreateEdge(int vertex_id_0, int vertex_id_1);
    int CreatePolygon(int polygon_group_id, std::vector<int> in_wedges, std::vector<int>& out_edges);
    void ComputeAABB();
    void CompressControlPoint();
    // all referenced
    bool Valid() const;
};

YArchive& operator<<(YArchive& mem_file,  YLODMesh& lod_mesh);

YArchive& operator<<(YArchive& mem_file,  YRawMesh& raw_mesh);

YArchive& operator<<(YArchive& mem_file,  YMeshEdge& mesh_edge);

YArchive& operator<<(YArchive& mem_file,  YMeshPolygonGroup& mesh_polygon_group);

YArchive& operator<<(YArchive& mem_file,  YMeshPolygon& mesh_polygon);

YArchive& operator<<(YArchive& mem_file,  YMeshVertexWedge& mesh_vertex_instance);

YArchive& operator<<(YArchive& mem_file,  YMeshControlPoint& mesh_vertex);


