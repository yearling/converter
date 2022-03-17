#include "Math/YRay.h"
#include "Math/YMatrix.h"
bool RayCastTriangle(const YRay& Ray, const YVector& Point0, const YVector& Point1, const YVector& Point2, float& t, float& u, float& v, float& w, bool bCullBackFace/*= true*/)
{
	YVector V01 = Point1 - Point0;
	YVector V02 = Point2 - Point0;
	YVector N = Ray.direction_ ^ V02;
	float det = V01 | N;
	if (bCullBackFace)
	{
		if (det > SMALL_NUMBER)
		{
			return false;
		}
	}
	else
	{
		if (YMath::Abs(det) < SMALL_NUMBER)
		{
			return false;
		}
	}
	float InvDet = 1.0f / det;
	YVector TVec = Ray.origin_ - Point0;
	u = (TVec | N) * InvDet;
	if (u < 0 || u >1)
	{
		return false;
	}
	YVector QVec = TVec ^ V01;
	v = (Ray.direction_ | QVec) * InvDet;
	if (v < 0 || u + v>1)
	{
		return false;
	}
	t = (V02 | QVec) * InvDet;
	w = 1 - u - v;
	return true;
}

bool RayCastTriangleReturnBackFaceResult(const YRay& Ray, const YVector& Point0, const YVector& Point1, const YVector& Point2, float& t, float& u, float& v, float& w, bool& bBackFace, bool bCullBackFace /*= true*/)
{
	YVector V01 = Point1 - Point0;
	YVector V02 = Point2 - Point0;
	YVector N = Ray.direction_ ^ V02;
	float det = V01 | N;
	if (bCullBackFace)
	{
		if (det > SMALL_NUMBER)
		{
			return false;
		}
		bBackFace = false;
	}
	else
	{
		if (YMath::Abs(det) < SMALL_NUMBER)
		{
			return false;
		}
		bBackFace = det > SMALL_NUMBER ? true : false;
	}
	float InvDet = 1.0 / det;
	YVector TVec = Ray.origin_ - Point0;
	u = (TVec | N) * InvDet;
	if (u < 0 || u >1)
	{
		return false;
	}
	YVector QVec = TVec ^ V01;
	v = (Ray.direction_ | QVec) * InvDet;
	if (v < 0 || u + v>1)
	{
		return false;
	}
	t = (V02 | QVec) * InvDet;
	w = 1 - u - v;
	return true;
}

bool RayCastTriangleTriangleMethodMatrix(const YRay& Ray, const YVector& Point0, const YVector& Point1, const YVector& Point2, float& t, float& u, float& v, float& w, bool bCullBackFace /*= true*/)
{
	YVector V01 = Point1 - Point0;
	YVector V02 = Point2 - Point0;
	YMatrix RayMat(YVector4(-Ray.direction_, 0), YVector4(V01, 0),YVector4(V02, 0), YVector4(0, 0, 0, 1));
	float det = RayMat.Determinant();
	if (bCullBackFace && det > SMALL_NUMBER)
	{
		return false;
	}
	else if (YMath::Abs(det) < SMALL_NUMBER)
	{
		return false;
	}
	YVector VOA = Ray.origin_ - Point0;
	YMatrix InvRayMat = RayMat.Inverse();
	YVector4 Result = InvRayMat.TransformVector4(YVector4(VOA, 0));
	t = Result.x;
	u = Result.y;
	v = Result.z;
	w = 1 - u - v;
	if (u < 0 || u >1)
	{
		return false;
	}
	if (v < 0 || u + v>1)
	{
		return false;
	}
	return true;
}

