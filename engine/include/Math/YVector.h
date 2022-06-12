#pragma once
#include "Math/YMath.h"
#include <type_traits>
struct YVector2
{
public:
	float x;
	float y;
	YVector2() = default;
	YVector2(const YVector2&) = default;
	YVector2& operator=(const YVector2&) = default;
	YVector2(YVector2&&) = default;
	YVector2& operator=(YVector2&&) = default;
	YVector2(float in_x, float in_y);
	bool operator==(const YVector2& v) const;
	bool operator!=(const YVector2& v) const;
	YVector2 operator*(float mul)const;
	YVector2 operator-(const YVector2& v) const;
	YVector2 operator+(const YVector2& v) const;
	static const YVector2 zero_vector;
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
	YVector(float in_x, float in_y, float in_z);
	float& operator[](int index);
	float operator[](int index)const;
	bool IsNearlyZero(float tolerance = KINDA_SMALL_NUMBER) const;
	bool ContainsNaN() const;
	YVector GetSafeNormal(float Tolerance = SMALL_NUMBER) const;
	YVector operator^(const YVector& v) const;
	YVector operator*(float mul)const;
	static YVector CrossProduct(const YVector& a, const YVector& b);
	float	operator|(const YVector& v) const;
	float Dot(const YVector& a, const YVector& b);
	bool Equals(const YVector& v, float Tolerance = KINDA_SMALL_NUMBER) const;
	YVector operator-(const YVector& v) const;
	YVector operator+(const YVector& v) const;
	YVector operator-() const;
	YVector operator*(const YVector& v) const;
	YVector GetAbs() const;
	YVector Reciprocal() const;
	inline bool operator==(const YVector& v) const
	{
		return (x == v.x) && (y == v.y) && (z == v.z);
	}
	float GetMin() const;
	float GetMax() const;
	/** zero vector (0,0,0) */
	static const YVector zero_vector;
	/** world up vector (0,1,0) */
	static const YVector up_vector;
	static const YVector forward_vector;
	static const YVector right_vector;
};

inline YVector operator*(float f, YVector in_v)
{
	return YVector(in_v.x * f, in_v.y * f, in_v.z * f);
}

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
	YVector4(float in_x, float in_y, float in_z, float in_w);
	YVector AffineTransform()const;
	// (0.f,0.f,0.f,0.f)
	static const YVector4 zero_vector;
};

namespace std
{
	template<>
	struct is_pod<YVector2>
	{
		static constexpr bool  value = true;
	};

	template<>
	struct is_pod<YVector>
	{
		static constexpr bool value = true;
	};

	template<>
	struct is_pod<YVector4>
	{
		static constexpr bool value = true;
	};
}
