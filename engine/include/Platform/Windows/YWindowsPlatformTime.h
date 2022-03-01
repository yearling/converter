#pragma once

#include "Platform/GenericPlatformTime.h"
struct  FWindowsPlatformTime
	: public FGenericPlatformTime
{
	static double InitTiming();

	static inline double Seconds()
	{
		::LARGE_INTEGER Cycles;
		::QueryPerformanceCounter(&Cycles);

		// add big number to make bugs apparent where return value is being passed to float
		return Cycles.QuadPart * GetSecondsPerCycle() + 16777216.0;
	}

	static inline unsigned int Cycles()
	{
		::LARGE_INTEGER Cycles;
		::QueryPerformanceCounter(&Cycles);
		return (int)Cycles.QuadPart;
	}

	static inline unsigned long long Cycles64()
	{
		::LARGE_INTEGER Cycles;
		QueryPerformanceCounter(&Cycles);
		return Cycles.QuadPart;
	}


	static void SystemTime(int& Year, int& Month, int& DayOfWeek, int& Day, int& Hour, int& Min, int& Sec, int& MSec);
	static void UtcTime(int& Year, int& Month, int& DayOfWeek, int& Day, int& Hour, int& Min, int& Sec, int& MSec);

	static bool UpdateCPUTime(float DeltaTime);
	static FCPUTime GetCPUTime();

protected:

	/** Percentage CPU utilization for the last interval relative to one core. */
	static float CPUTimePctRelative;
};


typedef FWindowsPlatformTime FPlatformTime;