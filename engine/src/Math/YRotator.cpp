#include "Math/YRotator.h"
#include "Math/YVector.h"
#include "Math/YMatrix.h"
#include "Math/YQuaterion.h"
YRotator::YRotator(float in_x, float in_y, float in_z)
	:pitch(in_x),
	yaw(in_y),
	roll(in_z)
{

}

YRotator::YRotator(const YMatrix& matrix)
{

}

YRotator::YRotator(const YMatrix3x3& matrix)
{

}

YRotator::YRotator(const YQuat& quta)
{
	*this = quta.Rotator();
}

YRotator YRotator::MakeFromEuler(const YVector& euler)
{
	YRotator rotator(euler.x, euler.y,euler.z);
	return rotator;
}

float YRotator::ClampAxis(float angle)
{
	// returns Angle in the range (-360,360)
	angle = YMath::Fmod(angle, 360.f);

	if (angle < 0.f)
	{
		// shift to [0,360) range
		angle += 360.f;
	}

	return angle;
}

float YRotator::NormalizeAxis(float angle)
{
	// returns Angle in the range [0,360)
	angle = ClampAxis(angle);

	if (angle > 180.f)
	{
		// shift to (-180,180]
		angle -= 360.f;
	}

	return angle;
}

YVector YRotator::Euler() const
{
	return YVector(pitch, yaw, roll);
}

YMatrix YRotator::ToMatrix() const
{
	// xyz
	float rad_pitch=	YMath::DegreesToRadians(pitch);
	float rad_yaw=	YMath::DegreesToRadians(yaw);
	float rad_roll=	YMath::DegreesToRadians(roll);
	float sp = YMath::Sin(rad_pitch);
	float cp = YMath::Cos(rad_pitch);
	float sy = YMath::Sin(rad_yaw);
	float cy = YMath::Cos(rad_yaw);
	float sr = YMath::Sin(rad_roll);
	float cr = YMath::Cos(rad_roll);
	YMatrix tmp;
	tmp.m[0][0] = cy * cr;
	tmp.m[0][1] = cy * sr;
	tmp.m[0][2] = -sy;
	tmp.m[0][3] = 0.0;
	tmp.m[1][0] = -cp * sr + sp * sy * cr;
	tmp.m[1][1] = cp * cr + sp * sy * sr;
	tmp.m[1][2] = sp * cy;
	tmp.m[1][3] = 0.0;
	tmp.m[2][0] = sp * sr + cp * sy * cr;
	tmp.m[2][1] = -sp * cr + cp * sy * sr;
	tmp.m[2][2] = cp * cy;
	tmp.m[2][3] = 0.0;
	tmp.m[3][0] = 0.0;
	tmp.m[3][1] = 0.0;
	tmp.m[3][2] = 0.0;
	tmp.m[3][3] = 1.0;
	return tmp;
}

YQuat YRotator::ToQuat() const
{
	float rad_half_pitch = YMath::DegreesToRadians(pitch*0.5f);
	float rad_half_yaw = YMath::DegreesToRadians(yaw*0.5f);
	float rad_half_roll = YMath::DegreesToRadians(roll*0.5f);
	float sp = YMath::Sin(rad_half_pitch);
	float cp = YMath::Cos(rad_half_pitch);
	float sy = YMath::Sin(rad_half_yaw);
	float cy = YMath::Cos(rad_half_yaw);
	float sr = YMath::Sin(rad_half_roll);
	float cr = YMath::Cos(rad_half_roll);
	YQuat tmp;
	tmp.x = sp * cy * cr - cp * sy * sr;
	tmp.y = cp * sy * cr + sp * cy * sr;
	tmp.z = -sp * sy * cr + cp * cy * sr;
	tmp.w = cp * cy * cr + sp * sy * sr;
	return tmp;
}
