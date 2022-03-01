#pragma once
struct FGenericPlatformTLS
{
	/**
	* Return false if this is an invalid TLS slot
	* @param SlotIndex the TLS index to check
	* @return true if this looks like a valid slot
	*/
	static inline bool IsValidTlsSlot(unsigned int SlotIndex)
	{
		return SlotIndex != 0xFFFFFFFF;
	}
};