#pragma once
#include "SObject.h"
#include "Math/YVector.h"
#include "RHI/DirectX11/D3D11Texture.h"
#include <unordered_map>
#include <variant>
#include <string>
#include "Engine/YMaterial.h"
#include "Engine/YFile.h"
#include "value.h"



class SMaterial :public SObject
{

public:
	 bool LoadFromJson(const Json::Value& RootJson) override;
	 bool LoadFromMemoryFile(std::unique_ptr<MemoryFile> mem_file) override;
	 void SaveToPackage(const std::string& Path) override;
	 bool PostLoadOp() override;
	 void Update(double deta_time) override;
	~SMaterial()override;
	SMaterial();
protected:
	virtual void SetFloat(const std::string& name, float value);
	virtual void SetVector4(const std::string& name, const YVector4& value);
	virtual void SetTexture(const std::string& name, const std::string& pic_path);
	virtual void SetSampler(const std::string& name, SamplerType sample_type);
	virtual void SetSampler(const std::string& name, SamplerAddressMode address_mode_x,SamplerAddressMode address_mode_y, SamplerFilterType filter_mode);
protected:
	virtual bool LoadFromPackage(const std::string& Path) override;

	std::unordered_map<std::string, MaterialParam> parameters_;
	SRenderState render_state_;
};

class SDynamicMaterial :public SMaterial
{
public:
	SDynamicMaterial();
	~SDynamicMaterial() override;
	SDynamicMaterial(const SMaterial& material);
	void SetFloat(const std::string& name, float value);
	void SetVector4(const std::string& name, const YVector4& value);
	void SetTexture(const std::string& name, const std::string& pic_path);
	void SetSampler(const std::string& name, SamplerType sample_type);
	void SetSampler(const std::string& name, SamplerAddressMode address_mode_x, SamplerAddressMode address_mode_y, SamplerFilterType filter_mode);
	void Update(double deta_time) override;
protected:
	bool modify_ = false;
};