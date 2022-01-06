#include "Math/YMath.h"
#include "Math/YVector.h"
#include "Math/YMatrix.h"
#include "Math/YBox.h"
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

bool YMath::LineBoxIntersection(const YBox& box, const YVector& start, const YVector& end, const YVector& direction)
{
	YVector time;
	YVector rece_vec = direction.Reciprocal();
	bool start_is_outside = false;
	if (start.x < box.min_.x)
	{
		start_is_outside = true;
		if (end.x >= box.min_.x)
		{
			time.x = (box.min_.x - start.x) * rece_vec.x;
		}
		else
		{
			return false;
		}
	}
	else if (start.x > box.max_.x)
	{
		start_is_outside = true;
		if (end.x <= box.max_.x)
		{
			time.x = (box.max_.x - start.x) * rece_vec.x;
		}
		else
		{
			return false;
		}
	}
	else
	{
		time.x = 0.0f;
	}

	if (start.y < box.min_.y)
	{
		start_is_outside = true;
		if (end.y >= box.min_.y)
		{
			time.y = (box.min_.y - start.y) * rece_vec.y;
		}
		else
		{
			return false;
		}
	}
	else if (start.y > box.max_.y)
	{
		start_is_outside = true;
		if (end.y <= box.max_.y)
		{
			time.y = (box.max_.y - start.y) * rece_vec.y;
		}
		else
		{
			return false;
		}
	}
	else
	{
		time.y = 0.0f;
	}

	if (start.z < box.min_.z)
	{
		start_is_outside = true;
		if (end.z >= box.min_.z)
		{
			time.z = (box.min_.z - start.z) * rece_vec.z;
		}
		else
		{
			return false;
		}
	}
	else if (start.z > box.max_.z)
	{
		start_is_outside = true;
		if (end.z <= box.max_.z)
		{
			time.z = (box.max_.z - start.z) * rece_vec.z;
		}
		else
		{
			return false;
		}
	}
	else
	{
		time.z = 0.0f;
	}

	if (start_is_outside)
	{
		const float max_time = YMath::Max(YMath::Max(time.x, time.y), time.z);
		if (max_time >= 0.0f && max_time <= 1.0f)
		{
			const YVector hit = start + direction * max_time;
			const float BOX_SIDE_THRESHOLD = 0.1f;
			if (hit.x > box.min_.x - BOX_SIDE_THRESHOLD && hit.x < box.max_.x + BOX_SIDE_THRESHOLD &&
				hit.y > box.min_.y - BOX_SIDE_THRESHOLD && hit.y < box.max_.y + BOX_SIDE_THRESHOLD &&
				hit.z > box.min_.z - BOX_SIDE_THRESHOLD && hit.z < box.max_.z + BOX_SIDE_THRESHOLD)
			{
				return true;
			}
		}
		return false;
	}
	else
	{
		return true;
	}
}

bool YMath::LineBoxIntersection(const YBox& box, const YVector& start, const YVector& direction)
{
	YVector time;
	YVector rece_vec = direction.Reciprocal();
	bool start_is_outside = false;
	if (start.x < box.min_.x)
	{
		start_is_outside = true;
		if (direction.x>0.f)
		{
			time.x = (box.min_.x - start.x) * rece_vec.x;
		}
		else
		{
			return false;
		}
	}
	else if (start.x > box.max_.x)
	{
		start_is_outside = true;
		if (direction.x <0.f)
		{
			time.x = (box.max_.x - start.x) * rece_vec.x;
		}
		else
		{
			return false;
		}
	}
	else
	{
		time.x = 0.0f;
	}

	if (start.y < box.min_.y)
	{
		start_is_outside = true;
		if (direction.y > 0.f)
		{
			time.y = (box.min_.y - start.y) * rece_vec.y;
		}
		else
		{
			return false;
		}
	}
	else if (start.y > box.max_.y)
	{
		start_is_outside = true;
		if (direction.y < 0.f)
		{
			time.y = (box.max_.y - start.y) * rece_vec.y;
		}
		else
		{
			return false;
		}
	}
	else
	{
		time.y = 0.0f;
	}

	if (start.z < box.min_.z)
	{
		start_is_outside = true;
		if (direction.z>0.f)
		{
			time.z = (box.min_.z - start.z) * rece_vec.z;
		}
		else
		{
			return false;
		}
	}
	else if (start.z > box.max_.z)
	{
		start_is_outside = true;
		if (direction.z < 0.f)
		{
			time.z = (box.max_.z - start.z) * rece_vec.z;
		}
		else
		{
			return false;
		}
	}
	else
	{
		time.z = 0.0f;
	}

	if (start_is_outside)
	{
		const float max_time = YMath::Max(YMath::Max(time.x, time.y), time.z);
		if (max_time >= 0.0f && max_time <= 1.0f)
		{
			const YVector hit = start + direction * max_time;
			const float BOX_SIDE_THRESHOLD = 0.1f;
			if (hit.x > box.min_.x - BOX_SIDE_THRESHOLD && hit.x < box.max_.x + BOX_SIDE_THRESHOLD &&
				hit.y > box.min_.y - BOX_SIDE_THRESHOLD && hit.y < box.max_.y + BOX_SIDE_THRESHOLD &&
				hit.z > box.min_.z - BOX_SIDE_THRESHOLD && hit.z < box.max_.z + BOX_SIDE_THRESHOLD)
			{
				return true;
			}
		}
		return false;
	}
	else
	{
		return true;
	}
}

