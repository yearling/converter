#include "Engine/YStaticMesh.h"
#include <cassert>
#include "RHI/DirectX11/D3D11VertexFactory.h"
void YStaticMesh::Render(CameraBase* camera)
{
	if (vertex_factory_)
	{
		vertex_factory_->DrawCall(camera);
	}
}
YStaticMesh::YStaticMesh()
{

}

bool YStaticMesh::AllocGpuResource()
{
	if (allocated_gpu_resource)
	{
		return true;
	}
	if (raw_meshes.size() == 0)
	{
		return false;
	}
	vertex_factory_ = std::make_unique<DXVertexFactory>();
	std::unique_ptr<YLODMesh>& lod_mesh = raw_meshes[0];
	vertex_factory_->AllocGPUResource(lod_mesh.get());
	//vb

}
