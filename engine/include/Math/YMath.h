#pragma once
#include <cstdint>
#include <cmath>
#include <float.h>
//pre declare
struct YVector2;
struct YVector;
struct YVector4;
struct YMatrix;
struct YMatrix3x3;
struct YRotator;
struct YQuat;

/*-----------------------------------------------------------------------------
	Floating point constants.
-----------------------------------------------------------------------------*/

#undef  PI
#define PI 					(3.1415926535897932f)
#define SMALL_NUMBER		(1.e-8f)
#define KINDA_SMALL_NUMBER	(1.e-4f)
#define BIG_NUMBER			(3.4e+38f)
#define EULERS_NUMBER       (2.71828182845904523536f)

// Copied from float.h
#define MAX_FLT 3.402823466e+38F

// Aux constants.
#define INV_PI			(0.31830988618f)
#define HALF_PI			(1.57079632679f)

// Magic numbers for numerical precision.
#define DELTA			(0.00001f)

/**
 * Lengths of normalized vectors (These are half their maximum values
 * to assure that dot products with normalized vectors don't overflow).
 */
#define FLOAT_NORMAL_THRESH				(0.0001f)

 //
 // Magic numbers for numerical precision.
 //
#define THRESH_POINT_ON_PLANE			(0.10f)		/* Thickness of plane for front/back/inside test */
#define THRESH_POINT_ON_SIDE			(0.20f)		/* Thickness of polygon side's side-plane for point-inside/outside/on side test */
#define THRESH_POINTS_ARE_SAME			(0.00002f)	/* Two points are same if within this distance */
#define THRESH_POINTS_ARE_NEAR			(0.015f)	/* Two points are near if within this distance and can be combined if imprecise math is ok */
#define THRESH_NORMALS_ARE_SAME			(0.00002f)	/* Two normal points are same if within this distance */
													/* Making this too large results in incorrect CSG classification and disaster */
#define THRESH_VECTORS_ARE_NEAR			(0.0004f)	/* Two vectors are near if within this distance and can be combined if imprecise math is ok */
													/* Making this too large results in lighting problems due to inaccurate texture coordinates */
#define THRESH_SPLIT_POLY_WITH_PLANE	(0.25f)		/* A plane splits a polygon in half */
#define THRESH_SPLIT_POLY_PRECISELY		(0.01f)		/* A plane exactly splits a polygon */
#define THRESH_ZERO_NORM_SQUARED		(0.0001f)	/* Size of a unit normal that is considered "zero", squared */
#define THRESH_NORMALS_ARE_PARALLEL		(0.999845f)	/* Two unit vectors are parallel if abs(A dot B) is greater than or equal to this. This is roughly cosine(1.0 degrees). */
#define THRESH_NORMALS_ARE_ORTHOGONAL	(0.017455f)	/* Two unit vectors are orthogonal (perpendicular) if abs(A dot B) is less than or equal this. This is roughly cosine(89.0 degrees). */

#define THRESH_VECTOR_NORMALIZED		(0.01f)		/** Allowed error for a normalized vector (against squared magnitude) */
#define THRESH_QUAT_NORMALIZED			(0.01f)		/** Allowed error for a normalized quaternion (against squared magnitude) */
#define MAX_MESH_TEXTURE_COORDS			 8
struct YMath
{
	template< class T >
	static T Square(const T A)
	{
		return A * A;
	}


	/** Computes absolute value in a generic way */
	template< class T >
	static   T Abs(const T A)
	{
		return (A >= (T)0) ? A : -A;
	}

	/** Returns 1, 0, or -1 depending on relation of T to 0 */
	template< class T >
	static   T Sign(const T A)
	{
		return (A > (T)0) ? (T)1 : ((A < (T)0) ? (T)-1 : (T)0);
	}

	/** Returns higher value in a generic way */
	template< class T >
	static   T Max(const T A, const T B)
	{
		return (A >= B) ? A : B;
	}

	/** Returns lower value in a generic way */
	template< class T >
	static   T Min(const T A, const T B)
	{
		return (A <= B) ? A : B;
	}

	template<>
	static   float Abs(const float A)
	{
		return fabsf(A);
	}

	static  float Sin(float Value) { return sinf(Value); }
	static  float Asin(float Value) { return asinf((Value < -1.f) ? -1.f : ((Value < 1.f) ? Value : 1.f)); }
	static  float Sinh(float Value) { return sinhf(Value); }
	static  float Cos(float Value) { return cosf(Value); }
	static  float Acos(float Value) { return acosf((Value < -1.f) ? -1.f : ((Value < 1.f) ? Value : 1.f)); }
	static  float Tan(float Value) { return tanf(Value); }
	static  float Atan(float Value) { return atanf(Value); }
	static float Atan2(float y, float x);
	static  float Sqrt(float Value) { return sqrtf(Value); }
	static  float Pow(float A, float B) { return powf(A, B); }
	/** Computes a fully accurate inverse square root */
	static  float InvSqrt(float F)
	{
		return 1.0f / sqrtf(F);
	}

