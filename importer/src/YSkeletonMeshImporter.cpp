#include "YFbxImporter.h"
#include "fbxsdk/scene/geometry/fbxdeformer.h"
#include <algorithm>
#include "Engine/YSkeletonMesh.h"
#include <deque>
#include "fbxsdk/scene/geometry/fbxcluster.h"

struct ConverterBone :public YBone
{
	std::set<FbxNode*> children_fbx_;
	FbxNode* parent;

};

inline bool IsABone(FbxNode* node) 
{
	if (node && node->GetNodeAttribute() && (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton || 
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eNull ||
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh))
	{
		return true;
	}
	return false;
}
std::unique_ptr<YSkeleton> YFbxImporter::BuildSkeleton(std::set<FbxNode*>& nodes, std::unordered_map<int, FbxNode*>& out_map)
{

	std::unique_ptr<YSkeleton> skeleton = std::make_unique<YSkeleton>();

	//find root_node
	auto find_root_node = [](FbxNode* node) {
		FbxNode* root_node = node;
		while (root_node->GetParent() && IsABone(root_node->GetParent()))
		{
			root_node = root_node->GetParent();
		}
		return root_node;
	};

	std::set<FbxNode*> skeleton_root_node;
	for (auto fbx_node : nodes)
	{

		skeleton_root_node.insert(find_root_node(fbx_node));
	}

	if (skeleton_root_node.size() != 1) {
		ERROR_INFO("skeleton has multi root nodes, there are");
		for (FbxNode* node : skeleton_root_node)
		{
			ERROR_INFO("	,node name is ", node->GetName());
		}
		return nullptr;
	}

	// build connection
	std::unordered_map<FbxNode*, std::unique_ptr<ConverterBone>> bone_cache;
	for (FbxNode* fbx_node : nodes)
	{
		std::unique_ptr<ConverterBone> bone = std::make_unique<ConverterBone>();
		bone->bone_name_ = fbx_node->GetName();
		bone_cache[fbx_node] = std::move(bone);
	}

	for (FbxNode* fbx_node : nodes)
	{
		std::unique_ptr<ConverterBone>& bone = bone_cache[fbx_node];
		FbxNode* parent = fbx_node->GetParent();

		if (parent && IsABone(parent)) {
			std::unique_ptr<ConverterBone>& parent_bone = bone_cache[parent];
			parent_bone->children_fbx_.insert(fbx_node);
			bone->parent = parent;
		}
	}

	FbxNode* root_node = *(skeleton_root_node.begin());

	std::deque<FbxNode*> node_queue;
	node_queue.push_back(root_node);
	skeleton->bones_.reserve(nodes.size());
	while (!node_queue.empty())
	{
		FbxNode* node = node_queue.front();
		node_queue.pop_front();
		std::unique_ptr<ConverterBone>& bone = bone_cache[node];
		YBone new_bone;

		new_bone.bone_name_ = bone->bone_name_;

		int bone_id = skeleton->bones_.size();
		new_bone.bone_id_ = bone_id;
		skeleton->bones_.push_back(new_bone);
		if (skeleton->bone_names_to_id_.count(new_bone.bone_name_) != 0) {
			ERROR_INFO("bones have the same name ", new_bone.bone_name_);
			return nullptr;
		}
		else
		{
			skeleton->bone_names_to_id_[new_bone.bone_name_] = bone_id;
		}
		for (FbxNode* child_node : bone->children_fbx_)
		{
			node_queue.push_back(child_node);
		}
		out_map[bone_id] = node;
		FbxNode* parent = node->GetParent();
		if (parent && IsABone(parent))
		{
			int parent_id = skeleton->bone_names_to_id_[parent->GetName()];
			YBone& parent_bone = skeleton->bones_[parent_id];
			parent_bone.children_.push_back(bone_id);
			skeleton->bones_[bone_id].parent_id_ = parent_id;
			FbxAMatrix parent_global_trans = parent->EvaluateGlobalTransform();
			FbxAMatrix child_global_trans = node->EvaluateGlobalTransform();
			FbxAMatrix local_trans = (parent_global_trans.Inverse()) * child_global_trans;
			skeleton->bones_[bone_id].bind_local_tranform_ = converter_.ConverterFbxTransform(local_trans);
			skeleton->bones_[bone_id].bind_local_matrix_ = converter_.ConvertFbxMatrix(local_trans);
			skeleton->bones_[bone_id].local_transform_ = skeleton->bones_[bone_id].bind_local_tranform_;
		}
		else
		{
			FbxAMatrix global_trans = node->EvaluateGlobalTransform();
			skeleton->bones_[bone_id].bind_local_tranform_ = converter_.ConverterFbxTransform(global_trans);
			skeleton->bones_[bone_id].bind_local_matrix_ = converter_.ConvertFbxMatrix(global_trans);
		}
		skeleton->bones_[bone_id].local_transform_ = skeleton->bones_[bone_id].bind_local_tranform_;
		skeleton->bones_[bone_id].local_matrix_ = skeleton->bones_[bone_id].bind_local_matrix_;
	}
	skeleton->root_bone_id_ = skeleton->bone_names_to_id_[root_node->GetName()];
	return skeleton;
}


