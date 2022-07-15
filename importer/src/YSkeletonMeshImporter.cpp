#include "YFbxImporter.h"
#include "fbxsdk/scene/geometry/fbxdeformer.h"
#include <algorithm>
#include "Engine/YSkeletonMesh.h"
#include <deque>
#include "fbxsdk/scene/geometry/fbxcluster.h"
#include "fbxsdk/scene/geometry/fbxblendshape.h"
#include "Engine/YLog.h"
#include "Utility/YStringFormat.h"

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
	std::unique_ptr<YSkinData> skin_data = ImportSkinData(skeleton_mesh->skeleton_.get(), mesh_contain_skeleton_and_bs);
	skeleton_mesh->skin_data_ = std::move(skin_data);
	ImportBlendShapeAnimation(skeleton_mesh->skin_data_.get(), skeleton_mesh->animation_data_.get(), mesh_contain_skeleton_and_bs);
	if (!skeleton_mesh->AllocGpuResource())
	{
		ERROR_INFO("SkeletonMesh alloc resource failed!!");
	}

	return skeleton_mesh;
}

std::unique_ptr<YSkinData> YFbxImporter::ImportSkinData(YSkeleton* skeleton, const std::vector<FbxNode*>& mesh_contain_skeleton_and_bs)
{
	LOG_INFO("ImportSkinData");
	std::unique_ptr<YSkinData> skin_data = std::make_unique<YSkinData>();
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
			WARNING_INFO("Triangulation static mesh", geo_node->GetName());
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
		skin_mesh.weights_.resize(vertex_count);
		skin_mesh.bone_index_.resize(vertex_count);
		/*	for (int vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
			{
				FbxVector4 fbx_position = mesh->GetControlPoints()[vertex_index];
				fbx_position = total_matrix.MultT(fbx_position);
				const YVector vertex_position = converter_.ConvertPos(fbx_position);
				skin_mesh.control_points_.push_back(vertex_position);
			}*/

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
						skin_mesh.weights_[control_point_index].push_back(contorl_point_weight);
						skin_mesh.bone_index_[control_point_index].push_back(bone.bone_id_);
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

		if (mesh->GetShapeCount() > 0)
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
					wedge.weights_ = skin_mesh.weights_[control_point_index];
					wedge.bone_index_ = skin_mesh.bone_index_[control_point_index];

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



