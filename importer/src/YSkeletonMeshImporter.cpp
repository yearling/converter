#include "YFbxImporter.h"
#include "fbxsdk/scene/geometry/fbxdeformer.h"
#include <algorithm>
#include "Engine/YSkeletonMesh.h"
#include <deque>
#include "fbxsdk/scene/geometry/fbxcluster.h"
#include "fbxsdk/scene/geometry/fbxblendshape.h"
#include "Engine/YLog.h"
#include "Utility/YStringFormat.h"
#include <unordered_set>

void YFbxImporter::RecursiveFindMesh(FbxNode* node, std::vector<FbxNode*>& mesh_nodes)
{
	if (!node)
	{
		return;
	}

	if (node && node->GetNodeAttribute() && (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh))
	{
		mesh_nodes.push_back(node);
	}
	int child_node_count = node->GetChildCount();
	for (int i_child = 0; i_child < child_node_count; ++i_child)
	{
		RecursiveFindMesh(node->GetChild(i_child), mesh_nodes);
	}
}

std::unique_ptr<YSkeletonMesh> YFbxImporter::ImportSkeletonMesh(FbxNode* root_node, const std::string& mesh_name)
{
	std::unordered_map<int, FbxNode*> bone_id_to_fbx_node;
	std::unique_ptr<YSkeleton> skeleton = BuildSkeleton(root_node, bone_id_to_fbx_node);
	std::unique_ptr<AnimationData> animation_data = ImportAnimationData(skeleton.get(), bone_id_to_fbx_node);

	std::unique_ptr<YSkeletonMesh> skeleton_mesh = std::make_unique<YSkeletonMesh>();
	skeleton_mesh->skeleton_ = std::move(skeleton);
	skeleton_mesh->animation_data_ = std::move(animation_data);
	std::unordered_map<int, FbxMesh*> tmp_skin_mesh_to_fbx;
	std::vector<FbxNode*> mesh_contain_skeleton_and_bs;
	RecursiveFindMesh(root_node, mesh_contain_skeleton_and_bs);
	std::unique_ptr<YSkinDataImported> skin_data = ImportSkinData(skeleton_mesh->skeleton_.get(), mesh_contain_skeleton_and_bs);
	skeleton_mesh->skin_data_ = std::move(skin_data);
	if (import_param_->import_morph) {
		ImportBlendShapeAnimation(skeleton_mesh->skin_data_.get(), skeleton_mesh->animation_data_.get(), mesh_contain_skeleton_and_bs);
	}
	if (!PostProcessSkeletonMesh(skeleton_mesh.get()))
	{
		return nullptr;
	}
	if (!skeleton_mesh->AllocGpuResource())
	{
		ERROR_INFO("SkeletonMesh alloc resource failed!!");
	}

	return skeleton_mesh;
}

