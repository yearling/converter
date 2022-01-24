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
	static constexpr  bool IsInstance() { return false; };
	 bool LoadFromJson(const Json::Value& RootJson) override;
	 void SaveToPackage(const std::string& Path) override;
	 bool PostLoadOp() override;
	 void Update(double deta_time) override;
	~SMaterial()override;
	SMaterial();
	SMaterial(SObject* parent);
	static TRefCountPtr<SMaterial> GetDefaultMaterial();
protected:
	virtual void SetFloat(const std::string& name, float value);
	virtual void SetVector4(const std::string& name, const YVector4& value);
	virtual void SetTexture(const std::string& name, const std::string& pic_path);
	const std::string& GetShaderPath() const;
	void SetShader(const std::string& shader_path);
protected:
	friend class SDynamicMaterial;
	std::unordered_map<std::string, MaterialParam> parameters_;
	SRenderState render_state_;
	std::string shader_path_;
	std::unique_ptr<YMaterial> material_;
};

class SDynamicMaterial :public SMaterial
{
public:
	SDynamicMaterial();
	~SDynamicMaterial() override;
	SDynamicMaterial(const SMaterial& material);
	void SetFloat(const std::string& name, float value) override;
	void SetVector4(const std::string& name, const YVector4& value)override;
	void SetTexture(const std::string& name, const std::string& pic_path)override;
	void Update(double deta_time) override;
protected:
	bool modify_ = false;
};