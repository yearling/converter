#include "Engine/YCanvas.h"
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "Engine/YLog.h"
#include <fstream>

class YCanvasVertexFactory :public DXVertexFactory
{
public:
	YCanvasVertexFactory(YCamvas* mesh)
		:mesh_(mesh)
	{

	}
	void SetupStreams()override
	{
		if (vertex_input_layout_)
		{
			const TComPtr<ID3D11DeviceContext> dc = g_device->GetDC();
			dc->IASetInputLayout(vertex_input_layout_);
			for (VertexStreamDescription& desc : vertex_descriptions_) {
				ID3D11Buffer* buffer = mesh_->vertex_buffers_[desc.cpu_data_index];
				unsigned int stride = desc.stride;
				unsigned int offset = 0;
				if (desc.slot != -1) {
					dc->IASetVertexBuffers(desc.slot, 1, &buffer, &stride, &offset);
				}
			}
		}
	}
	void SetupVertexDescriptionPolicy()
	{
		vertex_descriptions_.clear();
		VertexStreamDescription postion_desc(VertexAttribute::VA_POSITION, "position", DataType::Float32, 0, 3, 0, -1, sizeof(YVector), false, false, true);
		VertexStreamDescription uv_desc(VertexAttribute::VA_COLOR, "color", DataType::Uint8, 1, 4, 0, -1, sizeof(int), true, false, true);
		vertex_descriptions_.push_back(postion_desc);
		vertex_descriptions_.push_back(uv_desc);
	}
protected:
	YCamvas* mesh_ = nullptr;
};


YCamvas::YCamvas()
{
	std::unique_ptr<YCanvasVertexFactory> canvas_vf = std::make_unique<YCanvasVertexFactory>(this);
	canvas_vf->SetupVertexDescriptionPolicy();
	vertex_factory_ = std::move(canvas_vf);
}

YCamvas::~YCamvas()
{

}

void YCamvas::DrawCube(const YVector& Pos, const YVector4& Color, float length /*= 0.3f*/)
{
	YVector point0(-1, 1, -1);
	YVector point1(1, 1, -1);
	YVector point2(1, -1, -1);
	YVector point3(-1, -1, -1);
	YVector point4(-1, 1, 1);
	YVector point5(1, 1, 1);
	YVector point6(1, -1, 1);
	YVector point7(-1, -1, 1);

	DrawLine(point0 * length + Pos, point1 * length + Pos, Color);
	DrawLine(point0 * length + Pos, point3 * length + Pos, Color);
	DrawLine(point0 * length + Pos, point4 * length + Pos, Color);
	DrawLine(point1 * length + Pos, point2 * length + Pos, Color);
	DrawLine(point1 * length + Pos, point5 * length + Pos, Color);
	DrawLine(point2 * length + Pos, point3 * length + Pos, Color);
	DrawLine(point2 * length + Pos, point6 * length + Pos, Color);
	DrawLine(point3 * length + Pos, point7 * length + Pos, Color);
	DrawLine(point4 * length + Pos, point5 * length + Pos, Color);
	DrawLine(point4 * length + Pos, point7 * length + Pos, Color);
	DrawLine(point5 * length + Pos, point6 * length + Pos, Color);
	DrawLine(point6 * length + Pos, point7 * length + Pos, Color);
}

void YCamvas::Update()
{
	AllocGPUResource();

	std::vector<YVector> points_tmp;
	std::vector<unsigned int> color_tmp;
	points_tmp.reserve(lines_.size() * 2);
	color_tmp.reserve(lines_.size() * 2);
	for (LineDesc& desc : lines_)
	{
		points_tmp.push_back(desc.start);
		points_tmp.push_back(desc.end);
		color_tmp.push_back(YMath::GetIntColor(desc.color));
		color_tmp.push_back(YMath::GetIntColor(desc.color));
	}
	//update
	if (points_tmp.size())
	{
		g_device->UpdateVBDynaimc(vertex_buffers_[0], 0, &points_tmp[0], (unsigned int)points_tmp.size() * sizeof(YVector));
		g_device->UpdateVBDynaimc(vertex_buffers_[1], 0, &color_tmp[0], (unsigned int)color_tmp.size() * sizeof(int));
	}

}

