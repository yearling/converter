#include "YFbxImporter.h"
#include "fbxsdk/scene/geometry/fbxdeformer.h"
#include <algorithm>
#include "Engine/YSkeletonMesh.h"
#include <deque>
#include "fbxsdk/scene/geometry/fbxcluster.h"
#include "fbxsdk/scene/geometry/fbxblendshape.h"
#include "Engine/YLog.h"
#include "Utility/YStringFormat.h"

struct ConverterBone :public YBone
{
	std::set<FbxNode*> children_fbx_;
	FbxNode* parent;
	FbxNode* self_node;
};
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
inline bool IsBone(FbxNode* node)
{
	if (node && node->GetNodeAttribute() && (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton ||
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eNull ||
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh))
	{
		return true;
	}
	return false;
}
FbxNode* YFbxImporter::GetRootSketeton(FbxNode* link)
{
	FbxNode* root_bone = link;

	// get Unreal skeleton root
	// mesh and dummy are used as bone if they are in the skeleton hierarchy
	while (root_bone && root_bone->GetParent())
	{
		
		FbxNodeAttribute* Attr = root_bone->GetParent()->GetNodeAttribute();
		if (Attr &&
			(Attr->GetAttributeType() == FbxNodeAttribute::eMesh ||
				(Attr->GetAttributeType() == FbxNodeAttribute::eNull) ||
				Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton) &&
			root_bone->GetParent() != fbx_scene_->GetRootNode())
		{
			// in some case, skeletal mesh can be ancestor of bones
			// this avoids this situation
			if (Attr->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				FbxMesh* Mesh = (FbxMesh*)Attr;
				if (Mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
				{
					break;
				}
			}

			root_bone = root_bone->GetParent();
		}
		else
		{
			break;
		}
	}

	return root_bone;
}


void YFbxImporter::RecursiveBuildSkeleton(FbxNode* link, std::set<FbxNode*>& out_bones)
{
	if (IsBone(link))
	{
		out_bones.insert(link);
		for (int child_index = 0; child_index < link->GetChildCount(); ++child_index)
		{
			RecursiveBuildSkeleton(link->GetChild(child_index), out_bones);
		}
	}
}


std::unique_ptr<YSkeleton> YFbxImporter::BuildSkeleton(FbxNode* root_node, std::unordered_map<int, FbxNode*>& out_map)
{
	bool used_time0_as_bind_pose = false;
	std::vector<FbxPose*> pose_array;
	std::vector<FbxNode*> mesh_contain_skeleton_and_bs;
	RecursiveFindMesh(root_node, mesh_contain_skeleton_and_bs);
	if (!RetrievePoseFromBindPose(mesh_contain_skeleton_and_bs, pose_array))
	{
		WARNING_INFO("Getting valid bind pose failed. Try to recreate bind pose");
		const int pose_count = fbx_scene_->GetPoseCount();
		for (int pose_index = pose_count-1; pose_index >=0; --pose_index)
		{
			FbxPose* current_pose = fbx_scene_->GetPose(pose_index);
			if (current_pose && current_pose->IsBindPose())
			{
				fbx_scene_->RemovePose(current_pose);
				current_pose->Destroy();
			}
		}
		fbx_manager_->CreateMissingBindPoses(fbx_scene_);
		if (RetrievePoseFromBindPose(mesh_contain_skeleton_and_bs, pose_array))
		{
			WARNING_INFO("Recreating bind pose succeeded.");
		}
		else
		{
			WARNING_INFO("Recreating bind pose failed.");
		}
	}
	if (!used_time0_as_bind_pose && pose_array.empty())
	{
		used_time0_as_bind_pose = true;
	}

	// import skeleton
	// 找到skeleton对应cluster引用的bone
	std::set<FbxNode*> bones_attach_to_skin;
	std::set<FbxNode*> skeleton_nodes_in_scene;
	std::set<FbxCluster*> clusters;
	std::set<FbxNode*> skeleton_root_nodes;
	int node_count = fbx_scene_->GetNodeCount();
	for (int node_index = 0; node_index < node_count; ++node_index)
	{
		FbxNode* node = fbx_scene_->GetNode(node_index);
		if (IsBone(node))
		{
			skeleton_nodes_in_scene.insert(node);
		}

		FbxGeometry* geometry = node->GetGeometry();
		if (!geometry)
		{
			continue;
		}
		if (geometry->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
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
					skeleton_root_nodes.insert(GetRootSketeton(link));
					clusters.insert(cluster);
					bones_attach_to_skin.insert(link);
					while (link && IsBone(link))
					{
						bones_attach_to_skin.insert(link);
						link = link->GetParent();
					}
				}
			}
		}
	}

	
	std::set<FbxNode*> skeleton_nodes;
	for (FbxNode* root_node : skeleton_root_nodes)
	{
		RecursiveBuildSkeleton(root_node, skeleton_nodes);
	}

	std::unique_ptr<YSkeleton> skeleton = std::make_unique<YSkeleton>();
	//find root_node
	if (skeleton_root_nodes.size() != 1) {
		ERROR_INFO("skeleton has multi root nodes, there are");
		for (FbxNode* node : skeleton_root_nodes)
		{
			ERROR_INFO("	,node name is ", node->GetName());
		}
		return nullptr;
	}

	// build connection
	std::unordered_map<FbxNode*, std::unique_ptr<ConverterBone>> bone_cache;
	for (FbxNode* fbx_node : skeleton_nodes)
	{
		std::unique_ptr<ConverterBone> bone = std::make_unique<ConverterBone>();
		bone->bone_name_ = fbx_node->GetName();
		bone->self_node = fbx_node;
		bone_cache[fbx_node] = std::move(bone);
	}

	// 建立父子关系 
	for (FbxNode* fbx_node : skeleton_nodes)
	{
		std::unique_ptr<ConverterBone>& bone = bone_cache[fbx_node];
		FbxNode* parent = fbx_node->GetParent();

		if (parent && IsBone(parent)) {
			std::unique_ptr<ConverterBone>& parent_bone = bone_cache[parent];
			parent_bone->children_fbx_.insert(fbx_node);
			bone->parent = parent;
		}
	}

	FbxNode* skeleton_root_node = *(skeleton_root_nodes.begin());

	std::deque<FbxNode*> node_queue;
	node_queue.push_back(skeleton_root_node);
	skeleton->bones_.reserve(skeleton_nodes.size());
	
	std::vector<ConverterBone*> sorted_bones;
	sorted_bones.reserve(skeleton_nodes.size());
	assert(bone_cache.size() == skeleton_nodes.size());
	
	while (!node_queue.empty())
	{
		FbxNode* node = node_queue.front();
		node_queue.pop_front();
		std::unique_ptr<ConverterBone>& bone = bone_cache[node];
		int bone_id = sorted_bones.size();
		bone->bone_id_ = bone_id;
		sorted_bones.push_back(bone.get());

		for (FbxNode* child_node : bone->children_fbx_)
		{
			node_queue.push_back(child_node);
		}
		out_map[bone_id] = node;
		FbxNode* parent = node->GetParent();
		if (parent && IsBone(parent))
		{
			int parent_id = bone_cache.at(parent)->bone_id_;
			assert(parent_id != INDEX_NONE);
			bone->parent_id_ = parent_id;
			ConverterBone* parent_bone = sorted_bones[bone->parent_id_];
			parent_bone->children_.push_back(bone_id);
		}
	}
	int root_index = INDEX_NONE;
	bool global_link_found_flag= false;
	std::vector<FbxAMatrix> raw_bone_global_matrix;
	raw_bone_global_matrix.resize(sorted_bones.size(), FbxAMatrix());
	bool any_bone_not_in_bind_pose = false;
	std::string bone_without_bind_pose;
	for (int bone_index = 0; bone_index < bone_cache.size(); ++bone_index)
	{
		global_link_found_flag = false;
		ConverterBone* bone = sorted_bones[bone_index];
		if (pose_array.size() > 0)
		{
			for(int pose_index=0;pose_index< pose_array.size();++pose_index)
			{
				int pose_link_index = pose_array[pose_index]->Find(bone->self_node);
				if (pose_link_index >= 0)
				{
					FbxMatrix none_affine_matrix = pose_array[pose_index]->GetMatrix(pose_link_index);
					FbxAMatrix matrix = *(FbxAMatrix*)(double*)&none_affine_matrix;
					raw_bone_global_matrix[bone_index] = matrix;
					global_link_found_flag = true;
					break;
				}
			}
		}
		if (!global_link_found_flag)
		{
			if (!used_time0_as_bind_pose)
			{
				any_bone_not_in_bind_pose = true;
				bone_without_bind_pose += bone->bone_name_;
				bone_without_bind_pose += ",    ";
			}

			for (FbxCluster* cluster : clusters)
			{
				if (bone->self_node == cluster->GetLink())
				{
					cluster->GetTransformLinkMatrix(raw_bone_global_matrix[bone_index]);
					global_link_found_flag = true;
					break;
				}
			}
		}
		if (!global_link_found_flag)
		{
			raw_bone_global_matrix[bone_index] = bone->self_node->EvaluateGlobalTransform();
		}

		if (used_time0_as_bind_pose)
		{
			raw_bone_global_matrix[bone_index] = fbx_scene_->GetAnimationEvaluator()->GetNodeGlobalTransform(bone->self_node, 0);
		}
	}
	if (any_bone_not_in_bind_pose)
	{
		WARNING_INFO(StringFormat("FbxSkeletaLMeshimport_BonesAreMissingFromBindPose,The following bones are missing from the bind pose : \n %s \nThis can happen for bones that are not vert weighted.If they are not in the correct orientation after importing, \nplease set the \"Use T0 as ref pose\" option or add them to the bind pose and reimport the skeletal mesh.", bone_without_bind_pose.c_str()));
	}
	for (int bone_index = 0; bone_index < bone_cache.size(); ++bone_index)
	{
		ConverterBone* convert_bone = sorted_bones[bone_index];
		assert(bone_index == skeleton->bones_.size());
		skeleton->bones_.push_back(YBone());
		YBone& final_bone  = skeleton->bones_.back();
		final_bone.bone_id_ = convert_bone->bone_id_;
		final_bone.parent_id_ = convert_bone->parent_id_;
		final_bone.bone_name_ = convert_bone->bone_name_;
		final_bone.children_ = convert_bone->children_;
		skeleton->bone_names_to_id_[final_bone.bone_name_] = final_bone.bone_id_;
		if (bone_index)
		{
			FbxAMatrix self_global_trans = raw_bone_global_matrix[bone_index];
			FbxAMatrix parent_global_trans = raw_bone_global_matrix[final_bone.parent_id_];
			FbxAMatrix local_trans = parent_global_trans.Inverse() * self_global_trans;
			final_bone.bind_local_tranform_ = converter_.ConverterFbxTransform(local_trans);
			final_bone.bind_local_matrix_ = converter_.ConvertFbxMatrix(local_trans);
		}
		else
		{
			FbxAMatrix local_trans = raw_bone_global_matrix[bone_index];
			final_bone.bind_local_tranform_ = converter_.ConverterFbxTransform(local_trans);
			final_bone.bind_local_matrix_ = converter_.ConvertFbxMatrix(local_trans);
		}
		final_bone.local_transform_ = final_bone.bind_local_tranform_;
		final_bone.local_matrix_ = final_bone.bind_local_matrix_; 
	}
	skeleton->root_bone_id_ = skeleton->bone_names_to_id_.at(skeleton_root_node->GetName());
	return skeleton;
}


