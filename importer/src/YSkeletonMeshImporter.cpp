#include "YFbxImporter.h"
#include "fbxsdk/scene/geometry/fbxdeformer.h"
#include <algorithm>
#include "Engine/YSkeletonMesh.h"
#include <deque>

struct ConverterBone :public YBone
{
	std::set<FbxNode*> children_fbx_;
	FbxNode* parent;

};

inline bool IsABone(FbxNode* node) {
	if (node &&node->GetNodeAttribute() && (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton || node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eNull))
	{
		return true;
	}
	return false;
}
std::unique_ptr<YSkeleton> YFbxImporter::BuildSkeleton(std::set<FbxNode*>& nodes)
{
	
	std::unique_ptr<YSkeleton> skeleton = std::make_unique<YSkeleton>();
	
	//find root_node
	auto find_root_node = [](FbxNode* node) {
		FbxNode* root_node = node;
		while (root_node->GetParent()&& IsABone(root_node->GetParent()))
		{
			root_node = root_node->GetParent();
		}
		return root_node;
	};

	std::set<FbxNode*> skeleton_root_node;
	for(auto fbx_node:nodes)
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
	
		if (parent&& IsABone(parent)) {
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

		FbxNode* parent = node->GetParent();
		if (parent&& IsABone(parent)) 
		{
			int parent_id = skeleton->bone_names_to_id_[parent->GetName()];
			YBone& parent_bone = skeleton->bones_[parent_id];
			parent_bone.children_.push_back(bone_id);
			skeleton->bones_[bone_id].parent_id_ = parent_id;
			FbxAMatrix parent_global_trans = parent->EvaluateGlobalTransform();
			FbxAMatrix child_global_trans = node->EvaluateGlobalTransform();
			//FbxAMatrix local_trans = child_global_trans * (parent_global_trans.Inverse());
			FbxAMatrix local_trans = (parent_global_trans.Inverse())*child_global_trans;
			FbxQuaternion qua = local_trans.GetQ();
			FbxVector4 scale = local_trans.GetS();
			FbxVector4 t = local_trans.GetT();
			skeleton->bones_[bone_id].bone_bind_local_tranform_ = YTransform(converter_.ConvertPos(t), converter_.ConvertFbxQutaToQuat(qua), converter_.ConvertScale(scale));
			skeleton->bones_[bone_id].bone_bind_local_matrix_ = converter_.ConvertFbxMatrix(local_trans);
			skeleton->bones_[bone_id].rotator_ = YRotator(converter_.ConvertFbxQutaToQuat(qua));
			
			FbxAMatrix global_trans =  child_global_trans;
			FbxQuaternion qua_g = global_trans.GetQ();
			FbxVector4 scale_g = global_trans.GetS();
			FbxVector4 t_g = global_trans.GetT();
			skeleton->bones_[bone_id].bind_global_transform_ = YTransform(converter_.ConvertPos(t_g), converter_.ConvertFbxQutaToQuat(qua_g), converter_.ConvertScale(scale_g));
			skeleton->bones_[bone_id].bind_global_matrix_ =converter_.ConvertFbxMatrix(child_global_trans);

			//test
			YMatrix parent_matrix = converter_.ConvertFbxMatrix(parent_global_trans);
			YMatrix local_to_parent = converter_.ConvertFbxMatrix(local_trans);
			YMatrix child_matrix = converter_.ConvertFbxMatrix(child_global_trans);

			FbxAMatrix child_matrix_fbx = parent_global_trans * local_trans;
			YMatrix child_matrix_test = local_to_parent* parent_matrix;
			YMatrix local_to_parent_test = parent_matrix.Inverse() * child_matrix;
		}
		else
		{
			FbxAMatrix global_trans = node->EvaluateGlobalTransform();
			FbxQuaternion qua = global_trans.GetQ();
			FbxVector4 scale = global_trans.GetS();
			FbxVector4 t = global_trans.GetT();

			skeleton->bones_[bone_id].bone_bind_local_tranform_ = YTransform(converter_.ConvertPos(t), converter_.ConvertFbxQutaToQuat(qua), converter_.ConvertScale(scale));
			skeleton->bones_[bone_id].bone_bind_local_matrix_ = converter_.ConvertFbxMatrix(global_trans);
			skeleton->bones_[bone_id].rotator_ = YRotator(converter_.ConvertFbxQutaToQuat(qua));

			FbxQuaternion qua_g = global_trans.GetQ();
			FbxVector4 scale_g = global_trans.GetS();
			FbxVector4 t_g = global_trans.GetT();
			skeleton->bones_[bone_id].bind_global_transform_ = YTransform(converter_.ConvertPos(t_g), converter_.ConvertFbxQutaToQuat(qua_g), converter_.ConvertScale(scale_g));
			skeleton->bones_[bone_id].bind_global_matrix_ = converter_.ConvertFbxMatrix(global_trans);
		}
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
					while (link && link->GetParent() && link->GetParent()->GetSkeleton())
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
		if (node->GetNodeAttribute() &&(node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton || node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eNull))
		{
			skeleton_nodes_in_scene.insert(node);
		}
	}
	std::set<FbxNode*> skeleton_nodes;
	std::set_intersection(bone_attach_to_skin.begin(), bone_attach_to_skin.end(), skeleton_nodes_in_scene.begin(), skeleton_nodes_in_scene.end(), std::inserter(skeleton_nodes, skeleton_nodes.begin()));

	std::unique_ptr<YSkeleton> skeleton = BuildSkeleton(skeleton_nodes);

	std::unique_ptr<YSkeletonMesh> skeleton_mesh = std::make_unique<YSkeletonMesh>();
	skeleton_mesh->skeleton_ = std::move(skeleton);
	return skeleton_mesh;
}
