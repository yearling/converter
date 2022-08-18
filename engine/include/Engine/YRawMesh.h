#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <set>
#include "YMath.h"
#include "math/YVector.h"
#include "YFile.h"
#include "Math/YBox.h"
#include "YArchive.h"
#include "YPackedNormal.h"
#include "Math/Vector2DHalf.h"

enum NormalCaculateMethod
{
    ImportNormal = 0,
    Caculate = 1,
    ImportNormalAndTangnet = 2,
};

enum TangentMethod
{
    Mikkt = 0,
    BuildIn = 1
};

struct YImportedMaterial
{
    struct ParamTexture
    {
        std::string param_name;
        std::string texture_path;
        int uv_index;
        bool is_normal_map;
    };
public:
    YImportedMaterial() {};
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

struct YMeshWedge
{
	YMeshWedge();

	int control_point_id = INVALID_ID;
    YVector position; //copy of control point postion ,for quickly access
	YVector normal{ 0.0,0.0,1.0 };
	YVector tangent{ 1.0,0.0,0.0 };
	YVector bitangent{ 0.0,1.0,0.0 };
	float binormal_sign{ 0.0 };
	YVector4 color{ 0.0,0.0,0.0,0.0 };
	std::vector<YVector2> uvs;
    float corner_angle = 0.0; // for average normal
    /** List of connected triangles */
	std::vector<int> connected_triangles;

	void AddTriangleID(int triangle_id);
};

struct YMeshEdge
{
public:
    YMeshEdge();
    /** IDs of the two editable mesh vertices that make up this edge.  The winding direction is not defined. */
    int control_points_ids[2] = { INVALID_ID,INVALID_ID };
    /** The triangles that share this edge */
    std::vector<int> connected_triangles;
    bool edge_hardness = false;
    float edge_crease_sharpness = 0;
    bool is_uv_seam = false;

public:
    void AddTriangleID(int triangle_id);
};

struct YMeshPolygon
{
	int polygon_group_id = INVALID_ID; //deprecate 
	std::vector<int> wedge_ids;
    YVector face_normal{ 0.0,0.0,1.0 };
    YVector face_tangent{ 1.0,0.0,0.0 };
    YVector face_bitangent{ 0.0,1.0,0.0 };
    float face_bitanget_sign = 1.0;
    float face_aera = 0.0f; // for average normal
};

struct YMeshPolygonGroup
{
	std::vector<int> polygons;
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
	std::vector<YMeshWedge> vertex_instances;
	std::vector< YMeshPolygon> polygons;
	std::vector< YMeshEdge> edges;
	std::vector<YMeshPolygonGroup> polygon_groups;
	std::unordered_map<int,  std::shared_ptr<YImportedMaterial>> polygon_group_to_material_name;
	std::unordered_map<int,  std::shared_ptr<YImportedMaterial>> polygon_group_to_material;
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
    int LOD_index;
    std::string name;
    // topo
    std::vector<YMeshControlPoint> control_points;
    std::vector< YMeshEdge> edges;
    std::vector< YMeshPolygon> polygons;
    std::vector<YMeshPolygonGroup> polygon_groups;
    // info
    std::vector<YMeshWedge> wedges;
    //material
    std::vector<std::shared_ptr<YImportedMaterial>> polygon_group_to_material;
    YBox aabb;

public:
    ImportedRawMesh();
    int GetVertexPairEdge(int control_point_id0, int control_id1)const ;
    int CreateEdge(int control_point_id_0, int control_point_id_1);
    int CreatePolygon(int polygon_group_id, std::vector<int> in_wedges, std::vector<int>& out_edges);
    void ComputeAABB();
    void CompressControlPoint();
    // all referenced
    bool Valid() const;
    void Merge(ImportedRawMesh& other);
    void ComputeWedgeNormalAndTangent(NormalCaculateMethod normal_method, TangentMethod tangent_method,bool ignore_degenerate_triangle = true);
    void ComputeTriangleNormalAndTangent(NormalCaculateMethod normal_method, TangentMethod tangent_method);
    void ComputeUVSeam();
    void CompressMaterials();
    void GenerateLightMapUV();
    void CompressWedges();
protected:
    friend struct MikktHelper;
    std::set<int> GetSplitTriangleGroupBySoftEdge( int wedge_index, bool split_uv_seam );
    std::set<int> GetSplitTriangleGroupBySoftEdgeSameTangentSign(int wedge_index, const std::set<int>& connected_triangles);
    struct FlowFlagRawMesh
    {
        int triangle_id = -1;
        bool visited = false;
        int wedge_id = -1;
    };
    void RecursiveFindGroup(int triangle_id, std::set<int>& out_triangle_group, std::unordered_map<int, FlowFlagRawMesh>& around_triangle_ids, bool split_uv_seam);
    void RemoveNearHaredEdge(int triangle_id, std::set<int>& out_triangle_group, const std::unordered_map<int, FlowFlagRawMesh>& around_triangle_ids);
    bool IsUVSeam(int edge_index);
    static float CalculateTriangleArea(const YVector& v0, const YVector& v1, const YVector& v2);
    static float ComputeTriangleCornerAngle(const YVector& v0, const YVector& v1, const YVector& v2);
    void ComputeTangentSpaceMikktMethod(bool ignore_degenerate_triangle = true);
};


YArchive& operator<<(YArchive& mem_file,  YLODMesh& lod_mesh);

YArchive& operator<<(YArchive& mem_file,  YRawMesh& raw_mesh);

YArchive& operator<<(YArchive& mem_file,  YMeshEdge& mesh_edge);

YArchive& operator<<(YArchive& mem_file,  YMeshPolygonGroup& mesh_polygon_group);

YArchive& operator<<(YArchive& mem_file,  YMeshPolygon& mesh_polygon);

YArchive& operator<<(YArchive& mem_file,  YMeshWedge& mesh_vertex_instance);

YArchive& operator<<(YArchive& mem_file,  YMeshControlPoint& mesh_vertex);


