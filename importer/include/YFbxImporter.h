#pragma once
#include <string>
#include <memory>
#include "fbxsdk.h"
#include <vector>
#include "Math/YVector.h"
#include <set>
#include "fbxsdk/core/math/fbxaffinematrix.h"
#include "YFbxUtility.h"
#include "fbxsdk/scene/shading/fbxsurfacematerial.h"
#include "Engine/YRawMesh.h"
#include "Engine/YMaterial.h"

#include "Engine/YStaticMesh.h"
#include "Engine/YSkeletonMesh.h"
struct FbxImportParam 
{
	bool import_as_skelton = false;
	YVector import_translation{ 0.0,0.0,0.0 };
	YVector import_rotation{ 0.0,0.0,0.0 };
	YVector import_scaling{ 1.0,1.0,1.0 };
	bool combine_mesh = true;
	std::string model_name;
	bool transform_vertex_to_absolute = true; // true:for static mesh , false for skeleton mesh
	bool bake_pivot_in_vertex = true; // bake pivot in vetex only in  static mesh, we choose transform_vertex_to_absolute is false, and want vertex bake in pivot space
	bool remove_degenerate_triangles = true;
	int max_bone_per_section = 64;
	bool import_morph = false;
};

struct FbxMeshInfo
{
	std::string name;
	bool is_skeleton_mesh=false;
	int morph_num=0;
	std::string skeleton_name;
	int skeleton_count;
};
struct FbxImportSceneInfo
{
	double time=0;
	bool has_skin=false;
	bool has_animation=false;
	int geometry_count=0;
	int no_skin_mesh=0;
	int skin_mesh=0;
	double frame_rate = 0.0;
	std::vector<FbxMeshInfo> mesh_infos;
	std::string model_name;
};

struct ConvertedResult
{
	ConvertedResult();
	std::vector<std::unique_ptr<YStaticMesh>> static_meshes;
	std::unique_ptr<YSkeletonMesh> skeleton_mesh;
};
enum DCCSoftware {
	E3dsMax,
	EMaya,
	EBlender,
	EC4D,
	EZB,
	EUnknown
};
class YFbxImporter {
public:
	YFbxImporter();
	~YFbxImporter();
	bool ImportFile(const std::string& file_path);
	const FbxImportSceneInfo* GetImportedSceneInfo() const;
	bool ParseFile(const FbxImportParam& import_param, ConvertedResult& out_result);
protected:
	std::unique_ptr<YStaticMesh> ImportStaticMeshAsSingle(std::vector<FbxNode*>& mesh_nodes, const std::string&  mesh_name,int lod_index = 0);
	std::unique_ptr<YSkeletonMesh> ImportSkeletonMesh(FbxNode* root_node, const std::string& mesh_name);
	std::unique_ptr<YSkeleton> BuildSkeleton(FbxNode* root_node,std::unordered_map<int,FbxNode*>& out_map);
	std::unique_ptr<AnimationData> ImportAnimationData(YSkeleton* skeleton,  std::unordered_map<int, FbxNode*>& bone_id_to_fbxnode);
protected:
	void RenameNodeName();
	void RenameMaterialName();
	void ParseSceneInfo();
	void ConvertSystemAndUnit();
	void ValidateAllMeshesAreReferenceByNodeAttribute();
	void ApplyTransformSettingsToFbxNode(FbxNode* node);
	void BuildFbxMatrixForImportTransform(FbxAMatrix & out_matrix);
	void GetMeshArray(FbxNode* root,std::vector<FbxNode*>& fbx_mesh_nodes);
	void CheckSmoothingInfo(FbxMesh* fbx_mesh);
	bool BuildStaticMeshFromGeometry(FbxNode* node,YLODMesh* raw_mesh, std::vector<YFbxMaterial*>& existing_materials);
	void FindOrImportMaterialsFromNode(FbxNode* fbx_node, std::vector<YFbxMaterial*>& out_materials, std::vector<std::string>& us_sets);
	YFbxMaterial* FindExistingMaterialFormFbxMaterial(const FbxSurfaceMaterial* fbx_material);
	FbxAMatrix ComputeTotalMatrix(FbxNode* node);
	bool IsOddNegativeScale(FbxAMatrix& total_matrix);

protected:
	//skeleton mesh:bone
	FbxNode* GetRootSketeton(FbxNode* link);
	bool RetrievePoseFromBindPose(const std::vector<FbxNode*>& mesh_nodes, std::vector<FbxPose*>& pose_array);
	void RecursiveBuildSkeleton(FbxNode* link, std::vector<FbxNode*>& out_bones);

protected:
	std::unique_ptr<YSkinDataImported> ImportSkinData(YSkeleton* skeleton, const std::vector<FbxNode*>& mesh_contain_skeleton_and_bs);
	bool ImportBlendShapeAnimation(YSkinDataImported* skin_data, AnimationData* anim_data, const std::vector<FbxNode*>& mesh_contain_skeleton_and_bs);
	void RecursiveFindMesh(FbxNode* node, std::vector<FbxNode*>& mesh_nodes);
	bool PostProcessSkeletonMesh(YSkeletonMesh* skeleton_mesh);
private:
	bool InitSDK();
	FbxManager* fbx_manager_ = nullptr;
	FbxScene* fbx_scene_ = nullptr;
	FbxGeometryConverter* fbx_geometry_converter_ = nullptr;
	FbxImporter* importer_=nullptr;
	std::string origin_file_path_;
	DCCSoftware fbx_creator_;
	std::unique_ptr<FbxImportSceneInfo> scene_info_;
	std::unique_ptr< FbxImportParam> import_param_;
	FbxAxisSystem origin_coordinate_system_;
	FbxSystemUnit origin_system_unit_;
	std::set<FbxNode*> transform_settings_to_apply_;
	FbxDataConverter converter_;
	FImportedMaterialData imported_material_data;
};

