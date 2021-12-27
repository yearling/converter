#include "Engine/YLight.h"

LightBase::LightBase()
{

}

LightBase::LightBase(LightType in_light_type)
	:light_type_(in_light_type)
{

}

void LightBase::SetCastShadow(bool cast_shader)
{
	cast_shadow_ = cast_shader;
}

bool LightBase::GetCastShadow() const
{
	return cast_shadow_;
}

void LightBase::SetLightStrength(float strength)
{
	strength_ = strength;
}

float LightBase::GetLightStrength() const
{
	return strength_;
}

LightBase::~LightBase()
{

}

DirectLight::DirectLight() :LightBase(LightType::LT_Direct)
{

}

void DirectLight::SetLightDir(const YVector light_dir)
{
	light_dir_ = light_dir;
}

YVector DirectLight::GetLightdir() const
{
	return light_dir_;
}

IBLLight::IBLLight() :LightBase(LightType::LT_IBL)
{

}
