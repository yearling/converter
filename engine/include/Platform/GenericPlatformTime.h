// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "windows.h"


/** Contains CPU utilization data. */
struct FCPUTime
{
	/** Initialization constructor. */
	FCPUTime( float InCPUTimePct, float InCPUTimePctRelative ) 
		: CPUTimePct( InCPUTimePct )
		, CPUTimePctRelative( InCPUTimePctRelative )
	{}

	FCPUTime& operator+=( const FCPUTime& Other )
	{
		CPUTimePct += Other.CPUTimePct;
		CPUTimePctRelative += Other.CPUTimePctRelative;
		return *this;
	}

	/** Percentage CPU utilization for the last interval. */
	float CPUTimePct;

	/** Percentage CPU utilization for the last interval relative to one core, 
	 ** so if CPUTimePct is 8.0% and you have 6 core this value will be 48.0%. */
	float CPUTimePctRelative;
};


/**
* Generic implementation for most platforms
**/
struct  FGenericPlatformTime
{

	/** Updates CPU utilization, called through a delegate from the Core ticker. */
	static bool UpdateCPUTime( float DeltaTime )
	{
		return false;
	}

	/**
	 * @return structure that contains CPU utilization data.
	 */
	static FCPUTime GetCPUTime()
	{
		return FCPUTime( 0.0f, 0.0f );
	}

	/**
	* @return seconds per cycle.
	*/
	static double GetSecondsPerCycle()
	{
		return SecondsPerCycle;
	}
	/** Converts cycles to milliseconds. */
	static float ToMilliseconds( const unsigned int Cycles )
	{
		return (float)double( SecondsPerCycle * 1000.0 * Cycles );
	}

	/** Converts cycles to seconds. */
	static float ToSeconds( const unsigned int Cycles )
	{
		return (float)double( SecondsPerCycle * Cycles );
	}
	/**
	 * @return seconds per cycle.
	 */
	static double GetSecondsPerCycle64();
	/** Converts cycles to milliseconds. */
	static double ToMilliseconds64(const unsigned long long Cycles)
	{
		return ToSeconds64(Cycles) * 1000.0;
	}

	/** Converts cycles to seconds. */
	static double ToSeconds64(const unsigned long long Cycles)
	{
		return GetSecondsPerCycle64() * double(Cycles);
	}

protected:

	static double SecondsPerCycle;
	static double SecondsPerCycle64;
};
