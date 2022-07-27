#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include"RHI/DirectX11/D3D11Device.h"
#include "Math/YMatrix.h"

class IVertexFactory;
struct CommonUniformLocation;
struct UniformLoc;
class D3DTexture;

struct ShaderMacroEntry {
	std::string MacroName;
	std::string Value;
};

struct D3DShaderResourceView {
	std::string Name;
	unsigned int BindPoint;
	unsigned int BindCount;
	TComPtr<ID3D11ShaderResourceView> SRV;
};

class ID3DShader {
public:
	ID3DShader();
	ID3DShader(const ID3DShader&) = delete;
	ID3DShader(ID3DShader&&) = delete;
	ID3DShader& operator=(const ID3DShader&) = delete;
	virtual ~ID3DShader() = 0;

public:
	virtual void AddShaderMacros(const std::vector<ShaderMacroEntry>& InShaderMacroEntrys) = 0;
	virtual void SetInclude(const std::string& ShaderSrcInclude) = 0;
	virtual bool AddAlias(const std::string& AliasName) = 0;
	virtual bool CreateShader(const std::string& FileName, const std::string& MainPoint) = 0;
	virtual bool BindResource(const std::string& ParaName, int n) = 0;
	virtual bool BindResource(const std::string& ParaName, float f) = 0;
	virtual bool BindResource(const std::string& ParaName, const float* f, int Num) = 0;
	virtual bool BindResource(const std::string& ParaName, const YMatrix& Mat) = 0;
	virtual bool BindResource(int slot, int n) = 0;
	virtual bool BindResource(int slot, float f) = 0;
	virtual bool BindResource(int slot, const float* f, int Num) = 0;
	virtual bool BindResource(int slot, const char* c, int Num) = 0;
	/*  virtual bool BindResource(const std::string &ParaName, FVector2D V2) = 0;
	  virtual bool BindResource(const std::string &ParaName, FVector V3) = 0;
	  virtual bool BindResource(const std::string &ParaName, FPlane V4) = 0;*/
	  //virtual bool BindResource(const std::string &ParaName, const FMatrix &Mat) = 0;
	  //virtual bool BindResource(const std::string &ParaName, const FMatrix *Mat, int Num) = 0;
	virtual bool BindSRV(const std::string& ParamName, const D3DShaderResourceView& SRV) = 0;
	virtual bool BindSRV(const std::string& param_name, TComPtr<ID3D11ShaderResourceView> srv) = 0;
	virtual bool Update() = 0;
	virtual bool BindInputLayout(std::vector<D3D11_INPUT_ELEMENT_DESC> lInputLayoutDesc) = 0;
	virtual bool ClearSlot() = 0;
	virtual bool BindSRV(int slot, D3DTexture* texture) = 0;
	virtual bool BindTextureSampler(const std::string& sample_name, D3DTextureSampler* sampler) = 0;

protected:
	virtual TComPtr<ID3D11DeviceChild> GetInternalResource() const = 0;
};


struct D3DConstantBuffer {
	D3DConstantBuffer(unsigned int BufferSizeInBytes);
	bool AllocResource();
	enum class eCBType { Scalar, Texture, Interface, BindInfo, Num };
	std::string CBName;
	unsigned int CBSize;  // in bytes;
	std::vector<unsigned char> ShadowBuffer;
	eCBType CBType;
	int BindSlotIndex;
	int BindSlotNum;
	template <class T>
	struct YCBScalar {
		static T& GetValue(D3DConstantBuffer* ConstantBuffer, unsigned int Offset) {
			return *((T*)(ConstantBuffer->ShadowBuffer.data() + Offset));
		}
		static void SetValue(D3DConstantBuffer* ConstantBuffer, unsigned int Offset, T Value) {
			*((T*)(ConstantBuffer->ShadowBuffer.data() + Offset)) = Value;
		}
	};

	template <class T, int N>
	struct YCBVector {
		static T* GetValue(D3DConstantBuffer* ConstantBuffer, unsigned int Offset) {
			return (T*)(ConstantBuffer->ShadowBuffer.data() + Offset);
		}
		static void SetValue(D3DConstantBuffer* ConstantBuffer, unsigned int Offset, const T* Value) {
			memcpy_s((ConstantBuffer->ShadowBuffer.data() + Offset), N * sizeof(T), Value, N * sizeof(T));
		}
	};
	typedef YCBVector<float, 2> YCBVector2;
	typedef YCBVector<float, 3> YCBVector3;
	typedef YCBVector<float, 4> YCBVector4;
	typedef YCBVector<float, 12> YCBMat3X4;
	typedef YCBVector<float, 16> YCBMat4X4;

	template <class T>
	struct YCBVectorVary {
		static T* GetValue(D3DConstantBuffer* ConstantBuffer, unsigned int Offset) {
			return (T*)(ConstantBuffer->ShadowBuffer.data() + Offset);
		}
		static void SetValue(D3DConstantBuffer* ConstantBuffer, unsigned int Offset, int elem_num, const T* Value) {
			memcpy_s((ConstantBuffer->ShadowBuffer.data() + Offset), elem_num * sizeof(T), Value, elem_num * sizeof(T));
		}
	};
	struct YCBMatrix4X4 {
		static YMatrix& GetValue(D3DConstantBuffer* ConstantBuffer, unsigned int Offset) {
			return *((YMatrix*)(ConstantBuffer->ShadowBuffer.data() + Offset));
		}
		static void SetValue(D3DConstantBuffer* ConstantBuffer, unsigned int Offset, const YMatrix& Value) {
			*((YMatrix*)(ConstantBuffer->ShadowBuffer.data() + Offset)) = Value.GetTransposed();
		}
	};

	TComPtr<ID3D11Buffer> D3DBuffer;
};

