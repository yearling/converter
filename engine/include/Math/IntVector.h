// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <stdint.h>
#include "YVector.h"
#include <string>
#include <cassert>


/**
 * Structure for integer vectors in 3-d space.
 */
struct FIntVector
{
	/** Holds the point's x-coordinate. */
	int32_t X;

	/** Holds the point's y-coordinate. */
	int32_t Y;

	/**  Holds the point's z-coordinate. */
	int32_t Z;

public:

	/** An int point with zeroed values. */
	static const FIntVector ZeroValue;

	/** An int point with INDEX_NONE values. */
	static const FIntVector NoneValue;

public:

	/**
	 * Default constructor (no initialization).
	 */
	FIntVector();

	/**
	 * Creates and initializes a new instance with the specified coordinates.
	 *
	 * @param InX The x-coordinate.
	 * @param InY The y-coordinate.
	 * @param InZ The z-coordinate.
	 */
	FIntVector(int32_t InX, int32_t InY, int32_t InZ);

	/**
	 * Constructor
	 *
	 * @param InValue replicated to all components
	 */
	explicit FIntVector(int32_t InValue);

	/**
	 * Constructor
	 *
	 * @param InVector float vector converted to int
	 */
	explicit FIntVector(YVector InVector);

	/**
	 * Constructor
	 *
	 * @param EForceInit Force init enum
	 */
	explicit inline FIntVector(EForceInit);

public:

	/**
	 * Gets specific component of a point.
	 *
	 * @param ComponentIndex Index of point component.
	 * @return const reference to component.
	 */
	const int32_t& operator()(int32_t ComponentIndex) const;

	/**
	 * Gets specific component of a point.
	 *
	 * @param ComponentIndex Index of point component.
	 * @return reference to component.
	 */
	int32_t& operator()(int32_t ComponentIndex);

	/**
	 * Gets specific component of a point.
	 *
	 * @param ComponentIndex Index of point component.
	 * @return const reference to component.
	 */
	const int32_t& operator[](int32_t ComponentIndex) const;

	/**
	 * Gets specific component of a point.
	 *
	 * @param ComponentIndex Index of point component.
	 * @return reference to component.
	 */
	int32_t& operator[](int32_t ComponentIndex);

	/**
	 * Compares points for equality.
	 *
	 * @param Other The other int point being compared.
	 * @return true if the points are equal, false otherwise..
	 */
	bool operator==(const FIntVector& Other) const;

	/**
	 * Compares points for inequality.
	 *
	 * @param Other The other int point being compared.
	 * @return true if the points are not equal, false otherwise..
	 */
	bool operator!=(const FIntVector& Other) const;

	/**
	 * Scales this point.
	 *
	 * @param Scale What to multiply the point by.
	 * @return Reference to this point after multiplication.
	 */
	FIntVector& operator*=(int32_t Scale);

	/**
	 * Divides this point.
	 *
	 * @param Divisor What to divide the point by.
	 * @return Reference to this point after division.
	 */
	FIntVector& operator/=(int32_t Divisor);

	/**
	 * Adds to this point.
	 *
	 * @param Other The point to add to this point.
	 * @return Reference to this point after addition.
	 */
	FIntVector& operator+=(const FIntVector& Other);

	/**
	 * Subtracts from this point.
	 *
	 * @param Other The point to subtract from this point.
	 * @return Reference to this point after subtraction.
	 */
	FIntVector& operator-=(const FIntVector& Other);

	/**
	 * Assigns another point to this one.
	 *
	 * @param Other The point to assign this point from.
	 * @return Reference to this point after assignment.
	 */
	FIntVector& operator=(const FIntVector& Other);

	/**
	 * Gets the result of scaling on this point.
	 *
	 * @param Scale What to multiply the point by.
	 * @return A new scaled int point.
	 */
	FIntVector operator*(int32_t Scale) const;

	/**
	 * Gets the result of division on this point.
	 *
	 * @param Divisor What to divide the point by.
	 * @return A new divided int point.
	 */
	FIntVector operator/(int32_t Divisor) const;

	/**
	 * Gets the result of addition on this point.
	 *
	 * @param Other The other point to add to this.
	 * @return A new combined int point.
	 */
	FIntVector operator+(const FIntVector& Other) const;

	/**
	 * Gets the result of subtraction from this point.
	 *
	 * @param Other The other point to subtract from this.
	 * @return A new subtracted int point.
	 */
	FIntVector operator-(const FIntVector& Other) const;

