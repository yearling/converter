#include "SObject/SMaterial.h"

bool SMaterial::LoadFromJson(const Json::Value& RootJson)
{
	return true;
}

bool SMaterial::LoadFromMemoryFile(std::unique_ptr<MemoryFile> mem_file)
{
	return true;
}

void SMaterial::SaveToPackage(const std::string& Path)
{
	
}

bool SMaterial::PostLoadOp()
{
	return true;
}

void SMaterial::Update(double deta_time)
{
}

SMaterial::~SMaterial()
{

}

SMaterial::SMaterial()
{

}

void SMaterial::SetFloat(const std::string& name, float value)
{

}

void SMaterial::SetVector4(const std::string& name, const YVector4& value)
{

}

void SMaterial::SetTexture(const std::string& name, const std::string& pic_path)
{

}

void SMaterial::SetSampler(const std::string& name, SamplerType sample_type)
{

}

void SMaterial::SetSampler(const std::string& name, SamplerAddressMode address_mode_x, SamplerAddressMode address_mode_y, SamplerFilterType filter_mode)
{

}

bool SMaterial::LoadFromPackage(const std::string& Path)
{
	return true;
}

SDynamicMaterial::SDynamicMaterial()
{

}

SDynamicMaterial::SDynamicMaterial(const SMaterial& material)
{

}

void SDynamicMaterial::SetFloat(const std::string& name, float value)
{

}

void SDynamicMaterial::SetVector4(const std::string& name, const YVector4& value)
{

}

void SDynamicMaterial::SetTexture(const std::string& name, const std::string& pic_path)
{

}

void SDynamicMaterial::SetSampler(const std::string& name, SamplerType sample_type)
{

}

void SDynamicMaterial::SetSampler(const std::string& name, SamplerAddressMode address_mode_x, SamplerAddressMode address_mode_y, SamplerFilterType filter_mode)
{

}

void SDynamicMaterial::Update(double deta_time)
{

}

SDynamicMaterial::~SDynamicMaterial()
{

}
