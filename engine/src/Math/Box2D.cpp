// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Box2D.cpp: Implements the FBox2D class.
=============================================================================*/

/* FBox2D structors
 *****************************************************************************/

#include "Math/Box2D.h"

FBox2D::FBox2D(const YVector2* Points, const int32 Count)
	: Min(0.f, 0.f)
	, Max(0.f, 0.f)
	, bIsValid(false)
{
	for (int32 PointItr = 0; PointItr < Count; PointItr++)
	{
		*this += Points[PointItr];
	}
}


FBox2D::FBox2D(const std::vector<YVector2>& Points)
	: Min(0.f, 0.f)
	, Max(0.f, 0.f)
	, bIsValid(false)
{
	for(const YVector2& EachPoint : Points)
	{
		*this += EachPoint;
	}
}
