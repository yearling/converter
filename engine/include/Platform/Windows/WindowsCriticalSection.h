// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <windows.h>

class FName;

/**
 * This is the Windows version of a critical section. It uses an aggregate
 * CRITICAL_SECTION to implement its locking.
 */
class FWindowsCriticalSection
{
	/**
	 * The windows specific critical section
	 */
	::CRITICAL_SECTION CriticalSection;

public:

	/**
	 * Constructor that initializes the aggregated critical section
	 */
	inline FWindowsCriticalSection()
	{
		::InitializeCriticalSection(&CriticalSection);
		::SetCriticalSectionSpinCount(&CriticalSection,4000);
	}

	/**
	 * Destructor cleaning up the critical section
	 */
	inline ~FWindowsCriticalSection()
	{
		::DeleteCriticalSection(&CriticalSection);
	}

	/**
	 * Locks the critical section
	 */
	inline void Lock()
	{
		// Spin first before entering critical section, causing ring-0 transition and context switch.
		if(::TryEnterCriticalSection(&CriticalSection) == 0 )
		{
			::EnterCriticalSection(&CriticalSection);
		}
	}

	/**
	 * Attempt to take a lock and returns whether or not a lock was taken.
	 *
	 * @return true if a lock was taken, false otherwise.
	 */
	inline bool TryLock()
	{
		if (::TryEnterCriticalSection(&CriticalSection))
		{
			return true;
		}
		return false;
	}

	/**
	 * Releases the lock on the critical section
	 */
	inline void Unlock()
	{
		::LeaveCriticalSection(&CriticalSection);
	}

private:
	FWindowsCriticalSection(const FWindowsCriticalSection&);
	FWindowsCriticalSection& operator=(const FWindowsCriticalSection&);
};

///** System-Wide Critical Section for windows using mutex */
//class  FWindowsSystemWideCriticalSection
//{
//public:
//	/** Construct a named, system-wide critical section and attempt to get access/ownership of it */
//	explicit FWindowsSystemWideCriticalSection(const class std::string& InName, FTimespan InTimeout = FTimespan::Zero());
//
//	/** Destructor releases system-wide critical section if it is currently owned */
//	~FWindowsSystemWideCriticalSection();
//
//	/**
//	 * Does the calling thread have ownership of the system-wide critical section?
//	 *
//	 * @return True if obtained. WARNING: Returns true for an owned but previously abandoned locks so shared resources can be in undetermined states. You must handle shared data robustly.
//	 */
//	bool IsValid() const;
//
//	/** Releases system-wide critical section if it is currently owned */
//	void Release();
//
//private:
//	FWindowsSystemWideCriticalSection(const FWindowsSystemWideCriticalSection&);
//	FWindowsSystemWideCriticalSection& operator=(const FWindowsSystemWideCriticalSection&);
//
//private:
//	Windows::HANDLE Mutex;
//};

typedef FWindowsCriticalSection FCriticalSection;
//typedef FWindowsSystemWideCriticalSection FSystemWideCriticalSection;
