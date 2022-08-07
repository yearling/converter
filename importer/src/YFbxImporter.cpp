#include <set>
#include <vector>
#include <cassert>
#include "fbxsdk/core/fbxmanager.h"
#include "fbxsdk/fileio/fbxiosettings.h"
#include "fbxsdk/scene/fbxaxissystem.h"
#include "fbxsdk/core/math/fbxaffinematrix.h"
#include "fbxsdk/scene/geometry/fbxlayer.h"
#include "YFbxImporter.h"
#include "Engine/YLog.h"
#include "Engine/YRawMesh.h"
#include "Engine/YMaterial.h"
#include "YFbxUtility.h"
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "engine/YStaticMesh.h"
#include "YFbxMaterial.h"
#include "Utility/YPath.h"
#include "fbxsdk/scene/animation/fbxanimstack.h"
#include "Utility/YStringFormat.h"
#include "fbxsdk/scene/shading/fbxsurfacematerial.h"

YFbxImporter::YFbxImporter()
{
}
YFbxImporter::~YFbxImporter()
{
	fbx_manager_ = nullptr;
	delete fbx_geometry_converter_;
	fbx_geometry_converter_ = nullptr;
	if (importer_)
	{
		importer_->Destroy();
		importer_ = nullptr;
	}

	if (fbx_scene_)
	{
		fbx_scene_->Destroy();
		fbx_scene_ = nullptr;
	}
}

bool YFbxImporter::ImportFile(const std::string& file_path)
{
	if (!InitSDK()) {
		return false;
	}
	// Create an importer.
	importer_ = FbxImporter::Create(fbx_manager_, "");
	// Get the version number of the FBX files generated by the
	// version of FBX SDK that you are using.
	int sdk_major, skd_minor, sdk_revision;
	FbxManager::GetFileFormatVersion(sdk_major, skd_minor, sdk_revision);
	LOG_INFO("fbx file version major", sdk_major, " ,minor ", skd_minor, " ,revision ", sdk_revision);
	origin_file_path_ = file_path;
	bool import_success = importer_->Initialize(file_path.c_str());
	if (!import_success) {
		FbxString error = importer_->GetStatus().GetErrorString();
		ERROR_INFO( "open file ",file_path,"failed! " , error.Buffer());
		//MY_TRACE_TMP("hello world! ", 10, ", ", 42);
		return false;
	}
	fbx_creator_ = EUnknown;
	FbxIOFileHeaderInfo* file_head_info = importer_->GetFileHeaderInfo();
	if (file_head_info) {
		//Example of creator file info string
		//Blender (stable FBX IO) - 2.78 (sub 0) - 3.7.7
		//Maya and Max use the same string where they specify the fbx sdk version, so we cannot know it is coming from which software
		//We need blender creator when importing skeletal mesh containing the "armature" dummy node as the parent of the root joint. We want to remove this dummy "armature" node
		std::string creator_str(file_head_info->mCreator.Buffer());
		if (creator_str.find("Blender") != std::string::npos)
		{
			fbx_creator_ = DCCSoftware::EBlender;
		}
	}

	fbx_scene_ = FbxScene::Create(fbx_manager_, "");
	FbxIOSettings* ios_settings = fbx_manager_->GetIOSettings();
	ios_settings->SetBoolProp(IMP_FBX_MATERIAL, true);
	ios_settings->SetBoolProp(IMP_FBX_TEXTURE, true);
	ios_settings->SetBoolProp(IMP_FBX_LINK, true);
	ios_settings->SetBoolProp(IMP_FBX_SHAPE, true);
	ios_settings->SetBoolProp(IMP_FBX_GOBO, true);
	ios_settings->SetBoolProp(IMP_FBX_ANIMATION, true);
	ios_settings->SetBoolProp(IMP_SKINS, true);
	ios_settings->SetBoolProp(IMP_DEFORMATION, true);
	ios_settings->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	ios_settings->SetBoolProp(IMP_TAKE, true);

	bool import_scene_success = importer_->Import(fbx_scene_);
	if (!import_scene_success)
	{
		FbxString error = importer_->GetStatus().GetErrorString();
		ERROR_INFO("import scene ", file_path, "failed! ", error.Buffer());
		return false;
	}
	else
	{
		LOG_INFO(file_path," open succes!!");
	}
	RenameNodeName();
	RenameMaterialName();
	ParseSceneInfo();
	return true;
}