	/**
	 * Is vector equal to zero.
	 * @return is zero
	*/
	bool IsZero() const;

public:

	/**
	 * Gets the maximum value in the point.
	 *
	 * @return The maximum value in the point.
	 */
	float GetMax() const;

	/**
	 * Gets the minimum value in the point.
	 *
	 * @return The minimum value in the point.
	 */
	float GetMin() const;

	/**
	 * Gets the distance of this point from (0,0,0).
	 *
	 * @return The distance of this point from (0,0,0).
	 */
	int32_t Size() const;

	/**
	 * Get a textual representation of this vector.
	 *
	 * @return A string describing the vector.
	 */
	std::string ToString() const;

public:

	/**
	 * Divide an int point and round up the result.
	 *
	 * @param lhs The int point being divided.
	 * @param Divisor What to divide the int point by.
	 * @return A new divided int point.
	 */
	static FIntVector DivideAndRoundUp(FIntVector lhs, int32_t Divisor);

	/**
	 * Gets the number of components a point has.
	 *
	 * @return Number of components point has.
	 */
	static int32_t Num();

public:

	/**
	 * Serializes the Rectangle.
	 *
	 * @param Ar The archive to serialize into.
	 * @param Vector The vector to serialize.
	 * @return Reference to the Archive after serialization.
	 */
	 //friend FArchive& operator<<( FArchive& Ar, FIntVector& Vector )
	 //{
	 //	return Ar << Vector.X << Vector.Y << Vector.Z;
	 //}

	 //friend void operator<<(FStructuredArchive::FSlot Slot, FIntVector& Vector)
	 //{
	 //	FStructuredArchive::FRecord Record = Slot.EnterRecord();
	 //	Record << NAMED_ITEM("X", Vector.X);
	 //	Record << NAMED_ITEM("Y", Vector.Y);
	 //	Record << NAMED_ITEM("Z", Vector.Z);
	 //}

	 //bool Serialize( FArchive& Ar )
	 //{
	 //	Ar << *this;
	 //	return true;
	 //}
};


/* FIntVector inline functions
 *****************************************************************************/

inline FIntVector::FIntVector()
{ }


inline FIntVector::FIntVector(int32_t InX, int32_t InY, int32_t InZ)
	: X(InX)
	, Y(InY)
	, Z(InZ)
{ }


inline FIntVector::FIntVector(int32_t InValue)
	: X(InValue)
	, Y(InValue)
	, Z(InValue)
{ }


inline FIntVector::FIntVector(EForceInit)
	: X(0)
	, Y(0)
	, Z(0)
{ }


inline const int32_t& FIntVector::operator()(int32_t ComponentIndex) const
{
	return (&X)[ComponentIndex];
}


inline int32_t& FIntVector::operator()(int32_t ComponentIndex)
{
	return (&X)[ComponentIndex];
}


inline const int32_t& FIntVector::operator[](int32_t ComponentIndex) const
{
	return (&X)[ComponentIndex];
}


inline int32_t& FIntVector::operator[](int32_t ComponentIndex)
{
	return (&X)[ComponentIndex];
}

inline bool FIntVector::operator==(const FIntVector& Other) const
{
	return X == Other.X && Y == Other.Y && Z == Other.Z;
}


inline bool FIntVector::operator!=(const FIntVector& Other) const
{
	return X != Other.X || Y != Other.Y || Z != Other.Z;
}


inline FIntVector& FIntVector::operator*=(int32_t Scale)
{
	X *= Scale;
	Y *= Scale;
	Z *= Scale;

	return *this;
}


inline FIntVector& FIntVector::operator/=(int32_t Divisor)
{
	X /= Divisor;
	Y /= Divisor;
	Z /= Divisor;

	return *this;
}


inline FIntVector& FIntVector::operator+=(const FIntVector& Other)
{
	X += Other.X;
	Y += Other.Y;
	Z += Other.Z;

	return *this;
}


inline FIntVector& FIntVector::operator-=(const FIntVector& Other)
{
	X -= Other.X;
	Y -= Other.Y;
	Z -= Other.Z;

	return *this;
}


inline FIntVector& FIntVector::operator=(const FIntVector& Other)
{
	X = Other.X;
	Y = Other.Y;
	Z = Other.Z;

	return *this;
}


inline FIntVector FIntVector::operator*(int32_t Scale) const
{
	return FIntVector(*this) *= Scale;
}


inline FIntVector FIntVector::operator/(int32_t Divisor) const
{
	return FIntVector(*this) /= Divisor;
}


