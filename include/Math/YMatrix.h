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
	YMatrix(const YVector4& row0, const YVector4& row1,const YVector4& row2,const YVector4& row3);
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
	YVector4 TransformPosition(const YVector& v) const;
	YMatrix GetTransposed() const;
	/**
	 *	Transform a direction vector - will not take into account translation part of the FMatrix.
	 *	If you want to transform a surface normal (or plane) and correctly account for non-uniform scaling you should use TransformByUsingAdjointT.
	 */
	YVector TransformVector(const YVector& v) const;
	inline YVector GetScaledAxis(int axis) const;
	
	union
	{
		float m[4][4];
	};
	static const YMatrix Identity;

};