std::unique_ptr<YSkinDataImported> YFbxImporter::ImportSkinData(YSkeleton* skeleton, const std::vector<FbxNode*>& mesh_contain_skeleton_and_bs)
{
	LOG_INFO("ImportSkinData");
	std::unique_ptr<YSkinDataImported> skin_data = std::make_unique<YSkinDataImported>();
	for (int gemometry_index = 0; gemometry_index < mesh_contain_skeleton_and_bs.size(); ++gemometry_index)
	{
		int skin_mesh_index = skin_data->meshes_.size();
		skin_data->meshes_.push_back(SkinMesh());
		SkinMesh& skin_mesh = skin_data->meshes_[skin_mesh_index];
		FbxNode* geo_node = mesh_contain_skeleton_and_bs[gemometry_index];
		FbxMesh* mesh = geo_node->GetMesh();
		skin_mesh.name_ = geo_node->GetName();
		//uv 
		FbxUVs fbx_uvs(this, mesh);
		if (!mesh->IsTriangleMesh())
		{
			WARNING_INFO("Triangulation skeleton mesh", geo_node->GetName());
			const bool bReplace = true;
			FbxNodeAttribute* ConvertedNode = fbx_geometry_converter_->Triangulate(mesh, bReplace);
			if (ConvertedNode != NULL && ConvertedNode->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				mesh = (fbxsdk::FbxMesh*)ConvertedNode;
			}
			else
			{
				ERROR_INFO("Unable to triangulate mesh ", geo_node->GetName());
				return false;
			}
		}
		// renew the base layer
		FbxLayer* base_layer = mesh->GetLayer(0);
		FbxLayerElementMaterial* layer_element_material = base_layer->GetMaterials();
		FbxLayerElement::EMappingMode material_mapping_model = layer_element_material ? layer_element_material->GetMappingMode() : FbxLayerElement::eByPolygon;

		fbx_uvs.Phase2(this, mesh);
		//get normal layer
		FbxLayerElementNormal* normal_layer = base_layer->GetNormals();
		FbxLayerElementTangent* tangent_layer = base_layer->GetTangents();
		FbxLayerElementBinormal* binormal_layer = base_layer->GetBinormals();
		bool has_NTB_information = normal_layer && tangent_layer && binormal_layer;

		FbxLayerElement::EReferenceMode normal_reference_mode(FbxLayerElement::eDirect);
		FbxLayerElement::EMappingMode normal_mapping_mode(FbxLayerElement::eByControlPoint);
		if (normal_layer)
		{
			normal_reference_mode = normal_layer->GetReferenceMode();
			normal_mapping_mode = normal_layer->GetMappingMode();
		}
		FbxLayerElement::EReferenceMode tangent_reference_mode(FbxLayerElement::eDirect);
		FbxLayerElement::EMappingMode tangent_mapping_mode(FbxLayerElement::eByControlPoint);
		if (tangent_layer)
		{
			tangent_reference_mode = tangent_layer->GetReferenceMode();
			tangent_mapping_mode = tangent_layer->GetMappingMode();
		}

		FbxLayerElement::EReferenceMode binormal_reference_mode(FbxLayerElement::eDirect);
		FbxLayerElement::EMappingMode binormal_mapping_mode(FbxLayerElement::eByControlPoint);
		if (binormal_layer)
		{
			binormal_reference_mode = binormal_layer->GetReferenceMode();
			binormal_mapping_mode = binormal_layer->GetMappingMode();
		}
		FbxAMatrix total_matrix;
		FbxAMatrix total_matrix_for_normal;
		total_matrix = ComputeTotalMatrix(geo_node);
		total_matrix_for_normal = total_matrix.Inverse();
		total_matrix_for_normal = total_matrix_for_normal.Transpose();

		int polygon_count = mesh->GetPolygonCount();
		if (polygon_count == 0)
		{
			ERROR_INFO("No polygon were found on mesh ", mesh->GetName());
			return false;
		}

		//get control points
		int vertex_count = mesh->GetControlPointsCount();
		bool odd_negative_scale = IsOddNegativeScale(total_matrix);
		skin_mesh.control_points_.reserve(vertex_count);
		skin_mesh.bone_weights_and_id_.resize(vertex_count);

		if (mesh->GetDeformerCount(FbxDeformer::EDeformerType::eSkin) > 0)
		{
			FbxAMatrix test_mesh_to_model;
			bool first_mesh_to_model = false;
			int32 SkinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
			int control_point_count = mesh->GetControlPointsCount();
			for (int skin_index = 0; skin_index < SkinCount; ++skin_index)
			{
				FbxSkin* skin = FbxCast<FbxSkin>(mesh->GetDeformer(skin_index, FbxDeformer::eSkin));
				int cluster_count = skin->GetClusterCount();
				FbxAMatrix mesh_to_model;
				FbxNode* link = nullptr;
				for (int cluster_index = 0; cluster_index < cluster_count; ++cluster_index)
				{
					FbxCluster* cluster = skin->GetCluster(cluster_index);
					link = cluster->GetLink();
					FbxCluster::ELinkMode link_model = cluster->GetLinkMode();
					if (link_model != FbxCluster::ELinkMode::eNormalize)
					{
						ERROR_INFO("not support elinkmodel: ", link_model);
					}

					std::string link_name = link->GetName();
					assert(skeleton->bone_names_to_id_.count(link_name));
					int bone_id = skeleton->bone_names_to_id_[link_name];
					assert(bone_id != -1);
					YBone& bone = skeleton->bones_[bone_id];
					FbxAMatrix bone_to_model;
					cluster->GetTransformMatrix(mesh_to_model);
					if (!first_mesh_to_model)
					{
						test_mesh_to_model = mesh_to_model;
						first_mesh_to_model = true;
					}
					else
					{
						assert(test_mesh_to_model == mesh_to_model);
					}
					cluster->GetTransformLinkMatrix(bone_to_model);
					YTransform test_bone_to_model = converter_.ConverterFbxTransform(bone_to_model);
					if (!test_bone_to_model.NearEqual(bone.global_transform_,0.0002))
					{
						ERROR_INFO(bone.bone_name_, "'s global_transform does not equal cluster->GetTransformLinkMatrix");
					}
					
					FbxAMatrix inv_bone_bind = bone_to_model.Inverse();
					bone.inv_bind_global_matrix_ = converter_.ConvertFbxMatrix(inv_bone_bind);
				
					int cluster_control_point_count = cluster->GetControlPointIndicesCount();
					for (int i = 0; i < cluster_control_point_count; ++i)
					{
						int control_point_index = cluster->GetControlPointIndices()[i];
						if (control_point_index > control_point_count)
						{
							continue;
						}
						double contorl_point_weight = cluster->GetControlPointWeights()[i];
						skin_mesh.bone_weights_and_id_[control_point_index].push_back(BoneWeightAndID(contorl_point_weight, bone.bone_id_));
						//skin_mesh.weights_[control_point_index].push_back(contorl_point_weight);
						//skin_mesh.bone_index_[control_point_index].push_back(bone.bone_id_);
					}
				}
			}
			for (int bone_weidght_index = 0; bone_weidght_index < vertex_count; ++bone_weidght_index)
			{
				std::vector<BoneWeightAndID>& bone_weight_and_id = skin_mesh.bone_weights_and_id_[bone_weidght_index];
				std::sort(bone_weight_and_id.begin(), bone_weight_and_id.end(), [](const BoneWeightAndID& lhs, const BoneWeightAndID& rhs) { return lhs.weight > rhs.weight; });
				
				if (bone_weight_and_id.size() > 8) 
				{
					WARNING_INFO(StringFormat("mesh: %s' %d vertex has binded more than 8 bones", mesh->GetName(), bone_weight_and_id));
					bone_weight_and_id.resize(8);
				}
				float total_weight = 0.0;
				for (BoneWeightAndID& bone_weight : bone_weight_and_id)
				{
					total_weight += bone_weight.weight;
				}
				if (YMath::IsNearlyZero(total_weight))
				{
					bone_weight_and_id.clear();
					bone_weight_and_id.push_back(BoneWeightAndID(1.0, 0));
				}
				else if (total_weight != 1.0)
				{
					for (BoneWeightAndID& bone_weight : bone_weight_and_id)
					{
						bone_weight.weight /= total_weight;
					}
				}
			}
		}

		for (int vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
		{
			FbxVector4 fbx_position = mesh->GetControlPoints()[vertex_index];
			fbx_position = total_matrix.MultT(fbx_position);
			const YVector vertex_position = converter_.ConvertPos(fbx_position);
			skin_mesh.control_points_.push_back(vertex_position);
		}

		if (import_param_->import_morph && mesh->GetShapeCount() > 0)
		{
			BlendShape& bs = skin_mesh.bs_;
			int blend_shape_deformer_count = mesh->GetDeformerCount(FbxDeformer::eBlendShape);
			for (int i_blend_shape_deformer = 0; i_blend_shape_deformer < blend_shape_deformer_count; ++i_blend_shape_deformer)
			{
				FbxBlendShape* blend_shpae_fbx = (FbxBlendShape*)mesh->GetDeformer(i_blend_shape_deformer, FbxDeformer::eBlendShape);
				std::string bs_name = blend_shpae_fbx->GetName();
				int blend_shape_channel_count = blend_shpae_fbx->GetBlendShapeChannelCount();
				for (int i_blend_shape_channel = 0; i_blend_shape_channel < blend_shape_channel_count; ++i_blend_shape_channel)
				{
					BlendShapeTarget blend_shape_target;
					FbxBlendShapeChannel* channel = blend_shpae_fbx->GetBlendShapeChannel(i_blend_shape_channel);
					std::string channel_name = channel->GetName();
					LOG_INFO(" blend shape: ",channel_name);

					blend_shape_target.name_ = channel->GetName();
					if (channel)
					{
						int lShapeCount = channel->GetTargetShapeCount();
						double* lFullWeights = channel->GetTargetShapeFullWeights();
						assert(lShapeCount == 1);
						assert(lFullWeights[0] == 100.0f);
						FbxShape* shape = channel->GetTargetShape(0);
						int control_point_count = shape->GetControlPointsCount();
						FbxVector4* control_points = shape->GetControlPoints();
						blend_shape_target.control_points.reserve(control_point_count);
						for (int i_control_point = 0; i_control_point < control_point_count; ++i_control_point)
						{
							FbxVector4 bs_control_point = control_points[i_control_point];
							bs_control_point = total_matrix.MultT(bs_control_point);
							const YVector vertex_position = converter_.ConvertPos(bs_control_point);
							blend_shape_target.control_points.push_back(vertex_position);
						}
						bs.target_shapes_[blend_shape_target.name_] = blend_shape_target;
					}
				}
			}
		}


		std::vector<YVector> P;
		int triangle_control_index = 0;
		for (int polygon_index = 0; polygon_index < polygon_count; ++polygon_index)
		{
			int polygon_vertex_count = mesh->GetPolygonSize(polygon_index);
			skin_mesh.wedges_.reserve(polygon_vertex_count * polygon_count);
			//Verify if the polygon is degenerate, in this case do not add them
			{
				float comparision_threshold = (float)import_param_->remove_degenerate_triangles ? SMALL_NUMBER : 0.f;
				P.clear();
				P.resize(polygon_vertex_count);
				for (int corner_index = 0; corner_index < polygon_vertex_count; ++corner_index)
				{
					const int control_point_index = mesh->GetPolygonVertex(polygon_index, corner_index);
					const int vertex_id = control_point_index;
					P[corner_index] = skin_mesh.control_points_[vertex_id];
				}
				const YVector normal = (P[1] - P[2]) ^ (P[0] - P[2]).GetSafeNormal(comparision_threshold);
				if (normal.IsNearlyZero(comparision_threshold) || normal.ContainsNaN())
				{
					WARNING_INFO("degenrate triangle");
					continue;
				}

				for (int corner_index = 0; corner_index < polygon_vertex_count; ++corner_index, ++triangle_control_index)
				{
					VertexWedge wedge;
					const int control_point_index = mesh->GetPolygonVertex(polygon_index, corner_index);
					wedge.control_point_id = control_point_index;
					wedge.position = skin_mesh.control_points_[control_point_index];
					wedge.weights_and_ids = skin_mesh.bone_weights_and_id_[control_point_index];

					//uv
					wedge.uvs_.resize(8);
					for (int uv_layer_index = 0; uv_layer_index < fbx_uvs.unique_count; ++uv_layer_index)
					{
						YVector2 final_uv_vector(0.0, 0.0);
						if (fbx_uvs.layer_element_uv[uv_layer_index] != nullptr)
						{
							int uv_map_index = fbx_uvs.uv_mapping_mode[uv_layer_index] == FbxLayerElement::eByControlPoint ? control_point_index : triangle_control_index;
							int uv_index = fbx_uvs.uv_reference_mode[uv_layer_index] == FbxLayerElement::eDirect ? uv_map_index : fbx_uvs.layer_element_uv[uv_layer_index]->GetIndexArray().GetAt(uv_map_index);
							FbxVector2 uv_vector = fbx_uvs.layer_element_uv[uv_layer_index]->GetDirectArray().GetAt(uv_index);
							int uv_counts = fbx_uvs.layer_element_uv[uv_layer_index]->GetDirectArray().GetCount();
							final_uv_vector.x = static_cast<float>(uv_vector[0]);
							final_uv_vector.y = 1.f - static_cast<float>(uv_vector[1]);   //flip the Y of UVs for DirectX
						}
						wedge.uvs_[uv_layer_index] = final_uv_vector;
					}

					if (normal_layer)
					{
						//normals may have different reference and mapping mode than tangents and binormals
						int normal_map_index = (normal_mapping_mode == FbxLayerElement::eByControlPoint) ? control_point_index : triangle_control_index;
						int normal_value_index = (normal_reference_mode == FbxLayerElement::eDirect) ? normal_map_index : normal_layer->GetIndexArray().GetAt(normal_map_index);

						FbxVector4 temp_value = normal_layer->GetDirectArray().GetAt(normal_value_index);
						temp_value = total_matrix_for_normal.MultT(temp_value);
						YVector tangent_z = converter_.ConvertDir(temp_value);
						wedge.normal = tangent_z.GetSafeNormal();
					}
					skin_mesh.wedges_.push_back(wedge);
				}
			}
		}
	}
	return skin_data;
}

struct SplitMeshByBone
{
	std::vector<VertexWedge> wedges;
	std::vector<int> bone_map;
	std::unordered_set<int> bone_set;
	bool InBoneSet(const std::vector<BoneWeightAndID>& bone_ids)
	{
		bool in_bone_set = true;
		for (const BoneWeightAndID& bone_id : bone_ids)
		{
			if (!bone_set.count(bone_id.id))
			{
				in_bone_set = false;
				break;
			}
		}
		return in_bone_set;
	}
	bool InBoneSet(const VertexWedge& v0, const VertexWedge& v1, const VertexWedge& v2)
	{
		return InBoneSet(v0.weights_and_ids) && InBoneSet(v1.weights_and_ids) && InBoneSet(v2.weights_and_ids);
	}
	bool AddBoneSet(const VertexWedge& v0, const VertexWedge& v1, const VertexWedge& v2, const int max_bone_per_scetion)
	{
		bool success = true;
		std::unordered_set<int> tmp_add = bone_set;
		for (const BoneWeightAndID& bone_weight_and_id : v0.weights_and_ids)
		{
			tmp_add.insert(bone_weight_and_id.id);
		}
		for (const BoneWeightAndID& bone_weight_and_id : v1.weights_and_ids)
		{
			tmp_add.insert(bone_weight_and_id.id);
		}
		for (const BoneWeightAndID& bone_weight_and_id : v2.weights_and_ids)
		{
			tmp_add.insert(bone_weight_and_id.id);
		}
		// 当前section塞不下了
		if (tmp_add.size() > max_bone_per_scetion)
		{
			success = false;
		}
		else {
			bone_set.swap(tmp_add);
			assert(InBoneSet(v0, v1, v2));
			wedges.push_back(v0);
			wedges.push_back(v1);
			wedges.push_back(v2);
		}
		return success;
	}

	void Process()
	{
		std::unordered_map<int, int> bone_mapping_index;
		bone_map.reserve(bone_set.size());
		for (int bone_id : bone_set)
		{
			bone_map.push_back(bone_id);
		}
		std::sort(bone_map.begin(), bone_map.end());
		for (int i = 0; i < bone_map.size(); ++i)
		{
			bone_mapping_index[bone_map[i]] = i;
		}
		
		for (VertexWedge& wedge : wedges)
		{
			for (BoneWeightAndID& bone_w_i : wedge.weights_and_ids)
			{
				bone_w_i.id = bone_mapping_index.at(bone_w_i.id);
			}
		}
	}
	
	void Compress()
	{
		
	}
};

struct SplitMeshByBoneContainer
{
	std::vector<SplitMeshByBone> cached_meshes;
	
	void AddWedge(const VertexWedge& v0, const VertexWedge& v1, const VertexWedge& v2, int material_index, const int max_bone_per_scetion)
	{
		bool success_add = false;
		for (SplitMeshByBone& mesh_section : cached_meshes)
		{
			bool add_success = mesh_section.AddBoneSet(v0, v1, v2, max_bone_per_scetion);
			if (add_success)
			{
				success_add = true;
				break;
			}
		}
		if (success_add)
		{
			return;
		}
		else
		{
			cached_meshes.push_back(SplitMeshByBone());
			AddWedge(v0, v1, v2, material_index, max_bone_per_scetion);
		}
	}
};

static std::unique_ptr<RenderData> GenerateRenderData(const std::vector< SplitMeshByBoneContainer>& skin_split_bone_mesh_containers, const int max_bone_per_scetion)
{
	int reserve_triangle_count=0;
	for (const SplitMeshByBoneContainer& split_mesh_by_bone_container : skin_split_bone_mesh_containers)
	{
		for (const SplitMeshByBone& slit_mesh_by_bone : split_mesh_by_bone_container.cached_meshes)
		{
			reserve_triangle_count+= slit_mesh_by_bone.wedges.size() / 3;
		}
	}
	std::unique_ptr<RenderData> render_data = std::make_unique<RenderData>();
	std::vector<int> indexes_all;
	indexes_all.reserve(reserve_triangle_count * 3);
	std::vector<int> section_start;
	std::vector<int> triangle_count;
	std::vector<VertexWedge> vertex_wedge_all;
	vertex_wedge_all.reserve(reserve_triangle_count * 3);
	std::vector<std::vector<int>> bone_mappings;
	int index = 0;
	for (const SplitMeshByBoneContainer& split_mesh_by_bone_container : skin_split_bone_mesh_containers)
	{
		for (const SplitMeshByBone& slit_mesh_by_bone : split_mesh_by_bone_container.cached_meshes)
		{
			section_start.push_back(index);
			bone_mappings.push_back(slit_mesh_by_bone.bone_map);
			int triange_conunt_section = slit_mesh_by_bone.wedges.size() / 3;
			triangle_count.push_back(triange_conunt_section);
			for (int i = 0; i < triange_conunt_section * 3; ++i)
			{
				indexes_all.push_back(index++);
			}
			vertex_wedge_all.insert(vertex_wedge_all.end(), slit_mesh_by_bone.wedges.begin(), slit_mesh_by_bone.wedges.end());
		}
	}
	// 
	int section_count = section_start.size();
	float section_color_channel = 1.0 / (float)section_count*3.0;
	int section_per_color = YMath::Max(section_count / 3,1);
	int current_section_index = 0;
	index = 0;
	for (const SplitMeshByBoneContainer& split_mesh_by_bone_container : skin_split_bone_mesh_containers)
	{
		for (const SplitMeshByBone& slit_mesh_by_bone : split_mesh_by_bone_container.cached_meshes)
		{
			int color_index = current_section_index / section_per_color;
			int color_index_off = current_section_index % section_per_color;
			YVector4 current_color = YVector4(1.0, 0.0, 0.0, 1.0);
			if (color_index == 0)
			{
				current_color = YVector4(section_color_channel * current_section_index, 0.0, 0.0, 1.0);
			}
			else if (color_index == 1)
			{
				current_color = YVector4(0.0,section_color_channel * current_section_index, 0.0, 1.0);
			}
			else if (color_index == 2)
			{
				current_color = YVector4(0.0, 0.0, section_color_channel * current_section_index, 1.0);
			}
			int triange_conunt_section = slit_mesh_by_bone.wedges.size() / 3;
			for (int i = 0; i < triange_conunt_section * 3; ++i)
			{
				vertex_wedge_all[index++].color = current_color;
			}
			current_section_index++;
		}
	}//
	const int vertex_count = vertex_wedge_all.size();
	render_data->position.resize(vertex_count);
	render_data->normal.resize(vertex_count);
	render_data->uv.resize(vertex_count);
	render_data->weights.resize(vertex_count);
	render_data->bone_id.resize(vertex_count);
	render_data->color.resize(vertex_count);
	for (int wedge_index = 0; wedge_index < vertex_count; ++wedge_index)
	{
		render_data->position[wedge_index] = vertex_wedge_all[wedge_index].position;
		render_data->normal[wedge_index] = vertex_wedge_all[wedge_index].normal;
		render_data->color[wedge_index] = vertex_wedge_all[wedge_index].color;
		render_data->uv[wedge_index][0] = vertex_wedge_all[wedge_index].uvs_[0];
		if (vertex_wedge_all[wedge_index].uvs_.size() > 1)
		{
			render_data->uv[wedge_index][1] = vertex_wedge_all[wedge_index].uvs_[1];
		}
		else
		{
			render_data->uv[wedge_index][1] = vertex_wedge_all[wedge_index].uvs_[0];
		}
		int bone_id_index = 0;
		for (; bone_id_index < vertex_wedge_all[wedge_index].weights_and_ids.size(); ++bone_id_index)
		{
			render_data->weights[wedge_index][bone_id_index] = vertex_wedge_all[wedge_index].weights_and_ids[bone_id_index].weight;
			render_data->bone_id[wedge_index][bone_id_index] = vertex_wedge_all[wedge_index].weights_and_ids[bone_id_index].id;
		}
		for (;bone_id_index < 8; ++bone_id_index)
		{
			render_data->weights[wedge_index][bone_id_index] = 0.0;
			render_data->bone_id[wedge_index][bone_id_index] = render_data->bone_id[wedge_index][0];
		}
	}

	render_data->indices = indexes_all;
	render_data->sections = section_start;
	render_data->triangle_counts = triangle_count;
	render_data->bone_mapping = bone_mappings;
	return render_data;
}
bool YFbxImporter::PostProcessSkeletonMesh(YSkeletonMesh* skeleton_mesh)
{
	assert(skeleton_mesh);
	YSkinDataImported* skin_data = skeleton_mesh->skin_data_.get();
	
	//split mesh by bone
	const int max_bone = 16;
	//const int max_bone = 256; // 256*64 = 16384, constant buffer is 65535 bytes

	std::vector<SkinMesh>& skin_meshes = skin_data->meshes_;
	std::vector< SplitMeshByBoneContainer> skin_split_bone_mesh_containers;
	skin_split_bone_mesh_containers.resize(skin_meshes.size());
	for (int skin_mesh_index = 0; skin_mesh_index < skin_meshes.size(); ++skin_mesh_index)
	{
		SkinMesh& skin_mesh = skin_meshes[skin_mesh_index];
		int triangle_num = 0;
		SplitMeshByBoneContainer& split_mesh_by_bone_container = skin_split_bone_mesh_containers[skin_mesh_index];
		for (int triangle_index = 0; triangle_index * 3 < skin_mesh.wedges_.size(); ++triangle_index)
		{
			int wedge_index_base = triangle_index * 3;
			std::unordered_set<int> tmp_add;
			for (int i = 0; i < 3; ++i) {
				VertexWedge& v0 = skin_mesh.wedges_[wedge_index_base + i];
				for (const BoneWeightAndID& bone_weight_and_id : v0.weights_and_ids)
				{
					tmp_add.insert(bone_weight_and_id.id);
				}
			}
			if (tmp_add.size() > import_param_->max_bone_per_section)
			{
				ERROR_INFO(StringFormat("a triangle contains %d bones, more than max bone per section %d, please increase max bone per section or fix fbx", tmp_add.size(),import_param_->max_bone_per_section));
				return false;
			}
			split_mesh_by_bone_container.AddWedge(skin_mesh.wedges_[wedge_index_base + 0], skin_mesh.wedges_[wedge_index_base + 1], skin_mesh.wedges_[wedge_index_base + 2], 0, import_param_->max_bone_per_section);
		}
		for (SplitMeshByBone& split_mesh_by_bone : split_mesh_by_bone_container.cached_meshes)
		{
			split_mesh_by_bone.Process();
		}
	}
	skeleton_mesh->render_data_ =  GenerateRenderData(skin_split_bone_mesh_containers, import_param_->max_bone_per_section);
	return true;
}