bool YFbxImporter::RetrievePoseFromBindPose(const std::vector<FbxNode*>& mesh_nodes, std::vector<FbxPose*>& pose_array)
{
	const int pose_count = fbx_scene_->GetPoseCount();
	for (int pose_index = 0; pose_index < pose_count; ++pose_index)
	{
		FbxPose* current_pose = fbx_scene_->GetPose(pose_index);
		if (current_pose && current_pose->IsBindPose())
		{
			// IsValidBindPose doesn't work reliably
			// It checks all the parent chain(regardless root given), and if the parent doesn't have correct bind pose, it fails
			// It causes more false positive issues than the real issue we have to worry about
			// If you'd like to try this, set CHECK_VALID_BIND_POSE to 1, and try the error message
			// when Autodesk fixes this bug, then we might be able to re-open this
			std::string pose_name = current_pose->GetName();
			// all error report status
			FbxStatus Status;
			// it does not make any difference of checking with different node
			// it is possible pose 0 -> node array 2, but isValidBindPose function returns true even with node array 0
			for (FbxNode* current_node : mesh_nodes)
			{
				std::string mesh_name = current_node->GetName();
				NodeList missing_ancesteros, missing_deformers, missing_deformers_ancestors, wrong_matrix;
				if (current_pose->IsValidBindPoseVerbose(current_node, missing_ancesteros, missing_deformers, missing_deformers_ancestors, wrong_matrix))
				{
					pose_array.push_back(current_pose);
					LOG_INFO(StringFormat("Valid bind pose for Pose %s -- %s", pose_name.c_str(), mesh_name.c_str()));
					break;
				}
				else
				{
					// first try to fix up
					// add missing ancestors
					for (int i = 0; i < missing_ancesteros.GetCount(); i++)
					{
						FbxAMatrix mat = missing_ancesteros.GetAt(i)->EvaluateGlobalTransform(FBXSDK_TIME_ZERO);
						current_pose->Add(missing_ancesteros.GetAt(i), mat);
					}

					missing_ancesteros.Clear();
					missing_deformers.Clear();
					missing_deformers_ancestors.Clear();
					wrong_matrix.Clear();

					// check it again
					if (current_pose->IsValidBindPose(current_node))
					{
						pose_array.push_back(current_pose);
						LOG_INFO(StringFormat("Valid bind pose for Pose %s -- %s", pose_name.c_str(), mesh_name.c_str()));
						break;
					}
					else
					{
						// first try to find parent who is null group and see if you can try test it again
						FbxNode* parent_node = current_node->GetParent();
						while (parent_node)
						{
							FbxNodeAttribute* Attr = parent_node->GetNodeAttribute();
							if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eNull)
							{
								// found it 
								break;
							}

							// find next parent
							parent_node = parent_node->GetParent();
						}

						if (parent_node && current_pose->IsValidBindPose(parent_node))
						{
							pose_array.push_back(current_pose);
							LOG_INFO(StringFormat("Valid bind pose for Pose %s -- %s", pose_name.c_str(), mesh_name.c_str()));
							break;
						}
						else
						{
							std::string error_string = Status.GetErrorString();
							WARNING_INFO(StringFormat("Not valid bind pose for Pose (%s) - Node %s : %s", pose_name.c_str(), mesh_name.c_str(), error_string.c_str()));
						}
					}
				}
			}
		}
	}
	return pose_array.size() > 0;
}



