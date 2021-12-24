#pragma once
#include "Math/YMath.h"
#include "Math/YMatrix.h"
struct YRotator
{
	// rotation pitch yaw roll order
	// look into the axis, clock wise is the positive direction
public:
	/* US: Rotation around the right axis (around X axis), Looking up and down (0=Straight Ahead, +Down,-Up) */
	float pitch;
	/** US: Rotation around the up axis (around Y axis), Running in circles, 0=East, -North, +South. */
	float yaw;
	/** US: Rotation around the forward axis (around Z axis), Tilting your head, 0=Straight, +Clockwise, -CCW. */
	float roll;
	YRotator()=default;
	YRotator(const YRotator&) = default;
	YRotator& operator=(const YRotator&) = default;
	YRotator(YRotator&&) = default;
	YRotator& operator=(YRotator&&) = default;
	YRotator(float in_x, float in_y,float in_z);
	YRotator(const YMatrix& matrix);
	YRotator(const YMatrix3x3& matrix);
	YRotator(const YQuat& quta);
	YRotator(const YVector& in_v);
	static YRotator MakeFromEuler(const YVector& euler);	
	static float ClampAxis(float Angle); //return [0,360)
	static float NormalizeAxis(float Angle);//return (-180,180]
	YVector Euler() const;
	YMatrix ToMatrix() const;
	YQuat ToQuat() const;
};