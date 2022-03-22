#include "Platform/Windows/YWindowsPlatformMisc.h"
#include <windows.h>
#include "Platform/YPlatformAffinity.h"
#include <cassert>

int FWindowsPlatformMisc::NumberOfCoresIncludingHyperthreads()
{
	static int CoreCount = 0;
	if (CoreCount == 0)
	{
		// Get the number of logical processors, including hyperthreaded ones.
		SYSTEM_INFO SI;
		GetSystemInfo(&SI);
		CoreCount = (int)SI.dwNumberOfProcessors;
	}
	return CoreCount;
}
int FWindowsPlatformMisc::NumberOfCores()
{
	static int CoreCount = 0;
	if (CoreCount == 0)
	{
		// Get only physical cores
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION InfoBuffer = NULL;
		::DWORD BufferSize = 0;

		// Get the size of the buffer to hold processor information.
		::BOOL Result = GetLogicalProcessorInformation(InfoBuffer, &BufferSize);
		assert(!Result && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
		assert(BufferSize > 0);

		// Allocate the buffer to hold the processor info.
		InfoBuffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(BufferSize);
		assert(InfoBuffer);

		// Get the actual information.
		Result = GetLogicalProcessorInformation(InfoBuffer, &BufferSize);
		assert(Result);

		// Count physical cores
		const int InfoCount = (int)(BufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
		for (int Index = 0; Index < InfoCount; ++Index)
		{
			SYSTEM_LOGICAL_PROCESSOR_INFORMATION* Info = &InfoBuffer[Index];
			if (Info->Relationship == RelationProcessorCore)
			{
				CoreCount++;
			}
		}
		free(InfoBuffer);
	}
	return CoreCount;
}