std::unique_ptr<YSkeletonMesh> YFbxImporter::ImportSkeletonMesh(FbxNode* root_node, const std::string& mesh_name)
{
	//import skeleton
	// 找到skeleton对应cluster引用的bone
	std::set<FbxNode*> bone_attach_to_skin;
	for (int gemometry_index = 0; gemometry_index < fbx_scene_->GetGeometryCount(); ++gemometry_index)
	{
		FbxGeometry* geometry = fbx_scene_->GetGeometry(gemometry_index);
		if (geometry->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			FbxNode* geo_node = geometry->GetNode();
			FbxMesh* mesh = FbxCast<FbxMesh>(geometry);
			//lod todo 
			if (mesh->GetDeformerCount(FbxDeformer::EDeformerType::eSkin) > 0)
			{
				FbxSkin* skin = FbxCast<FbxSkin>(mesh->GetDeformer(0, FbxDeformer::eSkin));
				int cluster_count = skin->GetClusterCount();
				FbxNode* link = nullptr;
				for (int cluster_index = 0; cluster_index < cluster_count; ++cluster_index)
				{
					FbxCluster* cluster = skin->GetCluster(cluster_index);
					link = cluster->GetLink();
					bone_attach_to_skin.insert(link);
					while (link && IsABone(link))
					{
						bone_attach_to_skin.insert(link);
						link = link->GetParent();
					}
				}
			}
		}
	}

	int node_count = fbx_scene_->GetNodeCount();
	std::set<FbxNode*> skeleton_nodes_in_scene;
	for (int node_index = 0; node_index < node_count; ++node_index)
	{
		FbxNode* node = fbx_scene_->GetNode(node_index);
		//if (node->GetNodeAttribute() && (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton || node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMarker))
		if(IsABone(node))
		{
			skeleton_nodes_in_scene.insert(node);
		}
	}
	std::set<FbxNode*> skeleton_nodes;
	std::set_intersection(bone_attach_to_skin.begin(), bone_attach_to_skin.end(), skeleton_nodes_in_scene.begin(), skeleton_nodes_in_scene.end(), std::inserter(skeleton_nodes, skeleton_nodes.begin()));
	std::unordered_map<int, FbxNode*> bone_id_to_fbx_node;
	std::unique_ptr<YSkeleton> skeleton = BuildSkeleton(skeleton_nodes,bone_id_to_fbx_node);
	skeleton->Update(0.0); // test
	std::unique_ptr<AnimationData> animation_data = ImportAnimationData(skeleton.get(),bone_id_to_fbx_node);

	std::unique_ptr<YSkeletonMesh> skeleton_mesh = std::make_unique<YSkeletonMesh>();
	skeleton_mesh->skeleton_ = std::move(skeleton);
	skeleton_mesh->animation_data_ = std::move(animation_data);
	std::unique_ptr<YSkinData> skin_data = ImportSkinData(skeleton_mesh->skeleton_.get());
	skeleton_mesh->skin_data_ = std::move(skin_data);

	return skeleton_mesh;
}

