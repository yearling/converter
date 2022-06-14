#pragma once
#include "Math/YMatrix.h"
#include "Math/YTransform.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <array>
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "Engine/YCamera.h"
struct VertexWedge
{
	int control_point_id;
	YVector position;
	YVector normal;
	std::vector<YVector2> uvs_;
	std::vector<float> weights_;
	std::vector<int> bone_index_;
};
struct SkinMesh
{
	std::vector<YVector> control_points_;
	std::vector<std::vector<float>> weights_;
	std::vector<std::vector<int>> bone_index_;

	std::vector<YVector> position_;
	std::vector<YVector> normal_;
	std::vector<std::vector<YVector2>> uvs_;

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

//render
	bool AllocGpuResource();
	void ReleaseGPUReosurce();
	void Render(class RenderParam* render_param);
	friend class YSKeletonMeshVertexFactory;
	bool allocated_gpu_resource = false;
	std::vector<TComPtr<ID3D11Buffer>> vertex_buffers_;
	TComPtr<ID3D11Buffer> index_buffer_;
	TComPtr<ID3D11BlendState> bs_;
	TComPtr<ID3D11DepthStencilState> ds_;
	TComPtr<ID3D11RasterizerState> rs_;
	D3DTextureSampler* sampler_state_ = { nullptr };
	std::vector<int> polygon_group_offsets;
	std::unique_ptr<D3DVertexShader> vertex_shader_;
	std::unique_ptr<D3DPixelShader> pixel_shader_;
	std::unique_ptr<DXVertexFactory> vertex_factory_;
	std::string model_name;
};