const FbxImportSceneInfo* YFbxImporter::GetImportedSceneInfo() const
{
	return scene_info_.get();
}

bool YFbxImporter::ParseFile(const FbxImportParam& import_param, ConvertedResult& out_result)
{
	import_param_ = std::make_unique<FbxImportParam>();
	*import_param_ = import_param;
	ConvertSystemAndUnit();
	ValidateAllMeshesAreReferenceByNodeAttribute();
	FbxNode* root_node = fbx_scene_->GetRootNode();
	if (import_param_->import_as_skelton) {
		//import_param_->transform_vertex_to_absolute = false;
		ApplyTransformSettingsToFbxNode(root_node);
		out_result.skeleton_mesh = ImportSkeletonMesh(root_node,import_param_->model_name);
	}
	else {
		//import static mesh
		ApplyTransformSettingsToFbxNode(root_node);
		if (import_param_->combine_mesh) {
			// todo LOD
			std::vector<FbxNode*> mesh_nodes;
			GetMeshArray(root_node,mesh_nodes);
			out_result.static_meshes.emplace_back(ImportStaticMeshAsSingle(mesh_nodes, import_param_->model_name, 0));
		}
	}
	return true;
}

void YFbxImporter::RenameNodeName()
{
	std::set<std::string> node_names;
    std::set<std::string> renamed_node_name;
	int name_index = 0;
	// 给没有名字的节点一个default的名字
	for (int node_index = 0; node_index < fbx_scene_->GetNodeCount(); ++node_index)
	{
		FbxNode* node = fbx_scene_->GetNode(node_index);
		std::string node_name = node->GetName();
		if (node_name.empty())
		{
			do {
				node_name = "default_node_name_" + std::to_string(name_index++);
			} while (node_names.find(node_name) != node_names.end());
			node->SetName(node_name.c_str());
            renamed_node_name.insert(StringFormat("rename node name:" " ==> %s",node_name.c_str()));
		}
		if (node_name.find(':') != std::string::npos)
		{
            std::string before_rename = node_name;
			auto pos = node_name.find(':');
			node_name[pos] = '_';
			do {
				node_name = node_name + std::to_string(name_index++);
			} while (node_names.find(node_name) != node_names.end());
			node->SetName(node_name.c_str());
            renamed_node_name.insert(StringFormat("rename node name:" " ==> %s",node_name.c_str()));
		}
		node_names.insert(node_name);
	}
    if (!renamed_node_name.empty())
    {
        for (const std::string& renamed_name : renamed_node_name)
        {
            WARNING_INFO(renamed_name);
        }
    }
}

void YFbxImporter::RenameMaterialName()
{
	FbxArray<FbxSurfaceMaterial*> materials;
	fbx_scene_->FillMaterialArray(materials);
	std::set<std::string> material_name_sets;
	int material_name_index = 0;
	for (int material_index = 0; material_index < materials.Size(); ++material_index)
	{
		FbxSurfaceMaterial* material = materials[material_index];
		std::string material_name = material->GetName();
		if (material_name_sets.find(material_name) != material_name_sets.end())
		{
			do {
				material_name = material_name + std::to_string(material_name_index++);
			} while (material_name_sets.find(material_name) != material_name_sets.end());
			material->SetName(material_name.c_str());
		}
		material_name_sets.insert(material_name);
	}
}