std::unique_ptr<YSkinData> YFbxImporter::ImportSkinData(YSkeleton* skeleton)
{
	std::unique_ptr<YSkinData> skin_data = std::make_unique<YSkinData>();
	
	for (int gemometry_index = 0; gemometry_index < fbx_scene_->GetGeometryCount(); ++gemometry_index)
	{
		skin_data->meshes_.push_back(SkinMesh());
		SkinMesh& skin_mesh = skin_data->meshes_.back();
		FbxGeometry* geometry = fbx_scene_->GetGeometry(gemometry_index);
		if (geometry->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			FbxNode* geo_node = geometry->GetNode();
			FbxMesh* mesh = FbxCast<FbxMesh>(geometry);
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
			for (int vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
			{
				int real_vertex_index = vertex_index;
				FbxVector4 fbx_position = mesh->GetControlPoints()[vertex_index];
				fbx_position = total_matrix.MultT(fbx_position);
				const YVector vertex_position = converter_.ConvertPos(fbx_position);
				skin_mesh.control_points_.push_back(vertex_position);
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
					
					for (int corner_index = 0; corner_index < polygon_vertex_count; ++corner_index,++triangle_control_index)
					{
						VertexWedge wedge;
						const int control_point_index = mesh->GetPolygonVertex(polygon_index, corner_index);
						wedge.control_point_id = control_point_index;
						wedge.position = skin_mesh.control_points_[control_point_index];

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
							wedge.normal = tangent_z;
						}
						skin_mesh.wedges_.push_back(wedge);
					}
				}
			}

			if (mesh->GetDeformerCount(FbxDeformer::EDeformerType::eSkin) > 0)
			{
				FbxAMatrix test_mesh_to_model;
				bool first_mesh_to_model = false;
				int32 SkinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
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
						FbxAMatrix inv_bone_bind = bone_to_model.Inverse();
						if (!bone.fist_init)
						{
							//bone.fbx_inv_bind_matrix_ = inv_bone_bind;
							bone.inv_bind_global_transform_ = converter_.ConverterFbxTransform(inv_bone_bind);
							bone.inv_bind_global_matrix_ = converter_.ConvertFbxMatrix(inv_bone_bind);
							bone.fist_init = true;
						}
						else
						{
							//assert(inv_bone_bind == bone.fbx_inv_bind_matrix_);
							YMatrix test_matrix = converter_.ConvertFbxMatrix(inv_bone_bind);
							assert(test_matrix.Equals(bone.inv_bind_global_matrix_));
						}
					}
				}
			}
		}
	}
	return skin_data;
}


std::unique_ptr<AnimationData> YFbxImporter::ImportAnimationData(YSkeleton* skeleton, std::unordered_map<int, FbxNode*>& bone_id_to_fbxnode)
{

	std::unique_ptr<AnimationData> animation_data = std::make_unique<AnimationData>();
	int32 ResampleRate = 30.0f;
	int32 AnimStackCount = fbx_scene_->GetSrcObjectCount<FbxAnimStack>();
	if (AnimStackCount == 0)
	{
		return nullptr;
	}

	float time = -1;
	int frame_rate = FbxTime::GetFrameRate(fbx_scene_->GetGlobalSettings().GetTimeMode());
	int AnimStackIndex = 0;
	//for (int32 AnimStackIndex = 0; AnimStackIndex < AnimStackCount; AnimStackIndex++)
	FbxAnimStack* CurAnimStack = fbx_scene_->GetSrcObject<FbxAnimStack>(AnimStackIndex);
	FbxTimeSpan AnimTimeSpan = CurAnimStack->GetLocalTimeSpan();
	if (AnimTimeSpan.GetDirection() == FBXSDK_TIME_FORWARD)
	{
		time = YMath::Max((float)(AnimTimeSpan.GetDuration().GetMilliSeconds() / 1000.0f * scene_info_->frame_rate), time);
	}
	animation_data->name_ = CurAnimStack->GetName();

	const int32 NumSamplingKeys = YMath::FloorToInt(AnimTimeSpan.GetDuration().GetSecondDouble() * ResampleRate);
	animation_data->time_ = NumSamplingKeys * (1.0/(float)frame_rate);
	const FbxTime TimeIncrement = AnimTimeSpan.GetDuration() / YMath::Max(NumSamplingKeys, 1);
	for (YBone& bone : skeleton->bones_)
	{
		AnimationSequenceTrack& animation_sequence_track = animation_data->sequence_track[bone.bone_name_];
		animation_sequence_track.pos_keys_.reserve(NumSamplingKeys);
		animation_sequence_track.rot_keys_.reserve(NumSamplingKeys);
		animation_sequence_track.scale_keys_.reserve(NumSamplingKeys);
		for (FbxTime CurTime = AnimTimeSpan.GetStart(); CurTime <= AnimTimeSpan.GetStop(); CurTime += TimeIncrement)
		{
			FbxNode* current_node = bone_id_to_fbxnode[bone.bone_id_];
			FbxAMatrix cur_global_trans = current_node->EvaluateGlobalTransform(CurTime);
			YTransform cur_local_transform;
			if (bone.parent_id_ == -1)
			{
				cur_local_transform = converter_.ConverterFbxTransform(cur_global_trans);
			}
			else
			{
				FbxNode* parent_node = bone_id_to_fbxnode[bone.parent_id_];
				FbxAMatrix parent_cur_global_trans = parent_node->EvaluateGlobalTransform(CurTime);
				FbxAMatrix local_trans = (parent_cur_global_trans.Inverse()) * cur_global_trans;
				cur_local_transform = converter_.ConverterFbxTransform(local_trans);
			}
			animation_sequence_track.pos_keys_.push_back(cur_local_transform.translation);
			animation_sequence_track.rot_keys_.push_back(cur_local_transform.rotator);
			animation_sequence_track.scale_keys_.push_back(cur_local_transform.scale);
		}
	}
	return animation_data;
}
