#include <cassert>
#include "Math/YVector.h"

YVector2::YVector2(float in_x, float in_y)
	:x(in_x), y(in_y) {

}

bool YVector2::operator!=(const YVector2& v) const
{
	return !(*this == v);
}

bool YVector2::operator==(const YVector2& v) const
{
	return (x == v.x) && (y == v.y);
}

YVector::YVector(float in_x, float in_y, float in_z)
	: x(in_x), y(in_y), z(in_z)
{

}

float& YVector::operator[](int index)
{
	assert(0 <= index && index <= 2);
	if (index == 0)
	{
		return x;
	}
	else if (index == 1)
	{
		return y;
	}
	else if (index == 2)
	{
		return z;
	}
	assert(0);
}

float YVector::operator[](int index) const
{
	assert(0 <= index && index <= 2);
	if (index == 0)
	{
		return x;
	}
	else if (index == 1)
	{
		return y;
	}
	else if (index == 2)
	{
		return z;
	}
	assert(0);
}

bool YVector::IsNearlyZero(float tolerance /*= KINDA_SMALL_NUMBER*/) const
{
	return
		YMath::Abs(x) <= tolerance
		&& YMath::Abs(y) <= tolerance
		&& YMath::Abs(z) <= tolerance;
}

bool YVector::ContainsNaN() const
{
	return (!YMath::IsFinite(x) ||
		!YMath::IsFinite(x) ||
		!YMath::IsFinite(x));
}

YVector YVector::GetSafeNormal(float Tolerance) const
{
	const float SquareSum = x * x + y * y + z * z;

	// Not sure if it's safe to add tolerance in there. Might introduce too many errors
	if (SquareSum == 1.f)
	{
		return *this;
	}
	else if (SquareSum < Tolerance)
	{
		return YVector::zero_vector;
	}
	const float Scale = YMath::InvSqrt(SquareSum);
	return YVector(x * Scale, y * Scale, z * Scale);
}

YVector YVector::operator^(const YVector& v) const
{
	return YVector
	(
		y * v.z - z * v.y,
		z * v.x - x * v.z,
		x * v.y - y * v.x
	);
}

YVector YVector::operator*(float mul) const
{
	return YVector(x * mul, y * mul, z * mul);
}

YVector YVector::GetAbs() const
{
	return YVector(YMath::Abs(x), YMath::Abs(y), YMath::Abs(z));
}

YVector YVector::Reciprocal() const
{
	YVector rec_vector;
	if (x != 0.f)
	{
		rec_vector.x = 1.f / x;
	}
	else
	{
		rec_vector.x = BIG_NUMBER;
	}
	if (y != 0.f)
	{
		rec_vector.y = 1.f / y;
	}
	else
	{
		rec_vector.y = BIG_NUMBER;
	}
	if (z != 0.f)
	{
		rec_vector.z = 1.f / z;
	}
	else
	{
		rec_vector.z = BIG_NUMBER;
	}

	return rec_vector;
}

YVector YVector::CrossProduct(const YVector& a, const YVector& b)
{
	return a ^ b;
}

float YVector::operator|(const YVector& v) const
{
	return x * v.x + y * v.y + z * v.z;
}

float YVector::Dot(const YVector& a, const YVector& b)
{
	return a | b;
}

bool YVector::Equals(const YVector& v, float Tolerance /*= KINDA_SMALL_NUMBER*/) const
{
	return YMath::Abs(x - v.x) <= Tolerance && YMath::Abs(y - v.y) <= Tolerance && YMath::Abs(z - v.z) <= Tolerance;
}

YVector YVector::operator-(const YVector& v) const
{
	return YVector(x - v.x, y - v.y, z - v.z);
}

YVector YVector::operator-() const
{
	return YVector(-x, -y, -z);
}

float YVector::GetMin() const
{
	return YMath::Min(x, YMath::Min(y, z));
}

float YVector::GetMax() const
{
	return YMath::Max(x, YMath::Max(y, z));
}

YVector YVector::operator+(const YVector& v) const
{
	return YVector(x + v.x, y + v.y, z + v.z);
}

YVector YVector::operator*(const YVector& v) const
{
	return YVector(x * v.x, y * v.y, z * v.z);
}

const YVector YVector::zero_vector = YVector(0.f, 0.f, 0.f);
const YVector YVector::right_vector = YVector(1.f, 0.f, 0.f);
const YVector YVector::up_vector = YVector(0.f, 1.f, 0.f);
const YVector YVector::forward_vector = YVector(0.f, 0.f, 1.f);

YVector4::YVector4(float in_x, float in_y, float in_z, float in_w)
	:x(in_x), y(in_y), z(in_z), w(in_w)
{

}

YVector4::YVector4(const YVector& v, float in_w)
{
	x = v.x;
	y = v.y;
	z = v.z;
	w = in_w;
}

YVector4::YVector4(const float* v)
{
	x = v[0];
	y = v[1];
	z = v[2];
	w = v[3];
}

YVector YVector4::AffineTransform() const
{
	if (YMath::Abs(w) < SMALL_NUMBER)
	{
		return YVector::zero_vector;
	}

	return YVector(x / w, y / w, z / w);
}

const YVector4 YVector4::zero_vector = YVector4(0.f,0.f,0.f,0.f);
