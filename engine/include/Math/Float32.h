// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <stdint.h>

//#include "CoreTypes.h"

/**
* 32 bit float components
*/
class FFloat32
{
public:

	union
	{
		struct
		{
//#if PLATFORM_LITTLE_ENDIAN
#if 1
			uint32_t	Mantissa : 23;
			uint32_t	Exponent : 8;
			uint32_t	Sign : 1;			
#else
			uint32_t	Sign : 1;
			uint32_t	Exponent : 8;
			uint32_t	Mantissa : 23;			
#endif
		} Components;

		float	FloatValue;
	};

	/**
	 * Constructor
	 * 
	 * @param InValue value of the float.
	 */
	FFloat32(float InValue = 0.0f);
};


inline FFloat32::FFloat32(float InValue)
	: FloatValue(InValue)
{ }
