#include "RHI/DirectX11/D3D11Buffer.h"
D3DGPUBuffer::~D3DGPUBuffer() {

}

BufferType D3DGPUBuffer::Type() const
{
	return BufferType::BT_NONE;
}

bool D3DGPUBuffer::Upload(uint8_t* buffer, uint32_t size)
{
	return true;
}

/*
BufferType D3DGPUBuffer::Type() const { return BufferType::Directx11; }

bool D3DGPUBuffer::Upload(uint8_t* buffer, uint32_t size) { 
	return true; 
}


SKwai::GpuBuffer* D3DGPUVertexBuffer::Clone() const { 
	return new D3DGPUVertexBuffer(dynamic_); 
}

SKwai::GpuBuffer* D3DGPUIndexBuffer::Clone() const { 
	return new D3DGPUIndexBuffer(dynamic_); 
}

*/

