#pragma once 
#include "YVector.h"
class YRay
{
public:
	YRay() {};
	YRay(const YVector& InOrigin, const YVector& InDirection) :origin_(InOrigin), direction_(InDirection) {}
	YRay(const YRay&) = default;
	YRay(YRay&&) = default;
	YRay& operator=(const YRay&) = default;
	YRay& operator=(YRay&&) = default;

	YVector origin_;
	YVector direction_;
};

namespace std
{
	template<>
	struct is_pod<YRay>
	{
		static constexpr bool  value = true;
	};
}

bool RayCastTriangle(const YRay& Ray,
	const YVector& Point0, const YVector& Point1, const YVector& Point2,
	float& t, float& u, float& v, float& w, bool bCullBackFace = true);

bool RayCastTriangleReturnBackFaceResult(const YRay& Ray,
	const YVector& Point0, const YVector& Point1, const YVector& Point2,
	float& t, float& u, float& v, float& w, bool& bBackFace, bool bCullBackFace = true);

//bool RayCastTriangleTriangleMethodGeometry(const YRay& Ray,
//	const YVector& Point0, const YVector& Point1, const YVector& Point2,
//	float& t, float& u, float& v, float& w, bool bCullBackFace = true);

bool RayCastTriangleTriangleMethodMatrix(const YRay& Ray,
	const YVector& Point0, const YVector& Point1, const YVector& Point2,
	float& t, float& u, float& v, float& w, bool bCullBackFace = true);


