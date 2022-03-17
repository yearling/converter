// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Math/YMath.h"
#include "Math/YVector.h"
#include <string>
#include <cassert>
#include <vector>
class FFloat16Color;
struct FColor;
/**
 * Enum for the different kinds of gamma spaces we expect to need to convert from/to.
 */
enum class EGammaSpace
{
	/** No gamma correction is applied to this space, the incoming colors are assumed to already be in linear space. */
	Linear,
	/** A simplified sRGB gamma correction is applied, pow(1/2.2). */
	Pow22,
	/** Use the standard sRGB conversion. */
	sRGB,
};


/**
 * A linear, 32-bit/component floating point RGBA color.
 */
struct FLinearColor
{
	float	R,
		G,
		B,
		A;

	/** Static lookup table used for FColor -> FLinearColor conversion. Pow(2.2) */
	static float Pow22OneOver255Table[256];

	/** Static lookup table used for FColor -> FLinearColor conversion. sRGB */
	static float sRGBToLinearTable[256];

	inline FLinearColor() {}
	inline explicit FLinearColor(EForceInit)
		: R(0), G(0), B(0), A(0)
	{}
	inline FLinearColor(float InR, float InG, float InB, float InA = 1.0f) : R(InR), G(InG), B(InB), A(InA) {}

	/**
	 * Converts an FColor which is assumed to be in sRGB space, into linear color space.
	 * @param Color The sRGB color that needs to be converted into linear space.
	 */
	FLinearColor(const FColor& Color);

	FLinearColor(const YVector& Vector);

	explicit FLinearColor(const YVector4& Vector);

	explicit FLinearColor(const FFloat16Color& C);

	// Serializer.

	//friend FArchive& operator<<(FArchive& Ar,FLinearColor& Color)
	//{
		//return Ar << Color.R << Color.G << Color.B << Color.A;
	//}

	//bool Serialize( FArchive& Ar )
	//{
		//Ar << *this;
		//return true;
	//}

	// Conversions.
	FColor ToRGBE() const;

	/**
	 * Converts an FColor coming from an observed sRGB output, into a linear color.
	 * @param Color The sRGB color that needs to be converted into linear space.
	 */
	static FLinearColor FromSRGBColor(const FColor& Color);

	/**
	 * Converts an FColor coming from an observed Pow(1/2.2) output, into a linear color.
	 * @param Color The Pow(1/2.2) color that needs to be converted into linear space.
	 */
	static FLinearColor FromPow22Color(const FColor& Color);

	// Operators.

	inline float& Component(int32_t Index)
	{
		return (&R)[Index];
	}

	inline const float& Component(int32_t Index) const
	{
		return (&R)[Index];
	}

	inline FLinearColor operator+(const FLinearColor& ColorB) const
	{
		return FLinearColor(
			this->R + ColorB.R,
			this->G + ColorB.G,
			this->B + ColorB.B,
			this->A + ColorB.A
		);
	}
	inline FLinearColor& operator+=(const FLinearColor& ColorB)
	{
		R += ColorB.R;
		G += ColorB.G;
		B += ColorB.B;
		A += ColorB.A;
		return *this;
	}

	inline FLinearColor operator-(const FLinearColor& ColorB) const
	{
		return FLinearColor(
			this->R - ColorB.R,
			this->G - ColorB.G,
			this->B - ColorB.B,
			this->A - ColorB.A
		);
	}
	inline FLinearColor& operator-=(const FLinearColor& ColorB)
	{
		R -= ColorB.R;
		G -= ColorB.G;
		B -= ColorB.B;
		A -= ColorB.A;
		return *this;
	}

	inline FLinearColor operator*(const FLinearColor& ColorB) const
	{
		return FLinearColor(
			this->R * ColorB.R,
			this->G * ColorB.G,
			this->B * ColorB.B,
			this->A * ColorB.A
		);
	}
	inline FLinearColor& operator*=(const FLinearColor& ColorB)
	{
		R *= ColorB.R;
		G *= ColorB.G;
		B *= ColorB.B;
		A *= ColorB.A;
		return *this;
	}

	inline FLinearColor operator*(float Scalar) const
	{
		return FLinearColor(
			this->R * Scalar,
			this->G * Scalar,
			this->B * Scalar,
			this->A * Scalar
		);
	}

	inline FLinearColor& operator*=(float Scalar)
	{
		R *= Scalar;
		G *= Scalar;
		B *= Scalar;
		A *= Scalar;
		return *this;
	}

