#pragma once
#include "Math/YVector.h"
#include "RHI/DirectX11/D3D11VertexFactory.h"

class YCamvas
{
public:
	YCamvas();
	virtual ~YCamvas();
	static void DrawLine(const YVector& start, const YVector& end, const YVector4& color);

	void Update();
	void Render(CameraBase* camera);
protected:
	void DrawLineInternal(const YVector& start, const  YVector& end, const  YVector4& color);
	bool AllocGPUResource();
	friend class YCanvasVertexFactory;
	struct LineDesc
	{
		LineDesc(const YVector& in_s, const YVector& in_e, const YVector4& in_color)
		:start(in_s),end(in_e),color(in_color){}
		YVector start, end;
		YVector4 color;
	};

	std::unique_ptr<IVertexFactory> vertex_factory_;
	std::vector<LineDesc> lines_;
	std::vector<TComPtr<ID3D11Buffer>> vertex_buffers_;
	int max_line_num_ = 0;
	const int increament_ = 2 * 1024 * 1024;

	//render state
	TComPtr<ID3D11BlendState> bs_;
	TComPtr<ID3D11DepthStencilState> ds_;
	TComPtr<ID3D11RasterizerState> rs_;
	std::unique_ptr<D3DVertexShader> vertex_shader_;
	std::unique_ptr<D3DPixelShader> pixel_shader_;
};

extern YCamvas* g_Canvas;