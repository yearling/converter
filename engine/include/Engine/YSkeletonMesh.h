#pragma once
#include "Math/YMatrix.h"
#include "Math/YTransform.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <array>

struct VertexWedge
{
	int control_point_id;
	YVector position;
	YVector normal;
	std::vector<YVector2> uvs_;
	std::array<float, 8> weights_;
	std::array<int, 8> bone_index_;
};
struct SkinMesh
{
	std::vector<YVector> control_points_;
	std::vector<YVector> position_;
	std::vector<YVector> normal_;
	std::vector<std::vector<YVector2>> uvs_;
	std::vector<std::array<float, 8>> weights_;
	std::vector<std::array<int, 8>> bone_index_;

	//tmp
	std::vector<VertexWedge> wedges_;
};
struct YSkinData
{
	std::vector<SkinMesh> meshes_;
};

struct AnimationSequenceTrack
{
	std::vector<YVector> pos_keys_;
	std::vector<YQuat> rot_keys_;
	std::vector<YVector> scale_keys_;
	std::string track_name_;
};

class AnimationData
{
public:
	AnimationData();
	float time_;
	std::string name_;
	std::unordered_map<std::string,AnimationSequenceTrack> sequence_track;
};
class YBone
{
public:
	YBone();

	std::string bone_name_;
	int bone_id_;
	std::vector<int> children_;
	int parent_id_;

	YTransform bind_local_tranform_;
	YMatrix  bind_local_matrix_;
	YTransform local_transform_;
	YMatrix local_matrix_;
	YTransform global_transform_;
	YMatrix global_matrix_;

	YTransform inv_bind_global_transform_;
	//FbxAMatrix fbx_inv_bind_matrix_;
	YMatrix inv_bind_global_matrix_;
	bool fist_init;
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
	std::unique_ptr<AnimationData> animation_data_;
	std::unique_ptr<YSkinData> skin_data_;
	float play_time;
};