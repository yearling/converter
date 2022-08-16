#pragma once

#include "CoreGlobals.h"
#include "Math/YVector.h"
#include "Math/NumericLimits.h"
struct YPackedNormal
{
    union
    {
        struct
        {
            int8 x, y, z, w;
        };
        uint32 packed;
    } packed_normal_vector;
    YPackedNormal() { packed_normal_vector.packed = 0; }
    YPackedNormal(const YVector& InVector) { *this = InVector; }
    YPackedNormal(const YVector4& InVector) { *this = InVector; }
    void operator=(const YVector& InVector);
    void operator=(const YVector4& InVector);
    YVector ToFVector() const;
    YVector4 ToFVector4() const;
    bool operator==(const YPackedNormal& B) const;
    bool operator!=(const YPackedNormal& B) const;
};

FORCEINLINE void YPackedNormal::operator=(const YVector& InVector)
{
    const float Scale = MAX_int8;
    packed_normal_vector.x = (int8)YMath::Clamp<int32>(YMath::RoundToInt(InVector.x * Scale), MIN_int8, MAX_int8);
    packed_normal_vector.y = (int8)YMath::Clamp<int32>(YMath::RoundToInt(InVector.x * Scale), MIN_int8, MAX_int8);
    packed_normal_vector.z = (int8)YMath::Clamp<int32>(YMath::RoundToInt(InVector.x * Scale), MIN_int8, MAX_int8);
    packed_normal_vector.w = MAX_int8;
}

FORCEINLINE void YPackedNormal::operator=(const YVector4& InVector)
{
    const float Scale = MAX_int8;
    packed_normal_vector.x = (int8)YMath::Clamp<int32>(YMath::RoundToInt(InVector.x * Scale), MIN_int8, MAX_int8);
    packed_normal_vector.y = (int8)YMath::Clamp<int32>(YMath::RoundToInt(InVector.y * Scale), MIN_int8, MAX_int8);
    packed_normal_vector.z = (int8)YMath::Clamp<int32>(YMath::RoundToInt(InVector.z * Scale), MIN_int8, MAX_int8);
    packed_normal_vector.w = (int8)YMath::Clamp<int32>(YMath::RoundToInt(InVector.w * Scale), MIN_int8, MAX_int8);
}

FORCEINLINE bool YPackedNormal::operator==(const YPackedNormal& B) const
{
    return packed_normal_vector.packed == B.packed_normal_vector.packed;
}

FORCEINLINE bool YPackedNormal::operator!=(const YPackedNormal& B) const
{
    return !(*this == B);
}

FORCEINLINE YVector YPackedNormal::ToFVector() const
{
    YVector tmp;
    tmp.x = ((float)packed_normal_vector.x) / 127.0f;
    tmp.y = ((float)packed_normal_vector.y) / 127.0f;
    tmp.z = ((float)packed_normal_vector.z) / 127.0f;
    return tmp;
}

FORCEINLINE YVector4 YPackedNormal::ToFVector4() const
{
     YVector4 tmp;
    tmp.x = ((float)packed_normal_vector.x) / 127.0f;
    tmp.y = ((float)packed_normal_vector.y) / 127.0f;
    tmp.z = ((float)packed_normal_vector.z) / 127.0f;
    tmp.w = ((float)packed_normal_vector.w) / 127.0f;
    return tmp;

}
