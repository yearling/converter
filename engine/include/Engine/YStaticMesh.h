#pragma once
#include <vector>
#include "Engine/YRawMesh.h"
#include <memory>
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "Engine/YCamera.h"
class YStaticMesh
{
public:
	YStaticMesh();
	bool AllocGpuResource();
	void ReleaseGPUReosurce();
	void	Render(CameraBase* camera);
	std::vector<std::unique_ptr<YLODMesh>> raw_meshes;

	void SaveV0(const std::string& dir);
public:
	friend class YStaticMeshVertexFactory;
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