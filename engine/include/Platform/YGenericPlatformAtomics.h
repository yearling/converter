#pragma once
struct FGenericPlatformAtomics
{
	/**
	* @return true, if the processor we are running on can execute compare and exchange 128-bit operation.
	* @see cmpxchg16b, early AMD64 processors don't support this operation.
	*/
	static inline bool CanUseCompareExchange128()
	{
		return false;
	}

protected:
	/**
	* Checks if a pointer is aligned and can be used with atomic functions.
	*
	* @param Ptr - The pointer to check.
	*
	* @return true if the pointer is aligned, false otherwise.
	*/
	static inline bool IsAligned(const volatile void* Ptr, const unsigned int Alignment = sizeof(void*))
	{
		return !(unsigned long long(Ptr) & (Alignment - 1));
	}
};