void YFbxImporter::ParseSceneInfo()
{
	scene_info_ = std::make_unique<FbxImportSceneInfo>();
	scene_info_->model_name = YPath::GetBaseFilename(origin_file_path_);
	FbxTimeSpan global_time_span(FBXSDK_TIME_INFINITE, FBXSDK_TIME_MINUS_INFINITE);
	
	//统计Mesh
	for(int gemometry_index =0;gemometry_index<fbx_scene_->GetGeometryCount();++gemometry_index)
	{
		FbxGeometry* geometry = fbx_scene_->GetGeometry(gemometry_index);
		if (geometry->GetAttributeType() == FbxNodeAttribute::eMesh) 
		{
			scene_info_->geometry_count++;
			FbxNode* geo_node = geometry->GetNode();
			FbxMesh* mesh = FbxCast<FbxMesh>(geometry);
			FbxMeshInfo mesh_info;
			mesh_info.name = mesh->GetName();
			//lod todo 
			if (mesh->GetDeformerCount(FbxDeformer::EDeformerType::eSkin) > 0) 
			{
				mesh_info.is_skeleton_mesh = true;
				scene_info_->skin_mesh++;
				scene_info_->has_skin = true;
				mesh_info.morph_num = mesh->GetShapeCount();
				FbxSkin* skin = FbxCast<FbxSkin>(mesh->GetDeformer(0,FbxDeformer::eSkin));
				int cluster_count = skin->GetClusterCount();
				FbxNode* link = nullptr;
				for (int cluster_index = 0; cluster_index < cluster_count; ++cluster_index) 
				{
					FbxCluster* cluster = skin->GetCluster(cluster_index);
					link = cluster->GetLink();
					while(link && link->GetParent() && link->GetParent()->GetSkeleton())
					{
						link = link->GetParent();
					}
					if (link != nullptr) {
						break;
					}
				}
				mesh_info.skeleton_name = link ? link->GetName() : "NULLl";
				mesh_info.skeleton_count = link ? link->GetChildCount(true) : 0;
				if (link)
				{
					FbxTimeSpan anim_time_span(FBXSDK_TIME_INFINITE, FBXSDK_TIME_MINUS_INFINITE);
					link->GetAnimationInterval(anim_time_span);
					global_time_span.UnionAssignment(anim_time_span);
				}
			}
			else
			{
				scene_info_->no_skin_mesh++;
				mesh_info.is_skeleton_mesh = false;
			}
			scene_info_->mesh_infos.push_back(mesh_info);
		}
	}
	scene_info_->has_animation = false;
	int anim_curve_node_count = fbx_scene_->GetSrcObjectCount<FbxAnimCurveNode>();
	// sadly Max export with animation curve node by default without any change, so 
	// we'll have to skip the first two curves, which is translation/rotation
	// if there is a valid animation, we'd expect there are more curve nodes than 2. 
	for (int anim_curve_node_index = 2; anim_curve_node_index < anim_curve_node_count; ++anim_curve_node_index)
	{
		FbxAnimCurveNode* cur_anim_curve_node = fbx_scene_->GetSrcObject<FbxAnimCurveNode>(anim_curve_node_index);
		if (cur_anim_curve_node->IsAnimated(true)) 
		{
			scene_info_->has_animation = true;
			break;
		}
	}
	
	scene_info_->frame_rate = FbxTime::GetFrameRate(fbx_scene_->GetGlobalSettings().GetTimeMode());
	if (global_time_span.GetDirection() == FBXSDK_TIME_FORWARD)
	{
		scene_info_->time = (global_time_span.GetDuration().GetMilliSeconds() / 1000.0f * scene_info_->frame_rate);
	}
	else
	{
		scene_info_->time = 0.0;
	}
    int material_count = fbx_scene_->GetMaterialCount();
    std::vector<std::string> &material_names= scene_info_->material_names;
    for (int material_index = 0; material_index < material_count; ++material_index)
    {
        FbxSurfaceMaterial* cur_surface_material = fbx_scene_->GetMaterial(material_index);
        if (cur_surface_material)
        {
            std::string material_name = cur_surface_material->GetName();
            if (std::find(material_names.begin(), material_names.end(), material_name) == material_names.end())
            {
                material_names.push_back(material_name);
            }
        }
    }
    for (std::string& mtl_name : material_names)
    {
        LOG_INFO("materia name is ", mtl_name);
    }
}

void YFbxImporter::ConvertSystemAndUnit()
{
	origin_coordinate_system_ = fbx_scene_->GetGlobalSettings().GetAxisSystem();
	origin_system_unit_ = fbx_scene_->GetGlobalSettings().GetSystemUnit();
	
	// DX convert have bugs, use gl
	if(origin_coordinate_system_!= FbxAxisSystem::OpenGL)
	{
		FbxRootNodeUtility::RemoveAllFbxRoots(fbx_scene_);
		FbxAxisSystem::OpenGL.ConvertScene(fbx_scene_);
	}

	if (origin_system_unit_ != FbxSystemUnit::cm) {
		FbxSystemUnit::cm.ConvertScene(fbx_scene_);
	}
	//Reset all the transform evaluation cache since we change some node transform
	fbx_scene_->GetAnimationEvaluator()->Reset();
}

