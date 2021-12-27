#pragma once
#include <stdint.h>
#include "Math/YVector.h"

class LightBase
{
public:
	enum class LightType :uint8_t
	{
		LT_Direct = 0,
		LT_Spot = 1,
		LT_Point_light = 2,
		LT_IBL = 3,
		LT_Square = 4,
		LT_Mesh = 5
	};
	LightBase();
	~LightBase();
	LightBase(LightType in_light_type);
	void SetCastShadow(bool cast_shader);
	bool GetCastShadow() const;
	void SetLightStrength(float strength);
	float GetLightStrength() const;
protected:
	LightType light_type_ = LightType::LT_Direct;
	bool cast_shadow_{ false };
	float strength_{ 1.f };

};

class DirectLight :public LightBase
{
public:
	DirectLight();
	void SetLightDir(const YVector light_dir);
	YVector GetLightdir() const;
protected:
	YVector light_dir_{ YVector::forward_vector };
};

class IBLLight :public LightBase
{
public:
	IBLLight();
protected:
};
