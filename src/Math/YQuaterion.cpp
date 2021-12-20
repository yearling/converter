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
	*this = rotator.ToQuat();
}

YQuat::YQuat(const YMatrix& rotation_matrix)
{
	// If Matrix is NULL, return Identity quaternion. If any of them is 0, you won't be able to construct rotation
// if you have two plane at least, we can reconstruct the frame using cross product, but that's a bit expensive op to do here
// for now, if you convert to matrix from 0 scale and convert back, you'll lose rotation. Don't do that. 
	if (rotation_matrix.GetScaledAxis(0).IsNearlyZero() || rotation_matrix.GetScaledAxis(1).IsNearlyZero() || rotation_matrix.GetScaledAxis(2).IsNearlyZero())
	{
		*this = YQuat::Identity;
	}

	//const MeReal *const t = (MeReal *) tm;
	float	s;

	// Check diagonal (trace)
	const float tr = rotation_matrix.m[0][0] + rotation_matrix.m[1][1] + rotation_matrix.m[2][2];

	if (tr > 0.0f)
	{
		float InvS = YMath::InvSqrt(tr + 1.f);
		this->w = 0.5f * (1.f / InvS);
		s = 0.5f * InvS;

		this->x = (rotation_matrix.m[1][2] - rotation_matrix.m[2][1]) * s;
		this->y = (rotation_matrix.m[2][0] - rotation_matrix.m[0][2]) * s;
		this->z = (rotation_matrix.m[0][1] - rotation_matrix.m[1][0]) * s;
	}
	else
	{
		// diagonal is negative
		int i = 0;

		if (rotation_matrix.m[1][1] > rotation_matrix.m[0][0])
			i = 1;

		if (rotation_matrix.m[2][2] > rotation_matrix.m[i][i])
			i = 2;

		static const int nxt[3] = { 1, 2, 0 };
		const int j = nxt[i];
		const int k = nxt[j];

		s = rotation_matrix.m[i][i] - rotation_matrix.m[j][j] - rotation_matrix.m[k][k] + 1.0f;

		float InvS = YMath::InvSqrt(s);

		float qt[4];
		qt[i] = 0.5f * (1.f / InvS);

		s = 0.5f * InvS;

		qt[3] = (rotation_matrix.m[j][k] - rotation_matrix.m[k][j]) * s;
		qt[j] = (rotation_matrix.m[i][j] + rotation_matrix.m[j][i]) * s;
		qt[k] = (rotation_matrix.m[i][k] + rotation_matrix.m[k][i]) * s;

		this->x = qt[0];
		this->y = qt[1];
		this->z = qt[2];
		this->w = qt[3];

	}
}


YRotator YQuat::Rotator() const
{
	const float singularity_test = w * y - x * z;
	const float pitch_sin = 2.f * (w * x + y * z);
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
		rotator_from_quat.yaw = -90.f;
		rotator_from_quat.roll = YRotator::NormalizeAxis(-rotator_from_quat.pitch + (2.f * YMath::Atan2(x, w) * RAD_TO_DEG));
	}
	else if (singularity_test > SINGULARITY_THRESHOLD) // Sin(Yaw)> 1
	{
		rotator_from_quat.pitch = YMath::Atan2(pitch_sin, pitch_cos) * RAD_TO_DEG;
		rotator_from_quat.yaw = 90.f;
		rotator_from_quat.roll = YRotator::NormalizeAxis(rotator_from_quat.pitch - (2.f * YMath::Atan2(x, w) * RAD_TO_DEG));
	}
	else
	{
		rotator_from_quat.pitch = YMath::Atan2(pitch_sin, pitch_cos) * RAD_TO_DEG;
		rotator_from_quat.yaw = YMath::FastAsin(2.f * (singularity_test)) * RAD_TO_DEG;
		rotator_from_quat.roll = YMath::Atan2(2.0f * (x * y + w * z), 1.0f - 2.0f * (YMath::Square(y) + YMath::Square(z))) * RAD_TO_DEG;
	}
	return rotator_from_quat;
}

YVector YQuat::Euler() const
{
	return Rotator().Euler();
}

YMatrix YQuat::ToMatrix() const
{
	YMatrix tmp;
	tmp.m[0][0] = 1.f - 2.f * (y * y + z * z);
	tmp.m[0][1] = 2.f * (x * y + w * z);
	tmp.m[0][2] = 2.f * (x * z - w * y);
	tmp.m[0][3] = 0.f;

	tmp.m[1][0] = 2.f * (x * y - w * z);
	tmp.m[1][1] = 1.f - 2.f * (x * x + z * z);
	tmp.m[1][2] = 2.f * (w * x + y * z);
	tmp.m[1][3] = 0.f;

	tmp.m[2][0] = 2.f * (w * y + x * z);
	tmp.m[2][1] = 2.f * (y * z - w * x);
	tmp.m[2][2] = 1.f - 2.f * (x * x + y * y);
	tmp.m[2][3] = 0.f;

	tmp.m[3][0] = 0.f;
	tmp.m[3][1] = 0.f;
	tmp.m[3][2] = 0.f;
	tmp.m[3][3] = 1.f;
	return tmp;
}

YQuat YQuat::Identity = YQuat(0, 0, 0, 1.0);


void YQuat::Normalize(float Tolerance /*= SMALL_NUMBER*/)
{
	const float SquareSum = x * x + y * y + z * z + w * w;

	if (SquareSum >= Tolerance)
	{
		const float Scale = YMath::InvSqrt(SquareSum);

		x *= Scale;
		y *= Scale;
		z *= Scale;
		w *= Scale;
	}
	else
	{
		*this = YQuat::Identity;
	}
}

YQuat YQuat::GetNormalized(float Tolerance /*= SMALL_NUMBER*/) const
{
	YQuat Result(*this);
	Result.Normalize(Tolerance);
	return Result;
}

bool YQuat::IsNormalized() const
{
	return (YMath::Abs(1.f - YMath::Sqrt(x * x + y * y + z * z + w * w)) < THRESH_QUAT_NORMALIZED);

}