void YFbxImporter::ValidateAllMeshesAreReferenceByNodeAttribute()
{
	// get all fbxgeometry for node
	std::set<FbxUInt64> geo_ids;
	for (int node_index = 0; node_index < fbx_scene_->GetNodeCount(); ++node_index)
	{
		FbxNode* node = fbx_scene_->GetNode(node_index);
		FbxGeometry* geometry_node =  node->GetGeometry();
		if (geometry_node) {
			geo_ids.insert(geometry_node->GetUniqueID());
		}
	}

	for (int geo_index = 0; geo_index < fbx_scene_->GetGeometryCount(); ++geo_index)
	{
		FbxGeometry* geomotry_node = fbx_scene_->GetGeometry(geo_index);
		if (geo_ids.find(geomotry_node->GetUniqueID()) == geo_ids.end())
		{
			std::string geo_node_name = geomotry_node->GetName() ? geomotry_node->GetName() : " ";
			WARNING_INFO("mesh(", geomotry_node, ") do not have a link to fbx node,can not imported!!");
		}
	}
}

void YFbxImporter::ApplyTransformSettingsToFbxNode(FbxNode* node)
{
	assert(node);
	if (transform_settings_to_apply_.find(node) != transform_settings_to_apply_.end())
	{
		//applied
		return;
	}
	transform_settings_to_apply_.insert(node);
	FbxAMatrix transform_matrix;
	BuildFbxMatrixForImportTransform(transform_matrix);
	FbxDouble3 existing_translation = node->LclTranslation.Get();
	FbxDouble3 existing_rotation = node->LclRotation.Get();
	FbxDouble3 existing_scaling = node->LclScaling.Get();
	FbxVector4 added_translation = transform_matrix.GetT();
	FbxVector4 added_rotation = transform_matrix.GetR();
	FbxVector4 added_scaling = transform_matrix.GetS();

	FbxDouble3 new_translation = FbxDouble3(existing_translation[0] + added_translation[0], existing_translation[1] + added_translation[1], existing_translation[2] + added_translation[2]);
	FbxDouble3 new_rotation = FbxDouble3(existing_rotation[0] + added_rotation[0], existing_rotation[1] + added_rotation[1], existing_rotation[2] + added_rotation[2]);
	FbxDouble3 new_scaling = FbxDouble3(existing_scaling[0] * added_scaling[0], existing_scaling[1] * added_scaling[1], existing_scaling[2] * added_scaling[2]);

	node->LclTranslation.Set(new_translation);
	node->LclRotation.Set(new_rotation);
	node->LclScaling.Set(new_scaling);

	//Reset all the transform evaluation cache since we change some node transform
	fbx_scene_->GetAnimationEvaluator()->Reset();
}

void YFbxImporter::BuildFbxMatrixForImportTransform(FbxAMatrix& out_matrix)
{
	FbxVector4 fbx_added_translation = converter_.ConvertToFbxPos(import_param_->import_translation);
	FbxVector4 Fbx_added_scale = converter_.ConvertToFbxScale(import_param_->import_scaling);
	FbxVector4 Fbx_added_rotation = converter_.ConvertToFbxRot(import_param_->import_rotation);
	out_matrix = FbxAMatrix(fbx_added_translation, Fbx_added_rotation, Fbx_added_scale);
}

void YFbxImporter::GetMeshArray(FbxNode* root,std::vector<FbxNode*>& fbx_mesh_nodes)
{
	if (root->GetMesh()) {
		fbx_mesh_nodes.push_back(root);
	}
	for (int child_index = 0; child_index < root->GetChildCount(); ++child_index)
	{
		GetMeshArray(root->GetChild(child_index), fbx_mesh_nodes);
	}
}