	inline FLinearColor operator/(const FLinearColor& ColorB) const
	{
		return FLinearColor(
			this->R / ColorB.R,
			this->G / ColorB.G,
			this->B / ColorB.B,
			this->A / ColorB.A
		);
	}
	inline FLinearColor& operator/=(const FLinearColor& ColorB)
	{
		R /= ColorB.R;
		G /= ColorB.G;
		B /= ColorB.B;
		A /= ColorB.A;
		return *this;
	}

	inline FLinearColor operator/(float Scalar) const
	{
		const float	InvScalar = 1.0f / Scalar;
		return FLinearColor(
			this->R * InvScalar,
			this->G * InvScalar,
			this->B * InvScalar,
			this->A * InvScalar
		);
	}
	inline FLinearColor& operator/=(float Scalar)
	{
		const float	InvScalar = 1.0f / Scalar;
		R *= InvScalar;
		G *= InvScalar;
		B *= InvScalar;
		A *= InvScalar;
		return *this;
	}

	// clamped in 0..1 range
	inline FLinearColor GetClamped(float InMin = 0.0f, float InMax = 1.0f) const
	{
		FLinearColor Ret;

		Ret.R = YMath::Clamp(R, InMin, InMax);
		Ret.G = YMath::Clamp(G, InMin, InMax);
		Ret.B = YMath::Clamp(B, InMin, InMax);
		Ret.A = YMath::Clamp(A, InMin, InMax);

		return Ret;
	}

	/** Comparison operators */
	inline bool operator==(const FLinearColor& ColorB) const
	{
		return this->R == ColorB.R && this->G == ColorB.G && this->B == ColorB.B && this->A == ColorB.A;
	}
	inline bool operator!=(const FLinearColor& Other) const
	{
		return this->R != Other.R || this->G != Other.G || this->B != Other.B || this->A != Other.A;
	}

	// Error-tolerant comparison.
	inline bool Equals(const FLinearColor& ColorB, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return YMath::Abs(this->R - ColorB.R) < Tolerance && YMath::Abs(this->G - ColorB.G) < Tolerance && YMath::Abs(this->B - ColorB.B) < Tolerance && YMath::Abs(this->A - ColorB.A) < Tolerance;
	}

	FLinearColor CopyWithNewOpacity(float NewOpacicty) const
	{
		FLinearColor NewCopy = *this;
		NewCopy.A = NewOpacicty;
		return NewCopy;
	}

	/**
	 * Converts byte hue-saturation-brightness to floating point red-green-blue.
	 */
	static  FLinearColor FGetHSV(uint8_t H, uint8_t S, uint8_t V);

	/**
	* Makes a random but quite nice color.
	*/
	static  FLinearColor MakeRandomColor();

	/**
	* Converts temperature in Kelvins of a black body radiator to RGB chromaticity.
	*/
	static  FLinearColor MakeFromColorTemperature(float Temp);

	/**
	 * Euclidean distance between two points.
	 */
	static inline float Dist(const FLinearColor& V1, const FLinearColor& V2)
	{
		return YMath::Sqrt(YMath::Square(V2.R - V1.R) + YMath::Square(V2.G - V1.G) + YMath::Square(V2.B - V1.B) + YMath::Square(V2.A - V1.A));
	}

	/**
	 * Generates a list of sample points on a Bezier curve defined by 2 points.
	 *
	 * @param	ControlPoints	Array of 4 Linear Colors (vert1, controlpoint1, controlpoint2, vert2).
	 * @param	NumPoints		Number of samples.
	 * @param	OutPoints		Receives the output samples.
	 * @return					Path length.
	 */
	static  float EvaluateBezier(const FLinearColor* ControlPoints, int32_t NumPoints, std::vector<FLinearColor>& OutPoints);

	/** Converts a linear space RGB color to an HSV color */
	FLinearColor LinearRGBToHSV() const;

	/** Converts an HSV color to a linear space RGB color */
	FLinearColor HSVToLinearRGB() const;

	/**
	 * Linearly interpolates between two colors by the specified progress amount.  The interpolation is performed in HSV color space
	 * taking the shortest path to the new color's hue.  This can give better results than YMath::Lerp(), but is much more expensive.
	 * The incoming colors are in RGB space, and the output color will be RGB.  The alpha value will also be interpolated.
	 *
	 * @param	From		The color and alpha to interpolate from as linear RGBA
	 * @param	To			The color and alpha to interpolate to as linear RGBA
	 * @param	Progress	Scalar interpolation amount (usually between 0.0 and 1.0 inclusive)
	 * @return	The interpolated color in linear RGB space along with the interpolated alpha value
	 */
	static  FLinearColor LerpUsingHSV(const FLinearColor& From, const FLinearColor& To, const float Progress);

