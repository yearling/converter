#include "Math/YMath.h"
#include "Math/YVector.h"
#include "Math/YMatrix.h"
float YMath::Atan2(float y, float x)
{
	//return atan2f(Y,X);
	// atan2f occasionally returns NaN with perfectly valid input (possibly due to a compiler or library bug).
	// We are replacing it with a minimax approximation with a max relative error of 7.15255737e-007 compared to the C library function.
	// On PC this has been measured to be 2x faster than the std C version.

	const float absX = YMath::Abs(x);
	const float absY = YMath::Abs(y);
	const bool yAbsBigger = (absY > absX);
	float t0 = yAbsBigger ? absY : absX; // Max(absY, absX)
	float t1 = yAbsBigger ? absX : absY; // Min(absX, absY)

	if (t0 == 0.f)
		return 0.f;

	float t3 = t1 / t0;
	float t4 = t3 * t3;

	static const float c[7] = {
		+7.2128853633444123e-03f,
		-3.5059680836411644e-02f,
		+8.1675882859940430e-02f,
		-1.3374657325451267e-01f,
		+1.9856563505717162e-01f,
		-3.3324998579202170e-01f,
		+1.0f
	};

	t0 = c[0];
	t0 = t0 * t4 + c[1];
	t0 = t0 * t4 + c[2];
	t0 = t0 * t4 + c[3];
	t0 = t0 * t4 + c[4];
	t0 = t0 * t4 + c[5];
	t0 = t0 * t4 + c[6];
	t3 = t0 * t3;

	t3 = yAbsBigger ? (0.5f * PI) - t3 : t3;
	t3 = (x < 0.0f) ? PI - t3 : t3;
	t3 = (y < 0.0f) ? -t3 : t3;

	return t3;
}

float YMath::GetBasisDeterminantSign(const YVector& x_axis, const YVector& y_axis, const YVector& z_xis)
{
	YMatrix basis(
		YVector4(x_axis, 0),
		YVector4(y_axis, 0),
		YVector4(z_xis, 0),
		YVector4(0, 0, 0, 1)
	);
	return (basis.Determinant() < 0) ? -1.0f : +1.0f;
}

unsigned int YMath::GetIntColor(const YVector4& color)
{
	uint8_t x = (uint8_t)YMath::TruncToInt(color.x * 255.0f);
	uint8_t y = (uint8_t)YMath::TruncToInt(color.y * 255.0f);
	uint8_t z = (uint8_t)YMath::TruncToInt(color.z * 255.0f);
	uint8_t w = (uint8_t)YMath::TruncToInt(color.w * 255.0f);
	return (w << 24) | (z << 16) | (y << 8) | x;
}

