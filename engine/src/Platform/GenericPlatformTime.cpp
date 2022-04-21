#include "Platform/GenericPlatformTime.h"
#include "Engine/PlatformMacro.h"

double FGenericPlatformTime::SecondsPerCycle=0.0;

double FGenericPlatformTime::SecondsPerCycle64=0.0;

double FGenericPlatformTime::GetSecondsPerCycle64()
{
	check(SecondsPerCycle64 != 0.0);
	return SecondsPerCycle64;
}