	/** Quantizes the linear color and returns the result as a FColor.  This bypasses the SRGB conversion. */
	FColor Quantize() const;

	/** Quantizes the linear color with rounding and returns the result as a FColor.  This bypasses the SRGB conversion. */
	FColor QuantizeRound() const;

	/** Quantizes the linear color and returns the result as a FColor with optional sRGB conversion and quality as goal. */
	FColor ToFColor(const bool bSRGB) const;

	/**
	 * Returns a desaturated color, with 0 meaning no desaturation and 1 == full desaturation
	 *
	 * @param	Desaturation	Desaturation factor in range [0..1]
	 * @return	Desaturated color
	 */
	FLinearColor Desaturate(float Desaturation) const;

	/** Computes the perceptually weighted luminance value of a color. */
	inline float ComputeLuminance() const
	{
		return R * 0.3f + G * 0.59f + B * 0.11f;
	}

	/**
	 * Returns the maximum value in this color structure
	 *
	 * @return The maximum color channel value
	 */
	inline float GetMax() const
	{
		return YMath::Max(YMath::Max(YMath::Max(R, G), B), A);
	}

	/** useful to detect if a light contribution needs to be rendered */
	bool IsAlmostBlack() const
	{
		return YMath::Square(R) < DELTA && YMath::Square(G) < DELTA && YMath::Square(B) < DELTA;
	}

	/**
	 * Returns the minimum value in this color structure
	 *
	 * @return The minimum color channel value
	 */
	inline float GetMin() const
	{
		return YMath::Min(YMath::Min(YMath::Min(R, G), B), A);
	}

	inline float GetLuminance() const
	{
		return R * 0.3f + G * 0.59f + B * 0.11f;
	}

	std::string ToString() const
	{
		//return FString::Printf(TEXT("(R=%f,G=%f,B=%f,A=%f)"),R,G,B,A);
		return "";
	}

	/**
	 * Initialize this Color based on an FString. The String is expected to contain R=, G=, B=, A=.
	 * The FLinearColor will be bogus when InitFromString returns false.
	 *
	 * @param InSourceString FString containing the color values.
	 * @return true if the R,G,B values were read successfully; false otherwise.
	 */
	bool InitFromString(const std::string& InSourceString)
	{
		assert(0);
		bool bSuccessful = false;
		R = G = B = 0.f;
		A = 1.f;

		// The initialization is only successful if the R, G, and B values can all be parsed from the string
		//const bool bSuccessful = FParse::Value( *InSourceString, TEXT("R=") , R ) && FParse::Value( *InSourceString, TEXT("G="), G ) && FParse::Value( *InSourceString, TEXT("B="), B );

		// Alpha is optional, so don't factor in its presence (or lack thereof) in determining initialization success
		//FParse::Value( *InSourceString, TEXT("A="), A );

		return bSuccessful;
	}

	// Common colors.	
	static  const FLinearColor White;
	static  const FLinearColor Gray;
	static  const FLinearColor Black;
	static  const FLinearColor Transparent;
	static  const FLinearColor Red;
	static  const FLinearColor Green;
	static  const FLinearColor Blue;
	static  const FLinearColor Yellow;
};

inline FLinearColor operator*(float Scalar, const FLinearColor& Color)
{
	return Color.operator*(Scalar);
}

//
//	FColor
//	Stores a color with 8 bits of precision per channel.  
//	Note: Linear color values should always be converted to gamma space before stored in an FColor, as 8 bits of precision is not enough to store linear space colors!
//	This can be done with FLinearColor::ToFColor(true) 
//

struct FColor
{
public:
	// Variables.
//#if PLATFORM_LITTLE_ENDIAN
#if 1
#ifdef _MSC_VER
	// Win32 x86
	union { struct { uint8_t B, G, R, A; }; uint32_t AlignmentDummy; };
#else
	// Linux x86, etc
	uint8_t B GCC_ALIGN(4);
	uint8_t G, R, A;
#endif
#else // PLATFORM_LITTLE_ENDIAN
	union { struct { uint8_t A, R, G, B; }; uint32_t AlignmentDummy; };
#endif

	uint32_t& DWColor(void) { return *((uint32_t*)this); }
	const uint32_t& DWColor(void) const { return *((uint32_t*)this); }