void YCamvas::Render(CameraBase* camera)
{
	if (vertex_buffers_.empty())
	{
		return;
	}
	ID3D11Device* device = g_device->GetDevice();
	ID3D11DeviceContext* dc = g_device->GetDC();
	float BlendColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	dc->OMSetBlendState(bs_, BlendColor, 0xffffffff);
	dc->RSSetState(rs_);
	dc->OMSetDepthStencilState(ds_, 0);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	//bind im
	vertex_factory_->SetupStreams();
	//bind ib
	vertex_shader_->BindResource("g_projection", camera->GetProjectionMatrix());
	vertex_shader_->BindResource("g_view", camera->GetViewMatrix());
	vertex_shader_->Update();
	pixel_shader_->Update();
	dc->Draw((unsigned int)lines_.size() * 2, 0);

	lines_.clear();
}

void YCamvas::DrawLine(const YVector& start, const YVector& end, const YVector4& color)
{
	lines_.push_back(LineDesc(start, end, color));
}

bool YCamvas::AllocGPUResource()
{
	if (vertex_buffers_.empty())
	{
		assert(max_line_num_ == 0);
		max_line_num_ += increament_;
		int position_bytes = max_line_num_ * sizeof(YVector) * 2;
		int color_bytes = max_line_num_ * sizeof(unsigned int) * 2;
		{
			TComPtr<ID3D11Buffer> vb_tmp;
			g_device->CreateVertexBufferDynamic(position_bytes, nullptr, vb_tmp);
			vertex_buffers_.push_back(vb_tmp);
		}
		{
			TComPtr<ID3D11Buffer> vb_tmp;
			g_device->CreateVertexBufferDynamic(color_bytes, nullptr, vb_tmp);
			vertex_buffers_.push_back(vb_tmp);
		}

		if (!bs_) {
			g_device->CreateBlendState(bs_, true);
		}

		if (!rs_) {
			g_device->CreateRasterStateNonCull(rs_);
		}

		if (!ds_) {
			g_device->CreateDepthStencileState(ds_, true);
		}

		// vs
		{
			vertex_shader_ = std::make_unique<D3DVertexShader>();
			const std::string shader_path = "Shader/Canvas.hlsl";
			std::ifstream inFile;
			inFile.open(shader_path); //open the input file
			if (inFile.bad())
			{
				ERROR_INFO("open shader ", shader_path, " failed!");
				vertex_buffers_.clear();
				return false;
			}
			std::stringstream strStream;
			strStream << inFile.rdbuf(); //read the file
			std::string str = strStream.str();
			if (!vertex_shader_->CreateShaderFromSource(str, "VSMain", vertex_factory_.get()))
			{
				vertex_buffers_.clear();
				return false;
			}
		}
		//ps
		{
			pixel_shader_ = std::make_unique<D3DPixelShader>();
			const std::string shader_path = "Shader/Canvas.hlsl";
			std::ifstream inFile;
			inFile.open(shader_path); //open the input file
			if (inFile.bad())
			{
				ERROR_INFO("open shader ", shader_path, " failed!");
				vertex_buffers_.clear();
				return false;
			}
			std::stringstream strStream;
			strStream << inFile.rdbuf(); //read the file
			std::string str = strStream.str();
			if (!pixel_shader_->CreateShaderFromSource(str, "PSMain"))
			{
				vertex_buffers_.clear();
				return false;
			}
		}
	}
	int new_line_num = max_line_num_;
	while (lines_.size() > new_line_num)
	{
		new_line_num += increament_;
	}


	if (new_line_num > max_line_num_)
	{
		max_line_num_ = new_line_num;
		//alloc

		vertex_buffers_.clear(); //release
		int position_bytes = max_line_num_ * sizeof(YVector) * 2;
		int color_bytes = max_line_num_ * sizeof(int) * 2;
		{
			TComPtr<ID3D11Buffer> vb_tmp;
			g_device->CreateVertexBufferDynamic(position_bytes, nullptr, vb_tmp);
			vertex_buffers_.push_back(vb_tmp);
		}
		{
			TComPtr<ID3D11Buffer> vb_tmp;
			g_device->CreateVertexBufferDynamic(color_bytes, nullptr, vb_tmp);
			vertex_buffers_.push_back(vb_tmp);
		}
	}
	return true;
}

YCamvas* g_Canvas = nullptr;
