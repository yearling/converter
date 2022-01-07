#pragma once
#include "Math/YVector.h"
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "Math/YRay.h"
#include "Math/YBox.h"

class YCamvas
{
public:
	YCamvas();
	virtual ~YCamvas();
	void DrawLine(const YVector& start, const  YVector& end, const  YVector4& color,bool solid = true);
	void DrawCube(const YVector& Pos, const YVector4& Color, float length = 0.3f, bool solid=true);
	void DrawRay(const YRay& ray, const YVector4& Color, bool solid = true);
	void DrawAABB(const YBox& box, const YVector4& Color, bool solid = true);
	void Update();
	void Render(CameraBase* camera);
	void Render(class RenderParam* render_param);
protected:
	bool AllocGPUResource();
	friend class YCanvasVertexFactory;
	struct LineDesc
	{
		LineDesc(const YVector& in_s, const YVector& in_e, const YVector4& in_color)
			:start(in_s), end(in_e), color(in_color),solid_(true) {}
		LineDesc(const YVector& in_s, const YVector& in_e, const YVector4& in_color, bool solid)
			:start(in_s), end(in_e), color(in_color), solid_(solid) {}
		YVector start, end;
		YVector4 color;
		bool solid_;
	};

	std::unique_ptr<IVertexFactory> vertex_factory_;
	std::vector<LineDesc> lines_;
	std::vector<TComPtr<ID3D11Buffer>> vertex_buffers_;
	int max_line_num_ = 0;
	const int increament_ = 2 * 1024 * 1024;

	//render state
	TComPtr<ID3D11BlendState> bs_;
	TComPtr<ID3D11DepthStencilState> ds_;
	TComPtr<ID3D11DepthStencilState> ds_no_test_;
	TComPtr<ID3D11RasterizerState> rs_;
	std::unique_ptr<D3DVertexShader> vertex_shader_;
	std::unique_ptr<D3DPixelShader> pixel_shader_;
	int solid_index_ = 0;
	int transparent_index = 0;
};

extern YCamvas* g_Canvas;