	// Constructors.
	inline FColor() {}
	inline explicit FColor(EForceInit)
	{
		// put these into the body for proper ordering with INTEL vs non-INTEL_BYTE_ORDER
		R = G = B = A = 0;
	}
	inline FColor(uint8_t InR, uint8_t InG, uint8_t InB, uint8_t InA = 255)
	{
		// put these into the body for proper ordering with INTEL vs non-INTEL_BYTE_ORDER
		R = InR;
		G = InG;
		B = InB;
		A = InA;

	}

	inline explicit FColor(uint32_t InColor)
	{
		DWColor() = InColor;
	}

	// Serializer.
	//friend FArchive& operator<< (FArchive &Ar, FColor &Color )
	//{
	//	return Ar << Color.DWColor();
	//}

	//bool Serialize( FArchive& Ar )
	//{
	//	Ar << *this;
	//	return true;
	//}

	// Operators.
	inline bool operator==(const FColor& C) const
	{
		return DWColor() == C.DWColor();
	}

	inline bool operator!=(const FColor& C) const
	{
		return DWColor() != C.DWColor();
	}

	inline void operator+=(const FColor& C)
	{
		R = (uint8_t)YMath::Min((int32_t)R + (int32_t)C.R, 255);
		G = (uint8_t)YMath::Min((int32_t)G + (int32_t)C.G, 255);
		B = (uint8_t)YMath::Min((int32_t)B + (int32_t)C.B, 255);
		A = (uint8_t)YMath::Min((int32_t)A + (int32_t)C.A, 255);
	}

	FLinearColor FromRGBE() const;

	/**
	 * Creates a color value from the given hexadecimal string.
	 *
	 * Supported formats are: RGB, RRGGBB, RRGGBBAA, #RGB, #RRGGBB, #RRGGBBAA
	 *
	 * @param HexString - The hexadecimal string.
	 * @return The corresponding color value.
	 * @see ToHex
	 */
	static  FColor FromHex(const std::string& HexString);

	/**
	 * Makes a random but quite nice color.
	 */
	static  FColor MakeRandomColor();

	/**
	 * Makes a color red->green with the passed in scalar (e.g. 0 is red, 1 is green)
	 */
	static  FColor MakeRedToGreenColorFromScalar(float Scalar);

	/**
	* Converts temperature in Kelvins of a black body radiator to RGB chromaticity.
	*/
	static  FColor MakeFromColorTemperature(float Temp);

	/**
	 *	@return a new FColor based of this color with the new alpha value.
	 *	Usage: const FColor& MyColor = FColorList::Green.WithAlpha(128);
	 */
	FColor WithAlpha(uint8_t Alpha) const
	{
		return FColor(R, G, B, Alpha);
	}

	/**
	 * Reinterprets the color as a linear color.
	 *
	 * @return The linear color representation.
	 */
	inline FLinearColor ReinterpretAsLinear() const
	{
		return FLinearColor(R / 255.f, G / 255.f, B / 255.f, A / 255.f);
	}

	/**
	 * Converts this color value to a hexadecimal string.
	 *
	 * The format of the string is RRGGBBAA.
	 *
	 * @return Hexadecimal string.
	 * @see FromHex, ToString
	 */
	inline std::string ToHex() const
	{
		//return FString::Printf(TEXT("%02X%02X%02X%02X"), R, G, B, A);
		assert(0);
		return "";
	}

	/**
	 * Converts this color value to a string.
	 *
	 * @return The string representation.
	 * @see ToHex
	 */
	inline std::string ToString() const
	{
		//return FString::Printf(TEXT("(R=%i,G=%i,B=%i,A=%i)"), R, G, B, A);
		assert(0);
		return "";
	}

	/**
	 * Initialize this Color based on an FString. The String is expected to contain R=, G=, B=, A=.
	 * The FColor will be bogus when InitFromString returns false.
	 *
	 * @param	InSourceString	FString containing the color values.
	 * @return true if the R,G,B values were read successfully; false otherwise.
	 */
	bool InitFromString(const std::string& InSourceString)
	{
		assert(0);
		return false;
		R = G = B = 0;
		A = 255;

		//// The initialization is only successful if the R, G, and B values can all be parsed from the string
		//const bool bSuccessful = FParse::Value( *InSourceString, TEXT("R=") , R ) && FParse::Value( *InSourceString, TEXT("G="), G ) && FParse::Value( *InSourceString, TEXT("B="), B );
		//
		//// Alpha is optional, so don't factor in its presence (or lack thereof) in determining initialization success
		//FParse::Value( *InSourceString, TEXT("A="), A );
		//
		//return bSuccessful;
	}

	/**
	 * Gets the color in a packed uint32_t format packed in the order ARGB.
	 */
	inline uint32_t ToPackedARGB() const
	{
		return (A << 24) | (R << 16) | (G << 8) | (B << 0);
	}