inline FIntVector FIntVector::operator+(const FIntVector& Other) const
{
	return FIntVector(*this) += Other;
}

inline FIntVector FIntVector::operator-(const FIntVector& Other) const
{
	return FIntVector(*this) -= Other;
}


inline FIntVector FIntVector::DivideAndRoundUp(FIntVector lhs, int32_t Divisor)
{
	return FIntVector(YMath::DivideAndRoundUp(lhs.X, Divisor), YMath::DivideAndRoundUp(lhs.Y, Divisor), YMath::DivideAndRoundUp(lhs.Z, Divisor));
}


inline float FIntVector::GetMax() const
{
	return YMath::Max(YMath::Max(X, Y), Z);
}


inline float FIntVector::GetMin() const
{
	return YMath::Min(YMath::Min(X, Y), Z);
}


inline int32_t FIntVector::Num()
{
	return 3;
}


inline int32_t FIntVector::Size() const
{
	int64_t X64 = (int64_t)X;
	int64_t Y64 = (int64_t)Y;
	int64_t Z64 = (int64_t)Z;
	return int32_t(YMath::Sqrt(float(X64 * X64 + Y64 * Y64 + Z64 * Z64)));
}

inline bool FIntVector::IsZero() const
{
	return *this == ZeroValue;
}


inline std::string FIntVector::ToString() const
{
	assert(0);
	return "";
	//return std::string::Printf(TEXT("X=%d Y=%d Z=%d"), X, Y, Z);
}

inline uint32_t GetTypeHash(const FIntVector& Vector)
{
	assert(0);
	return 0;
	//return FCrc::MemCrc_DEPRECATED(&Vector,sizeof(FIntVector));
}

struct FIntVector4
{
	int32_t X, Y, Z, W;

	inline FIntVector4()
	{
	}

	inline FIntVector4(int32_t InX, int32_t InY, int32_t InZ, int32_t InW)
		: X(InX)
		, Y(InY)
		, Z(InZ)
		, W(InW)
	{
	}

	inline explicit FIntVector4(int32_t InValue)
		: X(InValue)
		, Y(InValue)
		, Z(InValue)
		, W(InValue)
	{
	}

	inline FIntVector4(EForceInit)
		: X(0)
		, Y(0)
		, Z(0)
		, W(0)
	{
	}

	inline const int32_t& operator[](int32_t ComponentIndex) const
	{
		return (&X)[ComponentIndex];
	}


	inline int32_t& operator[](int32_t ComponentIndex)
	{
		return (&X)[ComponentIndex];
	}

	inline bool operator==(const FIntVector4& Other) const
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z && W == Other.W;
	}


	inline bool operator!=(const FIntVector4& Other) const
	{
		return X != Other.X || Y != Other.Y || Z != Other.Z || W != Other.W;
	}
};

struct FUintVector4
{
	uint32_t X, Y, Z, W;

	inline FUintVector4()
	{
	}

	inline FUintVector4(uint32_t InX, uint32_t InY, uint32_t InZ, uint32_t InW)
		: X(InX)
		, Y(InY)
		, Z(InZ)
		, W(InW)
	{
	}

	inline explicit FUintVector4(uint32_t InValue)
		: X(InValue)
		, Y(InValue)
		, Z(InValue)
		, W(InValue)
	{
	}

	inline FUintVector4(EForceInit)
		: X(0)
		, Y(0)
		, Z(0)
		, W(0)
	{
	}

	inline const uint32_t& operator[](int32_t ComponentIndex) const
	{
		return (&X)[ComponentIndex];
	}


	inline uint32_t& operator[](int32_t ComponentIndex)
	{
		return (&X)[ComponentIndex];
	}

	inline bool operator==(const FUintVector4& Other) const
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z && W == Other.W;
	}


	inline bool operator!=(const FUintVector4& Other) const
	{
		return X != Other.X || Y != Other.Y || Z != Other.Z || W != Other.W;
	}
};

//template <> struct TIsPODType<FIntVector> { enum { Value = true }; };
//template <> struct TIsPODType<FIntVector4> { enum { Value = true }; };
//template <> struct TIsPODType<FUintVector4> { enum { Value = true }; };


namespace std
{
	template<>
	struct is_pod<FIntVector>
	{
		static constexpr bool  value = true;
	};

	template<>
	struct is_pod<FIntVector4>
	{
		static constexpr bool  value = true;
	};

	template<>
	struct is_pod<FUintVector4>
	{
		static constexpr bool  value = true;
	};
}
