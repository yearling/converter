#pragma once
#include "Math/YVector.h"
#include "Math/YMath.h"


/**
* Returns true if the specified points are about equal
*/
inline bool PointsEqual(const YVector& V1, const YVector& V2, float ComparisonThreshold)
{
    if (YMath::Abs(V1.x - V2.x) > ComparisonThreshold
        || YMath::Abs(V1.y - V2.y) > ComparisonThreshold
        || YMath::Abs(V1.z - V2.z) > ComparisonThreshold)
    {
        return false;
    }
    return true;
}
