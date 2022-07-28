#pragma once
#include "RHI/DirectX11/D3D11Device.h"
#include <string>
#include <vector>
#include "RHI/DirectX11/D3D11Shader.h"
#include "Engine/YCamera.h"

class D3DTextureSampler;
struct YLODMesh;
enum VertexAttribute
{
	VA_POSITION,
	VA_UV0,
	VA_UV1,
	VA_NORMAL,
	VA_TANGENT,
	VA_BITANGENT,
	VA_COLOR,
	VA_BONEID,
	VA_BONEIDEXTRA,
	VA_BONEWEIGHT,
	VA_BONEWEIGHTEXTRA,
	VA_ATTRIBUTE0,
	VA_ATTRIBUTE1,
	VA_ATTRIBUTE2,
	VA_ATTRIBUTE3,
	VA_ATTRIBUTE4,
	VA_ATTRIBUTE5,
	VA_ATTRIBUTE6,
	VA_ATTRIBUTE7
};
enum DataType
{
	Float32,
	Uint8,
	Uint32
};
struct VertexStreamDescription {
	VertexStreamDescription();
	VertexStreamDescription(VertexAttribute in_vertex_attribe,
		                 const std::string& in_name, 
		                 DataType in_type,  
						int in_cpu_data_index,           
                        int in_com_num, 
                        int in_buffer_size, 
                        int in_slot, 
                        int in_stride, 
                        int offset,
                        bool in_normalized, 
                        bool in_release, 
                        bool in_dynamic);
	VertexAttribute vertex_attribute;
	std::string name;
	DataType data_type;
	int cpu_data_index;
	int com_num;
	int buffer_size;
	int slot;
	int stride;
    int offset;
	bool normalized : 1;
	bool release : 1;
	bool dynamic : 1;
};
class IVertexFactory {
public:
	IVertexFactory();
	virtual ~IVertexFactory();
	virtual void SetupStreams();
	const std::vector<VertexStreamDescription>& GetVertexDescription() const;
	std::vector<VertexStreamDescription>& GetVertexDescription();
	bool IsGPUSKin() const;
	bool IsMorph() const;
	void SetUseGPUSKin(bool use_gpu_skin);
	void SetUseMorph(bool has_mporh);
	bool HasCustomData() const;
protected:
	std::vector<VertexStreamDescription> vertex_descriptions_;
	std::vector<TComPtr<ID3D11Buffer>> vbs_;
	bool is_alloc_resource_ : 1;
	bool is_gpu_skin_ : 1;
	bool is_morph_ : 1;
	bool has_custom_data_ : 1;
};

class DXVertexFactory :public IVertexFactory {
public:
	DXVertexFactory();
	~DXVertexFactory() override;
	void SetInputLayout(const TComPtr<ID3D11InputLayout>& input_layout);
protected:
	TComPtr<ID3D11InputLayout> vertex_input_layout_;
};