std::unique_ptr<YSkeletonMesh> YFbxImporter::ImportSkeletonMesh(FbxNode* root_node, const std::string& mesh_name)
{

	std::unordered_map<int, FbxNode*> bone_id_to_fbx_node;
	std::unique_ptr<YSkeleton> skeleton = BuildSkeleton(root_node, bone_id_to_fbx_node);
	if (skeleton)
	{
		skeleton->Update(0.0); // test
	}
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
					FbxAMatrix inv_bone_bind = bone_to_model.Inverse() * mesh_to_model;
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

					int cluster_control_point_count = cluster->GetControlPointIndicesCount();
					//double* weight = cluster->GetControlPointWeights();
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
			//FbxVector4 fbx_model_position = test_mesh_to_model.MultT(fbx_position);
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

				int blend_shape_channel_count = blend_shpae_fbx->GetBlendShapeChannelCount();
				for (int i_blend_shape_channel = 0; i_blend_shape_channel < blend_shape_channel_count; ++i_blend_shape_channel)
				{
					BlendShapeTarget blend_shape_target;
					FbxBlendShapeChannel* channel = blend_shpae_fbx->GetBlendShapeChannel(i_blend_shape_channel);
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
							//FbxVector4 fbx_model_position = test_mesh_to_model.MultT(fbx_position);
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
	animation_data->time_ = NumSamplingKeys * (1.0 / (float)frame_rate);
	const FbxTime TimeIncrement = AnimTimeSpan.GetDuration() / YMath::Max(NumSamplingKeys, 1);
	if (!skeleton)
	{
		return animation_data;
	}
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


bool YFbxImporter::ImportBlendShapeAnimation(YSkinData* skin_data, AnimationData* anim_data, const std::vector<FbxNode*>& mesh_contain_skeleton_and_bs)
{
	if (!(skin_data && anim_data))
	{
		return false;
	}


	std::unique_ptr<BlendShapeSequneceTrack> bs_sequence_track = std::make_unique<BlendShapeSequneceTrack>();

	float time = -1;
	int frame_rate = FbxTime::GetFrameRate(fbx_scene_->GetGlobalSettings().GetTimeMode());
	int AnimStackIndex = 0;
	int32 ResampleRate = 30.0f;
	//for (int32 AnimStackIndex = 0; AnimStackIndex < AnimStackCount; AnimStackIndex++)
	FbxAnimStack* CurAnimStack = fbx_scene_->GetSrcObject<FbxAnimStack>(AnimStackIndex);
	FbxTimeSpan AnimTimeSpan = CurAnimStack->GetLocalTimeSpan();
	FbxAnimLayer* current_animation_layer = CurAnimStack->GetMember<FbxAnimLayer>();
	if (AnimTimeSpan.GetDirection() == FBXSDK_TIME_FORWARD)
	{
		time = YMath::Max((float)(AnimTimeSpan.GetDuration().GetMilliSeconds() / 1000.0f * scene_info_->frame_rate), time);
	}

	const int32 NumSamplingKeys = YMath::FloorToInt(AnimTimeSpan.GetDuration().GetSecondDouble() * ResampleRate);
	const FbxTime TimeIncrement = AnimTimeSpan.GetDuration() / YMath::Max(NumSamplingKeys, 1);

	int mesh_index = 0;
	for (SkinMesh& skin_mesh : skin_data->meshes_)
	{
		//assert(skin_mesh_to_fbx_node.count(mesh_index));
		//FbxMesh* mesh = skin_mesh_to_fbx_node.at(mesh_index);
		FbxNode* geo_node = mesh_contain_skeleton_and_bs[mesh_index];
		FbxMesh* mesh = geo_node->GetMesh();
		//if (geometry->GetAttributeType() == FbxNodeAttribute::eMesh)

		if (!skin_mesh.bs_.target_shapes_.empty())
		{
			BlendShapeSequneceTrack& bs_sequence_track = anim_data->bs_sequence_track[mesh_index];
			for (FbxTime CurTime = AnimTimeSpan.GetStart(); CurTime <= AnimTimeSpan.GetStop(); CurTime += TimeIncrement)
			{
				int lBlendShapeDeformerCount = mesh->GetDeformerCount(FbxDeformer::eBlendShape);
				for (int blend_shape_index = 0; blend_shape_index < lBlendShapeDeformerCount; ++blend_shape_index)
				{
					FbxBlendShape* lBlendShape = (FbxBlendShape*)mesh->GetDeformer(blend_shape_index, FbxDeformer::eBlendShape);

					int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
					for (int channel_index = 0; channel_index < lBlendShapeChannelCount; ++channel_index)
					{
						FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(channel_index);
						if (lChannel)
						{
							// Get the percentage of influence on this channel.
							FbxAnimCurve* curve = mesh->GetShapeChannel(blend_shape_index, channel_index, current_animation_layer);
							std::string channel_name = lChannel->GetName();
							if (!curve)
							{
								bs_sequence_track.value_curve_[channel_name].push_back(0.0);
							}
							else
							{
								double lWeight = curve->Evaluate(CurTime);
								bs_sequence_track.value_curve_[channel_name].push_back(lWeight);
							}
						}
					}
				}
			}
		}
		mesh_index++;
	}
	return true;
}