std::unique_ptr<YStaticMesh> YFbxImporter::ImportStaticMeshAsSingle(std::vector<FbxNode*>& mesh_nodes, const std::string& mesh_name, int lod_index /*= 0*/)
{
	if (mesh_nodes.empty())
	{
		return nullptr;
	}

	int num_vertes = 0;
	for (int mesh_index = 0; mesh_index < mesh_nodes.size(); ++mesh_index)
	{
		FbxNode* node = mesh_nodes[mesh_index];
		FbxMesh* fbx_mesh = node->GetMesh();
		if (fbx_mesh)
		{
			num_vertes += fbx_mesh->GetControlPointsCount();
			if (!import_param_->combine_mesh)
			{
				num_vertes = 0;
			}
		}
	}
	CheckSmoothingInfo(mesh_nodes[0]->GetMesh());
	
	std::unique_ptr<YStaticMesh> static_mesh = std::make_unique<YStaticMesh>();
	static_mesh->model_name = mesh_name;
	if (static_mesh->raw_meshes.size() <= lod_index)
	{
		for (int i = 0; i <= lod_index; ++i)
		{
			if (static_mesh->raw_meshes.size() <= i)
			{
				static_mesh->raw_meshes.push_back(YLODMesh());
				static_mesh->raw_meshes.back().LOD_index = i;
			}
			else
			{
				//if (!static_mesh->raw_meshes[i])
				{
					static_mesh->raw_meshes.push_back(YLODMesh());
					static_mesh->raw_meshes.back().LOD_index = i;
				}
			}
		}
	}
	YLODMesh* raw_mesh = &static_mesh->raw_meshes[lod_index];
	std::vector<std::shared_ptr<YFbxMaterial>> mesh_materials;
	int node_fail_count = 0;
	bool is_all_degenerated = true;
	float SqrBoundingBoxThreshold = THRESH_POINTS_ARE_NEAR * THRESH_POINTS_ARE_NEAR;
	for (int mesh_index = 0; mesh_index < 1; ++mesh_index)
	{
		FbxNode* node = mesh_nodes[mesh_index];
		FbxMesh* fbx_mesh = node->GetMesh();
		if (fbx_mesh)
		{
			fbx_mesh->ComputeBBox();
			FbxVector4 global_scale = node->EvaluateGlobalTransform().GetS();
			FbxVector4 bbox_max = FbxVector4(fbx_mesh->BBoxMax.Get()) * global_scale;
			FbxVector4 bbox_min = FbxVector4(fbx_mesh->BBoxMin.Get()) * global_scale;
			FbxVector4 bbox_extent = bbox_max - bbox_min;
			double sqr_size = bbox_extent.SquareLength();
			if (sqr_size > SqrBoundingBoxThreshold)
			{
				is_all_degenerated = false;
				LOG_INFO("Importing static mesh gemometry ", mesh_index, " of ", mesh_nodes.size());
				//BuildStaticMeshFromGeometry(node, raw_mesh, mesh_materials);
                BuildStaticMeshFromGeometry(node, *raw_mesh);
			}
		}
	}
	return std::move(static_mesh);
}

void YFbxImporter::CheckSmoothingInfo(FbxMesh* fbx_mesh)
{
	if (fbx_mesh)
	{
		FbxLayer* smoothing_layer = fbx_mesh->GetLayer(0, FbxLayerElement::eSmoothing);
		if (!smoothing_layer)
		{
			WARNING_INFO("No smoothing group information was found in this FBX scene.  Please make sure to enable the 'Export Smoothing Groups' option in the FBX Exporter plug-in before exporting the file.  Even for tools that don't support smoothing groups, the FBX Exporter will generate appropriate smoothing data at export-time so that correct vertex normals can be inferred while importing.");
		}
	}
}



