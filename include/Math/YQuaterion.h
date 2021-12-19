#pragma once
#include "Math/YMath.h"
struct YQuat
{
public:
	float x;
	float y;
	float z;
	float w;
	YQuat()=default;
	YQuat(float x, float y, float z, float w);
	YQuat(const YQuat&) = default;
	YQuat(YQuat&&) = default;
	YQuat& operator=(const YQuat&) = default;
	YQuat& operator=(YQuat&&) = default;
	explicit YQuat(const YRotator& rotator);
	explicit YQuat(const YMatrix& rotation_matrix);

	YRotator Rotator() const;
	YVector Euler() const;

};