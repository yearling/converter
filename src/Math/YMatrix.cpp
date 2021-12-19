#include "Math/YMatrix.h"
#include "Math/YVector.h"
#include <cassert>
#include <cstring>

YMatrix3x3::YMatrix3x3()
{

}

YMatrix::YMatrix()
{

}

YMatrix::YMatrix(const YVector4& row0, const YVector4& row1, const YVector4& row2, const YVector4& row3)
{
	m[0][0] = row0.x; m[0][1] = row0.y; m[0][2] = row0.z; m[0][3] = row0.w;
	m[1][0] = row1.x; m[1][1] = row1.y; m[1][2] = row1.z; m[1][3] = row1.w;
	m[2][0] = row2.x; m[2][1] = row2.y; m[2][2] = row2.z; m[2][3] = row2.w;
	m[3][0] = row3.x; m[3][1] = row3.y; m[3][2] = row3.z; m[3][3] = row3.w;
}

YMatrix YMatrix::operator*(const YMatrix& other)
{
	YMatrix result;
	MatrixMuliply(result, *this, other);
	return result;
}

float YMatrix::Determinant() const
{
	return	m[0][0] * (
		m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
		m[2][1] * (m[1][2] * m[3][3] - m[1][3] * m[3][2]) +
		m[3][1] * (m[1][2] * m[2][3] - m[1][3] * m[2][2])
		) -
		m[1][0] * (
			m[0][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
			m[2][1] * (m[0][2] * m[3][3] - m[0][3] * m[3][2]) +
			m[3][1] * (m[0][2] * m[2][3] - m[0][3] * m[2][2])
			) +
		m[2][0] * (
			m[0][1] * (m[1][2] * m[3][3] - m[1][3] * m[3][2]) -
			m[1][1] * (m[0][2] * m[3][3] - m[0][3] * m[3][2]) +
			m[3][1] * (m[0][2] * m[1][3] - m[0][3] * m[1][2])
			) -
		m[3][0] * (
			m[0][1] * (m[1][2] * m[2][3] - m[1][3] * m[2][2]) -
			m[1][1] * (m[0][2] * m[2][3] - m[0][3] * m[2][2]) +
			m[2][1] * (m[0][2] * m[1][3] - m[0][3] * m[1][2])
			);
}

float YMatrix::RotDeterminant() const
{
	return
		m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
		m[1][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1]) +
		m[2][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
}

void YMatrix::SetIdentity()
{
	m[0][0] = 1; m[0][1] = 0;  m[0][2] = 0;  m[0][3] = 0;
	m[1][0] = 0; m[1][1] = 1;  m[1][2] = 0;  m[1][3] = 0;
	m[2][0] = 0; m[2][1] = 0;  m[2][2] = 1;  m[2][3] = 0;
	m[3][0] = 0; m[3][1] = 0;  m[3][2] = 0;  m[3][3] = 1;
}

void YMatrix::MatrixMuliply(YMatrix& out, const YMatrix& matrix1, const YMatrix& matrix2)
{
	YMatrix tmp;
	tmp.m[0][0] = matrix1.m[0][0] * matrix2.m[0][0] + matrix1.m[0][1] * matrix2.m[1][0] + matrix1.m[0][2] * matrix2.m[2][0] + matrix1.m[0][3] * matrix2.m[3][0];
	tmp.m[0][1] = matrix1.m[0][0] * matrix2.m[0][1] + matrix1.m[0][1] * matrix2.m[1][1] + matrix1.m[0][2] * matrix2.m[2][1] + matrix1.m[0][3] * matrix2.m[3][1];
	tmp.m[0][2] = matrix1.m[0][0] * matrix2.m[0][2] + matrix1.m[0][1] * matrix2.m[1][2] + matrix1.m[0][2] * matrix2.m[2][2] + matrix1.m[0][3] * matrix2.m[3][2];
	tmp.m[0][3] = matrix1.m[0][0] * matrix2.m[0][3] + matrix1.m[0][1] * matrix2.m[1][3] + matrix1.m[0][2] * matrix2.m[2][3] + matrix1.m[0][3] * matrix2.m[3][3];

	tmp.m[1][0] = matrix1.m[1][0] * matrix2.m[0][0] + matrix1.m[1][1] * matrix2.m[1][0] + matrix1.m[1][2] * matrix2.m[2][0] + matrix1.m[1][3] * matrix2.m[3][0];
	tmp.m[1][1] = matrix1.m[1][0] * matrix2.m[0][1] + matrix1.m[1][1] * matrix2.m[1][1] + matrix1.m[1][2] * matrix2.m[2][1] + matrix1.m[1][3] * matrix2.m[3][1];
	tmp.m[1][2] = matrix1.m[1][0] * matrix2.m[0][2] + matrix1.m[1][1] * matrix2.m[1][2] + matrix1.m[1][2] * matrix2.m[2][2] + matrix1.m[1][3] * matrix2.m[3][2];
	tmp.m[1][3] = matrix1.m[1][0] * matrix2.m[0][3] + matrix1.m[1][1] * matrix2.m[1][3] + matrix1.m[1][2] * matrix2.m[2][3] + matrix1.m[1][3] * matrix2.m[3][3];

	tmp.m[2][0] = matrix1.m[2][0] * matrix2.m[0][0] + matrix1.m[2][1] * matrix2.m[1][0] + matrix1.m[2][2] * matrix2.m[2][0] + matrix1.m[2][3] * matrix2.m[3][0];
	tmp.m[2][1] = matrix1.m[2][0] * matrix2.m[0][1] + matrix1.m[2][1] * matrix2.m[1][1] + matrix1.m[2][2] * matrix2.m[2][1] + matrix1.m[2][3] * matrix2.m[3][1];
	tmp.m[2][2] = matrix1.m[2][0] * matrix2.m[0][2] + matrix1.m[2][1] * matrix2.m[1][2] + matrix1.m[2][2] * matrix2.m[2][2] + matrix1.m[2][3] * matrix2.m[3][2];
	tmp.m[2][3] = matrix1.m[2][0] * matrix2.m[0][3] + matrix1.m[2][1] * matrix2.m[1][3] + matrix1.m[2][2] * matrix2.m[2][3] + matrix1.m[2][3] * matrix2.m[3][3];

	tmp.m[3][0] = matrix1.m[3][0] * matrix2.m[0][0] + matrix1.m[3][1] * matrix2.m[1][0] + matrix1.m[3][2] * matrix2.m[2][0] + matrix1.m[3][3] * matrix2.m[3][0];
	tmp.m[3][1] = matrix1.m[3][0] * matrix2.m[0][1] + matrix1.m[3][1] * matrix2.m[1][1] + matrix1.m[3][2] * matrix2.m[2][1] + matrix1.m[3][3] * matrix2.m[3][1];
	tmp.m[3][2] = matrix1.m[3][0] * matrix2.m[0][2] + matrix1.m[3][1] * matrix2.m[1][2] + matrix1.m[3][2] * matrix2.m[2][2] + matrix1.m[3][3] * matrix2.m[3][2];
	tmp.m[3][3] = matrix1.m[3][0] * matrix2.m[0][3] + matrix1.m[3][1] * matrix2.m[1][3] + matrix1.m[3][2] * matrix2.m[2][3] + matrix1.m[3][3] * matrix2.m[3][3];
	memcpy(out.m, tmp.m, sizeof(float) * 16);
}

YMatrix YMatrix::Inverse() const
{
	YMatrix result_matrix;
	if (GetScaledAxis(0).IsNearlyZero(SMALL_NUMBER) && GetScaledAxis(1).IsNearlyZero(SMALL_NUMBER) && GetScaledAxis(2).IsNearlyZero(SMALL_NUMBER))
	{
		result_matrix = YMatrix::Identity;
	}
	else
	{
		const float det = Determinant();
		if (det == 0.0)
		{
			result_matrix = YMatrix::Identity;
		}
		else
		{
			YMatrix tmp_matrix;
			float det[4];
			tmp_matrix.m[0][0] = m[2][2] * m[3][3] - m[2][3] * m[3][2];
			tmp_matrix.m[0][1] = m[1][2] * m[3][3] - m[1][3] * m[3][2];
			tmp_matrix.m[0][2] = m[1][2] * m[2][3] - m[1][3] * m[2][2];

			tmp_matrix.m[1][0] = m[2][2] * m[3][3] - m[2][3] * m[3][2];
			tmp_matrix.m[1][1] = m[0][2] * m[3][3] - m[0][3] * m[3][2];
			tmp_matrix.m[1][2] = m[0][2] * m[2][3] - m[0][3] * m[2][2];

			tmp_matrix.m[2][0] = m[1][2] * m[3][3] - m[1][3] * m[3][2];
			tmp_matrix.m[2][1] = m[0][2] * m[3][3] - m[0][3] * m[3][2];
			tmp_matrix.m[2][2] = m[0][2] * m[1][3] - m[0][3] * m[1][2];

			tmp_matrix.m[3][0] = m[1][2] * m[2][3] - m[1][3] * m[2][2];
			tmp_matrix.m[3][1] = m[0][2] * m[2][3] - m[0][3] * m[2][2];
			tmp_matrix.m[3][2] = m[0][2] * m[1][3] - m[0][3] * m[1][2];

			det[0] = m[1][1] * tmp_matrix.m[0][0] - m[2][1] * tmp_matrix.m[0][1] + m[3][1] * tmp_matrix.m[0][2];
			det[1] = m[0][1] * tmp_matrix.m[1][0] - m[2][1] * tmp_matrix.m[1][1] + m[3][1] * tmp_matrix.m[1][2];
			det[2] = m[0][1] * tmp_matrix.m[2][0] - m[1][1] * tmp_matrix.m[2][1] + m[3][1] * tmp_matrix.m[2][2];
			det[3] = m[0][1] * tmp_matrix.m[3][0] - m[1][1] * tmp_matrix.m[3][1] + m[2][1] * tmp_matrix.m[3][2];

			float determinant = m[0][0] * det[0] - m[1][0] * det[1] + m[2][0] * det[2] - m[3][0] * det[3];
			const float	r_det = 1.0f / determinant;

			result_matrix.m[0][0] = r_det * det[0];
			result_matrix.m[0][1] = -r_det * det[1];
			result_matrix.m[0][2] = r_det * det[2];
			result_matrix.m[0][3] = -r_det * det[3];
			result_matrix.m[1][0] = -r_det * (m[1][0] * tmp_matrix.m[0][0] - m[2][0] * tmp_matrix.m[0][1] + m[3][0] * tmp_matrix.m[0][2]);
			result_matrix.m[1][1] = r_det * (m[0][0] * tmp_matrix.m[1][0] - m[2][0] * tmp_matrix.m[1][1] + m[3][0] * tmp_matrix.m[1][2]);
			result_matrix.m[1][2] = -r_det * (m[0][0] * tmp_matrix.m[2][0] - m[1][0] * tmp_matrix.m[2][1] + m[3][0] * tmp_matrix.m[2][2]);
			result_matrix.m[1][3] = r_det * (m[0][0] * tmp_matrix.m[3][0] - m[1][0] * tmp_matrix.m[3][1] + m[2][0] * tmp_matrix.m[3][2]);
			result_matrix.m[2][0] = r_det * (
				m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
				m[2][0] * (m[1][1] * m[3][3] - m[1][3] * m[3][1]) +
				m[3][0] * (m[1][1] * m[2][3] - m[1][3] * m[2][1])
				);
			result_matrix.m[2][1] = -r_det * (
				m[0][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
				m[2][0] * (m[0][1] * m[3][3] - m[0][3] * m[3][1]) +
				m[3][0] * (m[0][1] * m[2][3] - m[0][3] * m[2][1])
				);
			result_matrix.m[2][2] = r_det * (
				m[0][0] * (m[1][1] * m[3][3] - m[1][3] * m[3][1]) -
				m[1][0] * (m[0][1] * m[3][3] - m[0][3] * m[3][1]) +
				m[3][0] * (m[0][1] * m[1][3] - m[0][3] * m[1][1])
				);
			result_matrix.m[2][3] = -r_det * (
				m[0][0] * (m[1][1] * m[2][3] - m[1][3] * m[2][1]) -
				m[1][0] * (m[0][1] * m[2][3] - m[0][3] * m[2][1]) +
				m[2][0] * (m[0][1] * m[1][3] - m[0][3] * m[1][1])
				);
			result_matrix.m[3][0] = -r_det * (
				m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
				m[2][0] * (m[1][1] * m[3][2] - m[1][2] * m[3][1]) +
				m[3][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
				);
			result_matrix.m[3][1] = r_det * (
				m[0][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
				m[2][0] * (m[0][1] * m[3][2] - m[0][2] * m[3][1]) +
				m[3][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1])
				);
			result_matrix.m[3][2] = -r_det * (
				m[0][0] * (m[1][1] * m[3][2] - m[1][2] * m[3][1]) -
				m[1][0] * (m[0][1] * m[3][2] - m[0][2] * m[3][1]) +
				m[3][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1])
				);
			result_matrix.m[3][3] = r_det * (
				m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
				m[1][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1]) +
				m[2][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1])
				);
		}
	}
	return result_matrix;
}