	/** Computes a faster but less accurate inverse square root */
	static  float InvSqrtEst(float F)
	{
		return InvSqrt(F);
	}

	/** Return true if value is NaN (not a number). */
	static  bool IsNaN(float A)
	{
		return ((*(uint32_t*)&A) & 0x7FFFFFFF) > 0x7F800000;
	}
	/** Return true if value is finite (not NaN and not Infinity). */
	static  bool IsFinite(float A)
	{
		return ((*(uint32_t*)&A) & 0x7F800000) != 0x7F800000;
	}
	static  bool IsNegativeFloat(const float& A)
	{
		return ((*(uint32_t*)&A) >= (uint32_t)0x80000000); // Detects sign bit.
	}

	static  bool IsNegativeDouble(const double& A)
	{
		return ((*(uint64_t*)&A) >= (uint64_t)0x8000000000000000); // Detects sign bit.
	}

	static   int TruncToInt(float F)
	{
		return static_cast<int>(F);
	}

	static float TruncToFloat(float F)
	{
		return (float)TruncToInt(F);
	}
	/**
	* Returns the floating-point remainder of X / Y
	* Warning: Always returns remainder toward 0, not toward the smaller multiple of Y.
	*			So for example Fmod(2.8f, 2) gives .8f as you would expect, however, Fmod(-2.8f, 2) gives -.8f, NOT 1.2f
	* Use Floor instead when snapping positions that can be negative to a grid
	*/
	static  float Fmod(float X, float Y)
	{
		if (fabsf(Y) <= 1.e-8f)
		{
			return 0.f;
		}
		const float Quotient = TruncToFloat(X / Y);
		float IntPortion = Y * Quotient;

		// Rounding and imprecision could cause IntPortion to exceed X and cause the result to be outside the expected range.
		// For example Fmod(55.8, 9.3) would result in a very small negative value!
		if (fabsf(IntPortion) > fabsf(X))
		{
			IntPortion = X;
		}

		const float Result = X - IntPortion;
		return Result;
	}


	// Note:  We use FASTASIN_HALF_PI instead of HALF_PI inside of FastASin(), since it was the value that accompanied the minimax coefficients below.
	// It is important to use exactly the same value in all places inside this function to ensure that FastASin(0.0f) == 0.0f.
	// For comparison:
	//		HALF_PI				== 1.57079632679f == 0x3fC90FDB
	//		FASTASIN_HALF_PI	== 1.5707963050f  == 0x3fC90FDA
#define FASTASIN_HALF_PI (1.5707963050f)
	/**
	* Computes the ASin of a scalar float.
	*
	* @param Value  input angle
	* @return ASin of Value
	*/
	static float FastAsin(float Value)
	{
		// Clamp input to [-1,1].
		bool nonnegative = (Value >= 0.0f);
		float x = YMath::Abs(Value);
		float omx = 1.0f - x;
		if (omx < 0.0f)
		{
			omx = 0.0f;
		}
		float root = YMath::Sqrt(omx);
		// 7-degree minimax approximation
		float result = ((((((-0.0012624911f * x + 0.0066700901f) * x - 0.0170881256f) * x + 0.0308918810f) * x - 0.0501743046f) * x + 0.0889789874f) * x - 0.2145988016f) * x + FASTASIN_HALF_PI;
		result *= root;  // acos(|x|)
		// acos(x) = pi - acos(-x) when x < 0, asin(x) = pi/2 - acos(x)
		return (nonnegative ? FASTASIN_HALF_PI - result : result - FASTASIN_HALF_PI);
	}
#undef FASTASIN_HALF_PI


	// Conversion Functions

	/**
	 * Converts radians to degrees.
	 * @param	RadVal			Value in radians.
	 * @return					Value in degrees.
	 */
	template<class T>
	static  auto RadiansToDegrees(T const& RadVal) -> decltype(RadVal* (180.f / PI))
	{
		return RadVal * (180.f / PI);
	}

	/**
	 * Converts degrees to radians.
	 * @param	DegVal			Value in degrees.
	 * @return					Value in radians.
	 */
	template<class T>
	static  auto DegreesToRadians(T const& DegVal) -> decltype(DegVal* (PI / 180.f))
	{
		return DegVal * (PI / 180.f);
	}

	static float GetBasisDeterminantSign(const YVector& x_axis, const YVector& y_axis, const YVector& z_xis);
	static unsigned int GetIntColor(const YVector4& color);

	template<class T>
	static T Clamp(T min_value, T max_value, T in_value)
	{
		if (in_value < min_value)
		{
			return min_value;
		}
		else if (in_value > max_value)
		{
			return max_value;
		}
		else
		{
			return in_value;
		}
	}
};