	/**
	 * Gets the color in a packed uint32_t format packed in the order ABGR.
	 */
	inline uint32_t ToPackedABGR() const
	{
		return (A << 24) | (B << 16) | (G << 8) | (R << 0);
	}

	/**
	 * Gets the color in a packed uint32_t format packed in the order RGBA.
	 */
	inline uint32_t ToPackedRGBA() const
	{
		return (R << 24) | (G << 16) | (B << 8) | (A << 0);
	}

	/**
	 * Gets the color in a packed uint32_t format packed in the order BGRA.
	 */
	inline uint32_t ToPackedBGRA() const
	{
		return (B << 24) | (G << 16) | (R << 8) | (A << 0);
	}

	/** Some pre-inited colors, useful for debug code */
	static  const FColor White;
	static  const FColor Black;
	static  const FColor Transparent;
	static  const FColor Red;
	static  const FColor Green;
	static  const FColor Blue;
	static  const FColor Yellow;
	static  const FColor Cyan;
	static  const FColor Magenta;
	static  const FColor Orange;
	static  const FColor Purple;
	static  const FColor Turquoise;
	static  const FColor Silver;
	static  const FColor Emerald;

private:
	/**
	 * Please use .ToFColor(true) on FLinearColor if you wish to convert from FLinearColor to FColor,
	 * with proper sRGB conversion applied.
	 *
	 * Note: Do not implement or make public.  We don't want people needlessly and implicitly converting between
	 * FLinearColor and FColor.  It's not a free conversion.
	 */
	explicit FColor(const FLinearColor& LinearColor);
};


inline uint32_t GetTypeHash(const FColor& Color)
{
	return Color.DWColor();
}


inline uint32_t GetTypeHash(const FLinearColor& LinearColor)
{
	// Note: this assumes there's no padding in FLinearColor that could contain uncompared data.
	//return FCrc::MemCrc_DEPRECATED(&LinearColor, sizeof(FLinearColor));
	assert(0);
	return 0;
}


/** Computes a brightness and a fixed point color from a floating point color. */
extern  void ComputeAndFixedColorAndIntensity(const FLinearColor& InLinearColor, FColor& OutColor, float& OutIntensity);

// These act like a POD
namespace std
{
	template<>
	struct is_pod<FColor>
	{
		static constexpr bool  value = true;
	};

	template<>
	struct is_pod<FLinearColor>
	{
		static constexpr bool  value = true;
	};
}

//template <> struct TIsPODType<FColor> { enum { Value = true }; };
//template <> struct TIsPODType<FLinearColor> { enum { Value = true }; };


/**
 * Helper struct for a 16 bit 565 color of a DXT1/3/5 block.
 */
struct FDXTColor565
{
	/** Blue component, 5 bit. */
	uint16_t B : 5;

	/** Green component, 6 bit. */
	uint16_t G : 6;

	/** Red component, 5 bit */
	uint16_t R : 5;
};


/**
 * Helper struct for a 16 bit 565 color of a DXT1/3/5 block.
 */
struct FDXTColor16
{
	union
	{
		/** 565 Color */
		FDXTColor565 Color565;
		/** 16 bit entity representation for easy access. */
		uint16_t Value;
	};
};


/**
 * Structure encompassing single DXT1 block.
 */
struct FDXT1
{
	/** Color 0/1 */
	union
	{
		FDXTColor16 Color[2];
		uint32_t Colors;
	};
	/** Indices controlling how to blend colors. */
	uint32_t Indices;
};


/**
 * Structure encompassing single DXT5 block
 */
struct FDXT5
{
	/** Alpha component of DXT5 */
	uint8_t	Alpha[8];
	/** DXT1 color component. */
	FDXT1	DXT1;
};


// Make DXT helpers act like PODs
//template <> struct TIsPODType<FDXT1> { enum { Value = true }; };
//template <> struct TIsPODType<FDXT5> { enum { Value = true }; };
//template <> struct TIsPODType<FDXTColor16> { enum { Value = true }; };
//template <> struct TIsPODType<FDXTColor565> { enum { Value = true }; };

namespace std
{
	template<>
	struct is_pod<FDXT1>
	{
		static constexpr bool  value = true;
	};

	template<>
	struct is_pod<FDXT5>
	{
		static constexpr bool  value = true;
	};

	template<>
	struct is_pod<FDXTColor16>
	{
		static constexpr bool  value = true;
	};

	template<>
	struct is_pod<FDXTColor565>
	{
		static constexpr bool  value = true;
	};
}
