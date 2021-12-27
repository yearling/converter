#pragma once
#include "RHI/DirectX11/ComPtr.h"
#include <stdint.h>
#include <d3d11.h>

enum BufferType
{
	BT_Constant = 0,
	BT_Vertex = 1,
	BT_Index = 2,
	BT_NONE
};
class D3DGPUBuffer {
public:
	explicit D3DGPUBuffer(bool dynamic) : dynamic_(dynamic) { }
	virtual ~D3DGPUBuffer();
	BufferType Type() const;
	virtual bool Upload(uint8_t* buffer, uint32_t size);


protected:
	bool dynamic_ = false;
	TComPtr<ID3D11Buffer> d3d_buffer_;
};

class D3DGPUVertexBuffer : public D3DGPUBuffer {
public:
	explicit D3DGPUVertexBuffer(bool dynamic) : D3DGPUBuffer(dynamic) {}
	//GpuBuffer* Clone() const override;
};

class D3DGPUIndexBuffer : public D3DGPUBuffer {
public:
	explicit D3DGPUIndexBuffer(bool dynamic) : D3DGPUBuffer(dynamic) {}
	//GpuBuffer* Clone() const override;
};
