#include "Math/YTransform.h"

YTransform::YTransform()
	:translation(YVector::zero_vector),
	rotator(0.f, 0.f, 0.f, 1.f),
	scale(1.f, 1.f, 1.f)
{

}

YTransform::YTransform(const YVector& in_translation, const YQuat& in_quat, const YVector& in_scale)
	:translation(in_translation),
	rotator(in_quat),
	scale(in_scale)
{

}

YTransform::YTransform(const YMatrix& mat)
{
	mat.Decompose(translation, rotator, scale);
}

YTransform YTransform::operator*(const YTransform& in_transform) const
{
	YTransform local;
	Multiply(&local, this, &in_transform);
	return local;
}

const YTransform YTransform::identity(YVector::zero_vector, YQuat::Identity, YVector(1.f, 1.f, 1.f));

YMatrix YTransform::ToMatrix() const
{
	YMatrix scale_mat = YMatrix::Identity;
	scale_mat.m[0][0] = scale[0];
	scale_mat.m[1][1] = scale[1];
	scale_mat.m[2][2] = scale[2];

	YMatrix rotation_mat = rotator.ToMatrix();
	YMatrix trans_mat = YMatrix::Identity;
	trans_mat.m[3][0] = translation.x;
	trans_mat.m[3][1] = translation.y;
	trans_mat.m[3][2] = translation.z;
	return  scale_mat * rotation_mat * trans_mat;
}

YVector YTransform::TransformPosition(const YVector& v) const
{
	return ToMatrix().TransformPosition(v);
}

YTransform YTransform::Inverse() const
{
	if (YMath::Abs(scale.x) > 0.0 || YMath::Abs(scale.y) > 0.0 || YMath::Abs(scale.z) > 0.0)
	{
		return InverseFast();
	}
	else
	{
		return YTransform::identity;
	}
}

YTransform YTransform::InverseFast() const
{
	// Inverse QST (A) = QST (~A)
		// Since A*~A = Identity, 
		// A(P) = Q(A)*S(A)*P*-Q(A) + T(A)
		// ~A(A(P)) = Q(~A)*S(~A)*(Q(A)*S(A)*P*-Q(A) + T(A))*-Q(~A) + T(~A) = Identity
		// Q(~A)*Q(A)*S(~A)*S(A)*P*-Q(A)*-Q(~A) + Q(~A)*S(~A)*T(A)*-Q(~A) + T(~A) = Identity
		// [Q(~A)*Q(A)]*[S(~A)*S(A)]*P*-[Q(~A)*Q(A)] + [Q(~A)*S(~A)*T(A)*-Q(~A) + T(~A)] = I

		// Identity Q = (0, 0, 0, 1) = Q(~A)*Q(A)
		// Identity Scale = 1 = S(~A)*S(A)
		// Identity Translation = (0, 0, 0) = [Q(~A)*S(~A)*T(A)*-Q(~A) + T(~A)]

		//	Q(~A) = Q(~A)
		//	S(~A) = 1.f/S(A)
		//	T(~A) = - (Q(~A)*S(~A)*T(A)*Q(A))	
	const YVector inv_scale = YVector(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
	assert(rotator.IsNormalized());
	const YQuat inv_rotation = -rotator;
	const YVector scaled_translation = inv_scale * translation;
	const YVector t2 = inv_rotation.RotateVector(scaled_translation);
	return YTransform(-t2, inv_rotation, inv_scale);
}

bool YTransform::NearEqual(const YTransform& other, float ErrorTolerance /*= SMALL_NUMBER*/)
{
	bool trans_equal = translation.Equals(other.translation,ErrorTolerance);
	bool scale_equal = scale.Equals(other.scale, ErrorTolerance);
	bool qua_equal = rotator.EqualAngle(other.rotator, ErrorTolerance);
	return trans_equal && scale_equal && qua_equal;
}

void YTransform::Multiply(YTransform* OutTransform, const YTransform* A, const YTransform* B)
{
	//	When Q = quaternion, S = single scalar scale, and T = translation
	//	QST(A) = Q(A), S(A), T(A), and QST(B) = Q(B), S(B), T(B)

	//	QST (AxB) 

	// QST(A) = Q(A)*S(A)*P*-Q(A) + T(A)
	// QST(AxB) = Q(B)*S(B)*QST(A)*-Q(B) + T(B)
	// QST(AxB) = Q(B)*S(B)*[Q(A)*S(A)*P*-Q(A) + T(A)]*-Q(B) + T(B)
	// QST(AxB) = Q(B)*S(B)*Q(A)*S(A)*P*-Q(A)*-Q(B) + Q(B)*S(B)*T(A)*-Q(B) + T(B)
	// QST(AxB) = [Q(B)*Q(A)]*[S(B)*S(A)]*P*-[Q(B)*Q(A)] + Q(B)*S(B)*T(A)*-Q(B) + T(B)

	//	Q(AxB) = Q(B)*Q(A)
	//	S(AxB) = S(A)*S(B)
	//	T(AxB) = Q(B)*S(B)*T(A)*-Q(B) + T(B)

	const bool bHaveNegativeScale = A->scale.GetMin() < 0 || B->scale.GetMin() < 0;
	if (bHaveNegativeScale)
	{
		// @note, if you have 0 scale with negative, you're going to lose rotation as it can't convert back to quat
		new(OutTransform) YTransform(A->ToMatrix() * B->ToMatrix());
	}
	else
	{
		OutTransform->rotator = B->rotator * A->rotator;
		OutTransform->scale = A->scale * B->scale;
		OutTransform->translation = B->rotator * (B->scale * A->translation) + B->translation;
	}

}
