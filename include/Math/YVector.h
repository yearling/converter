#pragma once
#include "Math/YMath.h"
struct YVector2
{
public:
	float x;
	float y;
	YVector2()=default;
	YVector2(const YVector2&)=default;
	YVector2& operator=(const YVector2&) = default;
	YVector2(YVector2&&)=default;
	YVector2& operator=(YVector2&&) = default;
	YVector2(float in_x, float in_y);
};

struct YVector
{
public:
	float x;
	float y;
	float z;
	YVector() = default;
	YVector(const YVector&) = default;
	YVector& operator=(const YVector&) = default;
	YVector(YVector&&) = default;
	YVector& operator=(YVector&&) = default;
	YVector(float in_x, float in_y,float in_z);
	float& operator[](int index);
	float operator[](int index)const;
	bool IsNearlyZero(float tolerance = KINDA_SMALL_NUMBER) const;
	bool ContainsNaN() const;
	YVector GetSafeNormal(float Tolerance = SMALL_NUMBER) const;
	YVector operator^(const YVector& v) const;
	YVector operator*(float mul)const;
	YVector CrossProduct(const YVector& a, const YVector& b);
	float	operator|(const YVector& v) const;
	float Dot(const YVector& a, const YVector& b);
	bool Equals(const YVector& v, float Tolerance = KINDA_SMALL_NUMBER) const;
	YVector operator-(const YVector& v) const;
	YVector operator+(const YVector& v) const;
	YVector operator-() const;
	/** zero vector (0,0,0) */
	static const YVector zero_vector;
	/** world up vector (0,1,0) */
	static const YVector up_vector;
	static const YVector forward_vector;
	static const YVector right_vector;
};

struct YVector4
{
public:
	float x;
	float y;
	float z;
	float w;
	YVector4() = default;
	YVector4(const YVector& v, float w);
	YVector4(const float* v);
	YVector4(const YVector4&) = default;
	YVector4& operator=(const YVector4&) = default;
	YVector4(YVector4&&) = default;
	YVector4& operator=(YVector4&&) = default;
	YVector4(float in_x, float in_y,float in_z, float in_w);
};
