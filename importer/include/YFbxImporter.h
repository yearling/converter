#pragma once
#include <string>
#include <memory>
#include <vector>
#include <set>
#include "fbxsdk.h"
#include "Math/YVector.h"
#include "fbxsdk/core/math/fbxaffinematrix.h"
#include "YFbxUtility.h"
#include "fbxsdk/scene/shading/fbxsurfacematerial.h"
#include "Engine/YRawMesh.h"
#include "Engine/YMaterial.h"
#include "Engine/YStaticMesh.h"
#include "Engine/YSkeletonMesh.h"
#include "SObject/SStaticMesh.h"



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
    bool import_morph = true;
    bool generate_lighmap_uv = true;
    bool generate_reverse_indices = true;
    bool generate_depth_only_indices = true;
    bool generate_reverse_depth_only_indices = true;
    bool generate_adjacency_indices = true;
    bool generate_hi_mesh_data = false;
};

struct FbxMeshInfo
{
    std::string name;
    bool is_skeleton_mesh = false;
    int morph_num = 0;
    std::string skeleton_name;
    int skeleton_count;
};

struct FbxImportSceneInfo
{
    double time = 0;
    bool has_skin = false;
    bool has_animation = false;
    int geometry_count = 0;
    int no_skin_mesh = 0;
    int skin_mesh = 0;
    double frame_rate = 0.0;
    std::vector<FbxMeshInfo> mesh_infos;
    std::string model_name;
    std::vector<std::string> material_names;
};

struct ConvertedResult
{
    ConvertedResult();
    //std::vector<std::unique_ptr<YStaticMesh>> static_meshes;
    std::vector<TRefCountPtr<SStaticMesh>> static_meshes;
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


protected:
    //skeleton mesh:bone
    FbxNode* GetRootSketeton(FbxNode* link);
    bool RetrievePoseFromBindPose(const std::vector<FbxNode*>& mesh_nodes, std::vector<FbxPose*>& pose_array);
    void RecursiveBuildSkeleton(FbxNode* link, std::vector<FbxNode*>& out_bones);
    std::unique_ptr<YSkeleton> BuildSkeleton(FbxNode* root_node, std::unordered_map<int, FbxNode*>& out_map);

protected:
    // animation 
    std::unique_ptr<AnimationData> ImportAnimationData(YSkeleton* skeleton, std::unordered_map<int, FbxNode*>& bone_id_to_fbxnode);

protected:
    //skeleton mesh:skin
    std::unique_ptr<YSkeletonMesh> ImportSkeletonMesh(FbxNode* root_node, const std::string& mesh_name);
    std::unique_ptr<YSkinDataImported> ImportSkinData(YSkeleton* skeleton, const std::vector<FbxNode*>& mesh_contain_skeleton_and_bs);
    bool ImportBlendShapeAnimation(YSkinDataImported* skin_data, AnimationData* anim_data, const std::vector<FbxNode*>& mesh_contain_skeleton_and_bs);
    bool PostProcessSkeletonMesh(YSkeletonMesh* skeleton_mesh);

protected:
    //static mesh
    TRefCountPtr<SStaticMesh> ImportStaticMeshAsSingle(std::vector<FbxNode*>& mesh_nodes, const std::string& mesh_name, int lod_index = 0);
    bool ImportFbxMeshToRawMesh(FbxNode* node, ImportedRawMesh& raw_mesh);
    bool BuildStaicMeshRenderData(YStaticMesh* static_mesh, std::vector<std::shared_ptr< ImportedRawMesh>>& raw_meshes);

    // 公共函数
protected:
    void RecursiveFindMesh(FbxNode* node, std::vector<FbxNode*>& mesh_nodes);
    void RenameNodeName();
    void RenameMaterialName();
    void ParseSceneInfo();
    void ValidateAllMeshesAreReferenceByNodeAttribute();
    void GetMeshArray(FbxNode* root, std::vector<FbxNode*>& fbx_mesh_nodes);

protected:
    // 矩阵相关
    void ApplyTransformSettingsToFbxNode(FbxNode* node);
    void BuildFbxMatrixForImportTransform(FbxAMatrix& out_matrix);
    void ConvertSystemAndUnit();
    void CheckSmoothingInfo(FbxMesh* fbx_mesh);
    FbxAMatrix ComputeTotalMatrix(FbxNode* node);
    bool IsOddNegativeScale(FbxAMatrix& total_matrix);
protected:
    //材质相关
    //获取当前node里所有的FbxMaterial，并保存在out_materials中，key是fbx里的material的index。
       //因为材质是跨mesh的，所以使用imported_material_data来作为manager，使用FbxSurfaceMaterial*作为key来保证相同的fbx材质
       //生成唯一的材质
    void FindOrImportMaterialsFromNode(FbxNode* fbx_node, std::unordered_map<int, std::shared_ptr<YImportedMaterial>>& out_materials, std::vector<std::string>& us_sets);
    std::shared_ptr<YImportedMaterial> FindExistingMaterialFormFbxMaterial(const FbxSurfaceMaterial* fbx_materia, std::vector<std::string>& uv_setsl);
    std::shared_ptr<YImportedMaterial> GenerateFbxMaterial(const FbxSurfaceMaterial* surface_material, const std::vector<std::string>& uv_set);
    void ConvertYImportedMaterialToSSMaterial(const std::vector<std::shared_ptr<YImportedMaterial>>& imported_materials, std::vector<TRefCountPtr<SMaterial>>& out_materials);





private:
    bool InitSDK();
    FbxManager* fbx_manager_ = nullptr;
    FbxScene* fbx_scene_ = nullptr;
    FbxGeometryConverter* fbx_geometry_converter_ = nullptr;
    FbxImporter* importer_ = nullptr;
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

