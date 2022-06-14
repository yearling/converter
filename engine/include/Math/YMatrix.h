#pragma once
#include "Math/YMath.h"
#include "Math/YVector.h"
struct YVector4;
struct YMatrix3x3
{
public:
	YMatrix3x3();
	float m[3][3];
};


struct YMatrix
{
public:
	YMatrix();
	YMatrix(const YVector4& row0, const YVector4& row1, const YVector4& row2, const YVector4& row3);
	YMatrix operator*(const YMatrix& other);
	YMatrix(const YRotator& rotator, const YVector& translation);
	float Determinant() const;
	float RotDeterminant() const;
	void SetIdentity();
	static void MatrixMuliply(YMatrix& out, const YMatrix& matrix1, const YMatrix& matrix2);
	YMatrix Inverse() const;
	// Homogeneous transform.
	YVector4 TransformVector4(const YVector4& v) const;
	/** Transform a location - will take into account translation part of the YMatrix. */
	YVector TransformPosition(const YVector& v) const;
	YMatrix GetTransposed() const;
	/**
	 *	Transform a direction vector - will not take into account translation part of the FMatrix.
	 *	If you want to transform a surface normal (or plane) and correctly account for non-uniform scaling you should use TransformByUsingAdjointT.
	 */
	YVector TransformVector(const YVector& v) const;
	YVector GetScaledAxis(int axis) const;
	/** Remove any scaling from this matrix (ie magnitude of each row is 1) and return the 3D scale vector that was initially present. */
	YVector ExtractScaling(float Tolerance = SMALL_NUMBER);
	void Decompose(YVector& tralsation, YQuat& quat, YVector& scale) const;
	void YMatrix::SetAxis(int i, const YVector& axis);
	YVector GetOrigin() const;
	bool ContainsNaN() const;
	union
	{
		float m[4][4];
	};
	static const YMatrix Identity;

	inline bool YMatrix::operator==(const YMatrix& Other) const
	{
		for (int32 X = 0; X < 4; X++)
		{
			for (int32 Y = 0; Y < 4; Y++)
			{
				if (m[X][Y] != Other.m[X][Y])
				{
					return false;
				}
			}
		}

		return true;
	}

	// Error-tolerant comparison.
	inline bool YMatrix::Equals(const YMatrix& Other, float Tolerance=KINDA_SMALL_NUMBER) const
	{
		for (int32 X = 0; X < 4; X++)
		{
			for (int32 Y = 0; Y < 4; Y++)
			{
				if (YMath::Abs(m[X][Y] - Other.m[X][Y]) > Tolerance)
				{
					return false;
				}
			}
		}

		return true;
	}

	inline bool YMatrix::operator!=(const YMatrix& Other) const
	{
		return !(*this == Other);
	}


};

namespace std
{

	template<>
	struct is_pod<YMatrix3x3>
	{
		static constexpr bool value = true;
	};

	template<>
	struct is_pod<YMatrix>
	{
		static constexpr bool value = true;
	};
}
