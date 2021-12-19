#include "Math/YQuaterion.h"
#include "Math/YRotator.h"
#include "Math/YVector.h"
#include<cassert>
YQuat::YQuat(float in_x, float in_y, float in_z, float in_w)
	:x(in_x),
	y(in_y),
	z(in_z),
	w(in_w)
{

}

YQuat::YQuat(const YRotator& rotator)
{

}

YQuat::YQuat(const YMatrix& rotation_matrix)
{

}


YRotator YQuat::Rotator() const
{
	const float singularity_test = w * y - x * z;
	const float pitch_sin = 2.f * (w * x + y* z);
	const float pitch_cos = (1.f - 2.f * (YMath::Square(x) + YMath::Square(y)));

	// reference 
	// http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/

	// this value was found from experience, the above websites recommend different values
	// but that isn't the case for us, so I went through different testing, and finally found the case 
	// where both of world lives happily. 
	const float SINGULARITY_THRESHOLD = 0.4999995f;
	const float RAD_TO_DEG = (180.f) / PI;
	YRotator rotator_from_quat;
	if (singularity_test < -SINGULARITY_THRESHOLD) // Sin(Yaw) < -1
	{
		rotator_from_quat.pitch = YMath::Atan2(pitch_sin, pitch_cos) * RAD_TO_DEG;
		rotator_from_quat.yaw = - 90.f;
		rotator_from_quat.roll = YRotator::NormalizeAxis( - rotator_from_quat.pitch + (2.f * YMath::Atan2(x, w) * RAD_TO_DEG));
	}
	else if (singularity_test > SINGULARITY_THRESHOLD) // Sin(Yaw)> 1
	{
		rotator_from_quat.pitch = YMath::Atan2(pitch_sin, pitch_cos) * RAD_TO_DEG;
		rotator_from_quat.yaw = 90.f;
		rotator_from_quat.roll = YRotator::NormalizeAxis( rotator_from_quat.pitch - (2.f * YMath::Atan2(x, w) * RAD_TO_DEG));
	}
	else
	{
		rotator_from_quat.pitch = YMath::Atan2(pitch_sin,pitch_cos) * RAD_TO_DEG;
		rotator_from_quat.yaw = YMath::FastAsin(2.f * (singularity_test)) * RAD_TO_DEG;
		rotator_from_quat.roll =YMath::Atan2(2.0f*( x*y + w*z ), 1.0f - 2.0f * (YMath::Square(y) + YMath::Square(z)) ) * RAD_TO_DEG;
	}
	return rotator_from_quat;
}

YVector YQuat::Euler() const
{
	return Rotator().Euler();
}
