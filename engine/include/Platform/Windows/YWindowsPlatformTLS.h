#pragma once
#include "Platform/YGenericPlatformTLS.h"
#include <windows.h>
/**
* Windows implementation of the TLS OS functions.
*/
struct  FWindowsPlatformTLS
	: public FGenericPlatformTLS
{
	/**
	* Returns the currently executing thread's identifier.
	*
	* @return The thread identifier.
	*/
	static inline unsigned int GetCurrentThreadId(void)
	{
		return ::GetCurrentThreadId();
	}

	/**
	* Allocates a thread local store slot.
	*
	* @return The index of the allocated slot.
	*/
	static inline unsigned int AllocTlsSlot(void)
	{
		return ::TlsAlloc();
	}

	/**
	* Sets a value in the specified TLS slot.
	*
	* @param SlotIndex the TLS index to store it in.
	* @param Value the value to store in the slot.
	*/
	static inline void SetTlsValue(unsigned int SlotIndex, void* Value)
	{
		::TlsSetValue(SlotIndex, Value);
	}

	/**
	* Reads the value stored at the specified TLS slot.
	*
	* @param SlotIndex The index of the slot to read.
	* @return The value stored in the slot.
	*/
	static inline void* GetTlsValue(unsigned int SlotIndex)
	{
		return ::TlsGetValue(SlotIndex);
	}

	/**
	* Frees a previously allocated TLS slot
	*
	* @param SlotIndex the TLS index to store it in
	*/
	static inline void FreeTlsSlot(unsigned int SlotIndex)
	{
		::TlsFree(SlotIndex);
	}
};


typedef FWindowsPlatformTLS FPlatformTLS;