YVector4 YMatrix::TransformVector4(const YVector4& v) const
{
	float f[4];
	const float* tmp = reinterpret_cast<const float*>(&v);
	f[0] = tmp[0] * m[0][0] + tmp[1] * m[1][0] + tmp[2] * m[2][0] + tmp[3] * m[3][0];
	f[1] = tmp[0] * m[0][1] + tmp[1] * m[1][1] + tmp[2] * m[2][1] + tmp[3] * m[3][1];
	f[2] = tmp[0] * m[0][2] + tmp[1] * m[1][2] + tmp[2] * m[2][2] + tmp[3] * m[3][2];
	f[3] = tmp[0] * m[0][3] + tmp[1] * m[1][3] + tmp[2] * m[2][3] + tmp[3] * m[3][3];
	return YVector4(f);
}

YVector4 YMatrix::TransformPosition(const YVector& v) const
{
	return TransformVector4(YVector4(v, 1.0f));
}

YMatrix YMatrix::GetTransposed() const
{
	YMatrix	result;

	result.m[0][0] = m[0][0];
	result.m[0][1] = m[1][0];
	result.m[0][2] = m[2][0];
	result.m[0][3] = m[3][0];

	result.m[1][0] = m[0][1];
	result.m[1][1] = m[1][1];
	result.m[1][2] = m[2][1];
	result.m[1][3] = m[3][1];

	result.m[2][0] = m[0][2];
	result.m[2][1] = m[1][2];
	result.m[2][2] = m[2][2];
	result.m[2][3] = m[3][2];

	result.m[3][0] = m[0][3];
	result.m[3][1] = m[1][3];
	result.m[3][2] = m[2][3];
	result.m[3][3] = m[3][3];

	return result;
}

YVector YMatrix::TransformVector(const YVector& v) const
{
	YVector4 result = TransformVector4(YVector4(v, 0.0));
	return YVector(result.x, result.y, result.z);
}

YVector YMatrix::GetScaledAxis(int axis) const
{
	if (axis == 0)
	{
		return YVector(m[0][0], m[0][1], m[0][2]);
	}
	else if (axis == 1)
	{
		return YVector(m[1][0], m[1][1], m[1][2]);
	}
	else if (axis == 2)
	{
		return YVector(m[2][0], m[2][1], m[2][2]);
	}
	assert(0);
	return YVector::zero_vector;
}

const YMatrix YMatrix::Identity(YVector4(1.0, 0.0, 0.0, 0.0), YVector4(0.0, 1.0, 0.0, 0.0), YVector4(0.0, 0.0, 1.0, 0.0), YVector4(0.0, 0.0, 0.0, 1.0));

