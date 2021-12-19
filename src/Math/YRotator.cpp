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
