#include "YFbxImporter.h"
#include "fbxsdk/scene/geometry/fbxdeformer.h"
#include <algorithm>
#include "Engine/YSkeletonMesh.h"
#include <deque>
#include "fbxsdk/scene/geometry/fbxcluster.h"
#include "fbxsdk/scene/geometry/fbxblendshape.h"
#include "Engine/YLog.h"
#include "Utility/YStringFormat.h"

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


void YFbxImporter::RecursiveBuildSkeleton(FbxNode* link, std::vector<FbxNode*>& out_bones)
{
	if (IsBone(link))
	{
		out_bones.push_back(link);
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
		for (int pose_index = pose_count - 1; pose_index >= 0; --pose_index)
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
	std::set<FbxCluster*> clusters;
	std::set<FbxNode*> skeleton_root_nodes;
	const int node_count = fbx_scene_->GetNodeCount();
	for (int node_index = 0; node_index < node_count; ++node_index)
	{
		FbxNode* node = fbx_scene_->GetNode(node_index);
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
					while (link && IsBone(link))
					{
						link = link->GetParent();
					}
				}
			}
		}
	}

	if (skeleton_root_nodes.size() != 1) {
		ERROR_INFO("skeleton has multi root nodes, there are");
		for (FbxNode* node : skeleton_root_nodes)
		{
			ERROR_INFO("	,node name is ", node->GetName());
		}
		return nullptr;
	}

	std::vector<FbxNode*> skeleton_nodes;
	for (FbxNode* root_node : skeleton_root_nodes)
	{
		RecursiveBuildSkeleton(root_node, skeleton_nodes);
	}

	std::unique_ptr<YSkeleton> skeleton = std::make_unique<YSkeleton>();
	skeleton->bones_.reserve(skeleton_nodes.size());
	const int bone_count = skeleton_nodes.size();
	for (int bone_id = 0; bone_id < skeleton_nodes.size(); ++bone_id)
	{
		FbxNode* link = skeleton_nodes[bone_id];
		skeleton->bones_.push_back(YBone());

		YBone& bone = skeleton->bones_[bone_id];
		bone.bone_id_ = bone_id;
		bone.bone_name_ = link->GetName();

		for (int find_id = 0; find_id < skeleton->bones_.size() - 1; ++find_id)
		{
			if (skeleton->bones_[find_id].bone_name_ == bone.bone_name_)
			{
				ERROR_INFO(StringFormat("%s has same name bone", bone.bone_name_.c_str()));
				return nullptr;
			}
			if (bone_id != 0)
			{
				if (skeleton_nodes[find_id] == link->GetParent())
				{
					bone.parent_id_ = find_id;
					skeleton->bones_[find_id].children_.push_back(bone_id);
				}
			}
			else
			{
				bone.parent_id_ = INDEX_NONE;
			}
		}
	}

	bool global_link_found_flag = false;
	std::vector<FbxAMatrix> raw_bone_global_matrix;
	raw_bone_global_matrix.resize(bone_count, FbxAMatrix());
	bool any_bone_not_in_bind_pose = false;
	std::string bone_without_bind_pose;
	for (int bone_index = 0; bone_index < bone_count; ++bone_index)
	{
		global_link_found_flag = false;
		//ConverterBone* bone = sorted_bones[bone_index];
		FbxNode* link = skeleton_nodes[bone_index];
		YBone& bone = skeleton->bones_[bone_index];
		out_map[bone_index] = link;
		if (pose_array.size() > 0)
		{
			for (int pose_index = 0; pose_index < pose_array.size(); ++pose_index)
			{
				int pose_link_index = pose_array[pose_index]->Find(link);
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
				bone_without_bind_pose += bone.bone_name_;
				bone_without_bind_pose += ",    ";
			}

			for (FbxCluster* cluster : clusters)
			{
				if (link == cluster->GetLink())
				{
					cluster->GetTransformLinkMatrix(raw_bone_global_matrix[bone_index]);
					global_link_found_flag = true;
					break;
				}
			}
		}
		if (!global_link_found_flag)
		{
			raw_bone_global_matrix[bone_index] = link->EvaluateGlobalTransform();
		}

		if (used_time0_as_bind_pose)
		{
			raw_bone_global_matrix[bone_index] = fbx_scene_->GetAnimationEvaluator()->GetNodeGlobalTransform(link, 0);
		}
	}
	if (any_bone_not_in_bind_pose)
	{
		WARNING_INFO(StringFormat("FbxSkeletaLMeshimport_BonesAreMissingFromBindPose,The following bones are missing from the bind pose : \n %s \nThis can happen for bones that are not vert weighted.If they are not in the correct orientation after importing, \nplease set the \"Use T0 as ref pose\" option or add them to the bind pose and reimport the skeletal mesh.", bone_without_bind_pose.c_str()));
	}
	for (int bone_index = 0; bone_index < bone_count; ++bone_index)
	{
		YBone& final_bone = skeleton->bones_[bone_index];
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
		bool has_nan = false;
		bool has_zero_scale = false;
		YVector trans = final_bone.bind_local_tranform_.translation;
		YVector scale = final_bone.bind_local_tranform_.scale;
		YQuat qua = final_bone.bind_local_tranform_.rotator;

		if (YMath::IsNaN(trans.x) || YMath::IsNaN(trans.y) || YMath::IsNaN(trans.z))
		{
			has_nan = true;
		}
		if (YMath::IsNearlyZero(scale.x) || YMath::IsNearlyZero(scale.y) || YMath::IsNearlyZero(scale.z))
		{
			has_zero_scale = true;
		}
		if (YMath::IsNaN(qua.x) || YMath::IsNaN(qua.y) || YMath::IsNaN(qua.z) || YMath::IsNaN(qua.w))
		{
			has_nan = true;
		}
		if (has_nan)
		{
			WARNING_INFO(StringFormat("%s bone has nan transform", final_bone.bone_name_.c_str()));
		}
		if (has_zero_scale)
		{
			WARNING_INFO(StringFormat("%s bone has zero scale", final_bone.bone_name_.c_str()));
		}
	}
	skeleton->root_bone_id_ = 0;
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
					continue;
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
						continue;
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
							continue;
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