//class ID3D11ShaderReflection;

class ID3DShaderBind : public ID3DShader {
public:
	ID3DShaderBind();
	virtual ~ID3DShaderBind();
	//virtual bool PostReflection(TComPtr<ID3DBlob> &Blob, TComPtr<ID3D11ShaderReflection> &ShaderReflector);
	virtual void AddShaderMacros(const std::vector<ShaderMacroEntry>& InShaderMacroEntrys) override;
	virtual void SetInclude(const std::string& ShaderSrcInclude) override;
	virtual bool AddAlias(const std::string& AliasName) override;
	virtual bool Update() override;
	virtual bool BindResource(const std::string& ParaName, int n) override;
	virtual bool BindResource(const std::string& ParaName, float f) override;
	virtual bool BindResource(const std::string& ParaName, const  float* f, int Num) override;
	bool BindResource(const std::string& ParaName, const YMatrix& Mat) override;
	virtual bool BindResource(int slot, int n) override;
	virtual bool BindResource(int slot, float f) override;
	virtual bool BindResource(int slot, const float* f, int Num) override;
	virtual bool BindResource(int slot, const char* c, int Num) override;
	/*  virtual bool BindResource(const std::string &ParaName, FVector2D V2) override;
	  virtual bool BindResource(const std::string &ParaName, FVector V3) override;
	  virtual bool BindResource(const std::string &ParaName, FPlane V4) override;
	  virtual bool BindResource(const std::string &ParaName, const FMatrix &Mat) override;
	  virtual bool BindResource(const std::string &ParaName, const FMatrix *Mat, int Num) override;*/
	virtual bool BindSRV(const std::string& ParamName, const D3DShaderResourceView& SRVSRV) override;
	virtual bool BindSRV(int slot, D3DTexture* texture) override;
	virtual bool BindTextureSampler(const std::string& sample_name, D3DTextureSampler* sampler) override;
	virtual bool BindInputLayout(std::vector<D3D11_INPUT_ELEMENT_DESC> lInputLayoutDesc) override { return true; };

	virtual bool ReflectShader(TComPtr<ID3DBlob> Blob);
protected:
	std::string ShaderPath;
	std::string ShaderIncludePath;
	std::string AliasNameForDebug;
	std::vector<ShaderMacroEntry> ShaderMacroEntrys;
	std::vector<std::unique_ptr<D3DConstantBuffer>> ConstantBuffers;
	struct ScalarIndex {
		unsigned int ConstantBufferIndex;
		unsigned int ValueIndex;
		enum class eType { BOOL, INT, FLOAT, FLOAT2, FLOAT3, FLOAT4, MATRIX4X4, NUM };
		// 用来保证在BindResource时，ShadowBuffer的类型与绑定类型一致。
		// 防止在HLSL中是float g_matWorld, Matrix g_fStrength,
		// 但在Cpp里是BindResource(g_fStrength,1.0f),BindResource(g_matWorld,XMMATRIX::Identity())
		eType Type;
	};

	bool BindResourceHelp(const std::string& ParaName, ScalarIndex& Index);
	void AddScalarVariable(const std::string& Name, unsigned int InConstantBufferIndex, unsigned int InValueIndex, ScalarIndex::eType InType);
	std::unordered_map<std::string, ScalarIndex> MapShaderVariableToScalar;
	std::unordered_map<std::string, ScalarIndex> MapSRVToScalar;
	std::unordered_map<std::string, D3DShaderResourceView> MapSRV;
	std::unordered_map<std::string, int> sampler_slot;
	std::unordered_map<std::string, D3DTextureSampler*> samplers_map;
};

class D3DVertexShader : public ID3DShaderBind {
public:
	D3DVertexShader();
	virtual ~D3DVertexShader();
	virtual bool CreateShader(const std::string& FileName, const std::string& MainPoint) override;
	bool CreateShaderFromSource(const std::string& FileName, const std::string& MainPoint, IVertexFactory* vertex_factory);
	virtual bool Update() override;
	virtual bool BindInputLayout(std::vector<D3D11_INPUT_ELEMENT_DESC> InInputLayoutDesc) override;
	virtual bool ClearSlot() override;
	virtual bool BindSRV(int slot, D3DTexture* texture) override;
	bool BindSRV(const std::string& param_name, TComPtr<ID3D11ShaderResourceView> srv) override;
	//private:
public:
	//virtual bool PostReflection(TComPtr<ID3DBlob> &Blob, TComPtr<ID3D11ShaderReflection> &ShaderReflector) override;
	bool ReflectShader(TComPtr<ID3DBlob> Blob) override;
	bool CreateInputLayout(TComPtr<ID3DBlob> Blob, IVertexFactory* vertex_factory);
	virtual TComPtr<ID3D11DeviceChild> GetInternalResource() const;
	TComPtr<ID3D11VertexShader> VertexShader;
	TComPtr<ID3D11InputLayout> InputLayout;
	std::vector<D3D11_INPUT_ELEMENT_DESC> InputLayoutDesc;
};

class D3DPixelShader : public ID3DShaderBind {
public:
	D3DPixelShader();
	virtual ~D3DPixelShader();
	virtual bool CreateShader(const std::string& FileName, const std::string& MainPoint) override;
	bool CreateShaderFromSource(const std::string& FileName, const std::string& MainPoint);
	virtual bool Update() override;
	virtual bool ClearSlot() override;
	virtual bool BindSRV(int slot, D3DTexture* texture) override;
	bool BindSRV(const std::string& param_name, TComPtr<ID3D11ShaderResourceView> srv) override;
	//private:
public:
	virtual TComPtr<ID3D11DeviceChild> GetInternalResource() const;
	TComPtr<ID3D11PixelShader> PixShader;
};