void YFbxImporter::FindOrImportMaterialsFromNode(FbxNode* fbx_node, std::unordered_map<int, std::shared_ptr<YFbxMaterial>>& out_materials, std::vector<std::string>& us_sets)
{
	if (FbxMesh* mesh_node = fbx_node->GetMesh()) 
	{
		std::set<int> used_material_indexes;
        int mesh_node_element_material_count = mesh_node->GetElementMaterialCount();
		for (int element_material_index = 0; element_material_index != mesh_node_element_material_count; ++element_material_index)
		{
			FbxGeometryElementMaterial* element_material = mesh_node->GetElementMaterial(element_material_index);
			switch (element_material->GetMappingMode())
			{
			case FbxLayerElement::eAllSame:
				if (element_material->GetIndexArray().GetCount() > 0)
				{
					used_material_indexes.insert(element_material->GetIndexArray()[0]);
				}
				break;
			case FbxLayerElement::eByPolygon:
				for (int triangle_index = 0; triangle_index < element_material->GetIndexArray().GetCount(); ++triangle_index)
				{
					used_material_indexes.insert(element_material->GetIndexArray()[triangle_index]);
				}
				break;
			}
		}

		for (int material_index = 0; material_index < fbx_node->GetMaterialCount(); ++material_index)
		{
			const FbxSurfaceMaterial* fbx_material = fbx_node->GetMaterial(material_index);
			//only create the material used by mesh element material
			if (fbx_material && (used_material_indexes.find(material_index) != used_material_indexes.end()))
			{
				std::shared_ptr<YFbxMaterial> material = 	FindExistingMaterialFormFbxMaterial(fbx_material, us_sets);
				out_materials[material_index] = material;
			}
		}
	}
}

std::shared_ptr<YFbxMaterial> YFbxImporter::FindExistingMaterialFormFbxMaterial(const FbxSurfaceMaterial* fbx_materia, std::vector<std::string>& uv_setsl)
{
	if (imported_material_data.fbx_material_to_us_material.find(fbx_materia) == imported_material_data.fbx_material_to_us_material.end())
	{
        std::shared_ptr<YFbxMaterial> material = GenerateFbxMaterial(fbx_materia,uv_setsl);
        //material->InitFromFbx(fbx_materia, uv_setsl);
		imported_material_data.fbx_material_to_us_material[fbx_materia] = material;
	}
	return imported_material_data.fbx_material_to_us_material[fbx_materia];
}

FbxAMatrix YFbxImporter::ComputeTotalMatrix(FbxNode* node)
{
	FbxVector4 geometry_translation = node->GetGeometricTranslation(FbxNode::eSourcePivot);
	FbxVector4 geometry_rotation = node->GetGeometricRotation(FbxNode::eSourcePivot);
	FbxVector4 geometry_scaling = node->GetGeometricScaling(FbxNode::eSourcePivot);
	FbxAMatrix geometry(geometry_translation, geometry_rotation, geometry_scaling);
	//For Single Matrix situation, obtain transfrom matrix from eDESTINATION_SET, which include pivot offsets and pre/post rotations.
	FbxAMatrix global_matrix = fbx_scene_->GetAnimationEvaluator()->GetNodeGlobalTransform(node);
	//We can bake the pivot only if we don't transform the vertex to the absolute position
	if (!import_param_->transform_vertex_to_absolute)
	{
		if (import_param_->bake_pivot_in_vertex)
		{
			FbxVector4 rotation_pivot = node->GetRotationPivot(FbxNode::eSourcePivot);
			FbxVector4 local_to_pivot(-rotation_pivot[0], -rotation_pivot[1], -rotation_pivot[2]);
			FbxAMatrix local_to_pivot_mat;
			local_to_pivot_mat.SetT(local_to_pivot);
			geometry = geometry * local_to_pivot_mat;
		}
		else
		{
			//No Vertex transform and no bake pivot, it will be the mesh as-is.
			geometry.SetIdentity();
		}
	}
	//We must always add the geometric transform. Only Max use the geometric transform which is an offset to the local transform of the node

	FbxAMatrix total_matrix = import_param_->transform_vertex_to_absolute ? global_matrix * geometry : geometry;
	return total_matrix;
}

bool YFbxImporter::IsOddNegativeScale(FbxAMatrix& total_matrix)
{
	FbxVector4 scale= total_matrix.GetS();
	int negative_num = 0;

	if (scale[0] < 0) negative_num++;
	if (scale[1] < 0) negative_num++;
	if (scale[2] < 0) negative_num++;

	return negative_num == 1 || negative_num == 3;
}






bool YFbxImporter::InitSDK() {
	// create sdk manager
	fbx_manager_ = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(fbx_manager_, IOSROOT);
	fbx_manager_->SetIOSettings(ios);

	fbx_geometry_converter_ = new FbxGeometryConverter(fbx_manager_);
	return true;
}

ConvertedResult::ConvertedResult()
{

}
