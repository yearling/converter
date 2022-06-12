#pragma once
#include "Math/YMath.h"
#include <type_traits>
struct YQuat
{
public:
	float x;
	float y;
	float z;
	float w;
	YQuat() = default;
	YQuat(float x, float y, float z, float w);
	YQuat(const YQuat&) = default;
	YQuat(YQuat&&) = default;
	YQuat& operator=(const YQuat&) = default;
	YQuat& operator=(YQuat&&) = default;
	YQuat(const YVector& axis, float degree);
	explicit YQuat(const YRotator& rotator);
	explicit YQuat(const YMatrix& rotation_matrix);

	YRotator Rotator() const;
	YVector Euler() const;
	YMatrix ToMatrix() const;
	YVector RotateVector(const YVector& V) const;
	static YQuat Identity;
	void Normalize(float Tolerance = SMALL_NUMBER);
	YQuat GetNormalized(float Tolerance = SMALL_NUMBER) const;
	bool ContainsNaN() const;
	// Return true if this quaternion is normalized
	bool IsNormalized() const;
	static YQuat Multiply(const YQuat& p, const YQuat& q);
	YQuat operator*(const YQuat& q) const;
	YVector operator*(const YVector& v) const;
	YQuat operator-() const;
};

YVector operator*(const YVector& v, const YQuat& qua);
namespace std
{
	template<>
	struct is_pod<YQuat>
	{
		static constexpr bool value = true;
	};
}