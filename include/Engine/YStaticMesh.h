#pragma once
#include <vector>
#include "Engine/YRawMesh.h"
#include <memory>
#include "RHI/DirectX11/D3D11VertexFactory.h"

class YStaticMesh
{
public:
	YStaticMesh();
	bool AllocGpuResource();
	void	Render();
	std::vector<std::unique_ptr<YLODMesh>> raw_meshes;

private:
	bool allocated_gpu_resource = false;
	std::unique_ptr<DXVertexFactory> vertex_factory_;
};