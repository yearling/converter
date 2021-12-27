#pragma once
#include "Math/YVector.h"
#include "Math/YMath.h"
#include "Math/YMatrix.h"
#include "Math/YRotator.h"
#include "Math/YQuaterion.h"
struct YTransform
{
public:
	YVector translation;
	YQuat rotator;
	YVector scale;
	YTransform();
	YTransform(const YVector& in_translation, const YQuat& in_quat, const YVector& in_scale);
	YTransform(const YMatrix& mat);
	YTransform operator*(const YTransform& in_transform)const;
	static void YTransform::Multiply(YTransform* OutTransform, const YTransform* A, const YTransform* B);
	static const YTransform identity;
	YMatrix ToMatrix()const;
};