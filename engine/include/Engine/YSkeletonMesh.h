#pragma once
#include "Math/YMatrix.h"
#include "Math/YTransform.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>


class YBone
{
public:
	YBone();

	YTransform bone_bind_local_tranform_;
	YMatrix  bone_bind_local_matrix_;
	std::string bone_name_;
	int bone_id_;
	std::vector<int> children_;
	int parent_id_;
	YRotator rotator_;

	YTransform bone_local_transform_;
	YMatrix bone_local_matrix_;
	YTransform bone_global_transform_;
	YMatrix bone_global_matrix_;

	YTransform bind_global_transform_;
	YMatrix bind_global_matrix_;
};
class YSkeleton
{
public:
	YSkeleton();
	std::vector<YBone> bones_;
	std::unordered_map<std::string, int> bone_names_to_id_;
	int root_bone_id_;
	void Update(double delta_time);
	void UpdateRecursive(double delta_time,int bone_id);
};
class YSkeletonMesh
{
public:
	YSkeletonMesh();
	void Update(double delta_time);
	void Render();
	std::unique_ptr<YSkeleton> skeleton_;
	
};