#pragma once
#include "Math/YVector.h"
#include "RHI/DirectX11/D3D11VertexFactory.h"

class YCamvas
{
public:
	YCamvas();
	virtual ~YCamvas();
	static void DrawLine(YVector start, YVector end, YVector4 color);
	void Update();
	void Render(CameraBase* camera);
protected:
	struct LineDesc
	{
		YVector start, end;
		YVector4 color;
	};

	std::unique_ptr<DXVertexFactory> vertex_factory_;
	std::vector<LineDesc> points_;
	std::vector<TComPtr<ID3D11Buffer>> vertex_buffers_;
};

extern YCamvas* g_Canvas;