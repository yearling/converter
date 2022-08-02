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

struct BoneWeightAndID {
	BoneWeightAndID() :weight(0.0), id(-1) {}
	BoneWeightAndID(float in_weight, int in_id) :weight(in_weight), id(in_id) {}
	float weight;
	int id;
};

struct VertexWedge
{
	int control_point_id;
	YVector position;
	YVector normal;
	std::vector<YVector2> uvs_;
	std::vector<BoneWeightAndID> weights_and_ids;
	YVector4 color;
    int mesh_index; // for morph index
};

struct CompressMorphWedge
{
    std::vector<YVector> position;
    std::vector<YVector> normal;
    std::vector<int> map_index;
};

struct MorphWedge
{
    YVector position;
    YVector normal;
};

struct MorphRenderData
{
    std::vector<YVector> position;
    std::vector<YVector> normal;
};
struct RenderData
{
	std::vector<YVector> position;
	std::vector<YVector> normal;
	std::vector<std::array<YVector2, 2>> uv;
	std::vector<std::array<float, 8>> weights;
	std::vector < std::array<int,8>> bone_id;
	std::vector<int> indices;
	std::vector<YVector4> color;

	std::vector<int> sections;
	std::vector<int> triangle_counts;
	std::vector<std::vector<int>> bone_mapping;
    std::unordered_map<std::string, MorphRenderData> morph_render_data;
    std::unordered_map<std::string, CompressMorphWedge> morph_render_data_compress;
};
struct BlendShapeTarget
{
	std::string name_;
	std::vector<YVector> control_points;
};

struct BlendShapeSequneceTrack
{
	std::unordered_map<std::string, std::vector<float>> value_curve_;
	float time_;
	std::string name_;
};

struct BlendShape
{
	std::unordered_map<std::string, BlendShapeTarget> target_shapes_;
	std::vector<YVector> cached_control_point;
};


struct SkinMesh
{
	std::vector<YVector> control_points_;
	std::vector<std::vector<BoneWeightAndID>> bone_weights_and_id_;
	std::vector<YVector> position_;
	std::vector<YVector> normal_;
	std::vector<std::vector<YVector2>> uvs_;

	//tmp
	std::vector<VertexWedge> wedges_;
	BlendShape bs_;
	std::string name_;
	// RenderData

};
struct YSkinDataImported
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
	std::unordered_map<int, BlendShapeSequneceTrack> bs_sequence_track;
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

	YMatrix inv_bind_global_matrix_;
	YMatrix inv_bind_global_matrix_mul_global_matrix;
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
	std::unique_ptr<YSkinDataImported> skin_data_;
	std::unique_ptr<BlendShapeSequneceTrack> bs_anim_;
	float play_time;

//render
	bool AllocGpuResource();
	void ReleaseGPUReosurce();
	void Render(class RenderParam* render_param);
	friend class YSKeletonMeshVertexFactory;
	bool allocated_gpu_resource = false;
	std::vector<TComPtr<ID3D11Buffer>> vertex_buffers_;
	TComPtr<ID3D11Buffer> index_buffer_;
	TComPtr<ID3D11BlendState> blend_state_;
	TComPtr<ID3D11DepthStencilState> ds_;
	TComPtr<ID3D11RasterizerState> rs_;
	TComPtr<ID3D11Buffer> bone_matrix_buffer_;
	TComPtr<ID3D11ShaderResourceView> bone_matrix_buffer_srv_;
	D3DTextureSampler* sampler_state_ = { nullptr };
	std::vector<int> polygon_group_offsets;
	std::unique_ptr<D3DVertexShader> vertex_shader_;
	std::unique_ptr<D3DPixelShader> pixel_shader_;
	std::unique_ptr<DXVertexFactory> vertex_factory_;
	std::string model_name;
	std::vector<YVector> cached_morph_offset;
    TComPtr<ID3D11Buffer> morph_position_offset_buffer;
	std::unique_ptr<RenderData> render_data_;
};