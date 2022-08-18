#pragma once
#include "Engine/YCommonHeader.h"
#include "Math/YMath.h"
#include <string>
#include "Math/YVector.h"
#include "Math/YMatrix.h"
#include "Math/YRotator.h"
#include "Math/YQuaterion.h"
#include <vector>
#include <unordered_map>

template<class TEnum> class TEnumAsByte;

class YArchive
{
public:
	YArchive();
	virtual	~YArchive();
	bool IsReading() { return is_reading_; }
	bool IsWriting() { return is_writing_; }
	void SetIsReading(bool is_reading) { is_reading_ = is_reading; }
	void SetIsWriting(bool is_writing) { is_writing_ = is_writing; }
	virtual int64 Tell()
	{
		return INDEX_NONE;
	}

	virtual int64 TotalSize()
	{
		return INDEX_NONE;
	}
	virtual bool AtEnd()
	{
		int64 Pos = Tell();

		return ((Pos != INDEX_NONE) && (Pos >= TotalSize()));
	}

	virtual void Seek(int64 InPos) { }

	virtual void Flush() { }

	virtual bool Close()
	{
		return !ArIsError;
	}

	virtual bool GetError()
	{
		return ArIsError;
	}

	void SetError()
	{
		ArIsError = false;
	}

	FORCEINLINE bool IsByteSwapping()
	{
#if PLATFORM_LITTLE_ENDIAN
		const bool SwapBytes = false;
#else
		bool SwapBytes = this->IsPersistent();
#endif
		return SwapBytes;
	}

	// Used to do byte swapping on small items. This does not happen usually, so we don't want it inline
	void ByteSwap(void* V, int32 Length);

	FORCEINLINE YArchive& ByteOrderSerialize(void* V, int32 Length)
	{
		Serialize(V, Length);
		if (IsByteSwapping())
		{
			// Transferring between memory and file, so flip the byte order.
			ByteSwap(V, Length);
		}
		return *this;
	}
	virtual void Serialize(void* V, int64 Length) { }

	virtual void SerializeBits(void* V, int64 LengthBits)
	{
		Serialize(V, (LengthBits + 7) / 8);

		if (IsReading() && (LengthBits % 8) != 0)
		{
			((uint8*)V)[LengthBits / 8] &= ((1 << (LengthBits & 7)) - 1);
		}
	}
	FORCEINLINE friend	YArchive& operator<<(YArchive& ar, bool& value)
	{
		ar.ByteOrderSerialize(&value, sizeof(bool));
		return ar;
	}

    /*  FORCEINLINE	friend	YArchive& operator<<(YArchive& ar, uint8& value)
      {
          ar.ByteOrderSerialize(&value, sizeof(uint8));
          return ar;
      }

      FORCEINLINE	friend	YArchive& operator<<(YArchive& ar, int8& value)
      {
          ar.ByteOrderSerialize(&value, sizeof(int8));
          return ar;
      }*/
	FORCEINLINE	friend	YArchive& operator<<(YArchive& ar, int32& value)
	{
		ar.ByteOrderSerialize(&value, sizeof(int32));
		return ar;
	}
	FORCEINLINE	friend YArchive& operator<<(YArchive& ar, uint32_t& value)
	{
		ar.ByteOrderSerialize(&value, sizeof(uint32_t));
		return ar;
	}
	FORCEINLINE friend YArchive& operator<<(YArchive& ar, float& value)
	{
		ar.ByteOrderSerialize(&value, sizeof(float));
		return ar;
	}

	FORCEINLINE friend YArchive& operator<<(YArchive& ar, double& value)
	{
		ar.ByteOrderSerialize(&value, sizeof(double));
		return ar;
	}
	FORCEINLINE friend YArchive& operator<<(YArchive& ar, char& value)
	{
		ar.ByteOrderSerialize(&value, sizeof(char));
		return ar;
	}
	FORCEINLINE friend YArchive& operator<<(YArchive& ar, unsigned char& value)
	{
		ar.ByteOrderSerialize(&value, sizeof(unsigned char));
		return ar;
	}

	FORCEINLINE friend YArchive& operator<<(YArchive& ar, int64& value)
	{
		ar.ByteOrderSerialize(&value, sizeof(int64));
		return ar;
	}

	FORCEINLINE friend YArchive& operator<<(YArchive& ar, uint64& value)
	{
		ar.ByteOrderSerialize(&value, sizeof(uint64));
		return ar;
	}

    template<class TEnum>
    FORCEINLINE friend YArchive& operator<<(YArchive& Ar, TEnumAsByte<TEnum>& Value)
    {
        Ar.Serialize(&Value, 1);
        return Ar;
    }
	//template <
	//	typename EnumType,
	//	typename = typename std::enable_if<std::is_enum<EnumType>::value, EnumType>::type
	//>
	//FORCEINLINE friend YArchive& operator<<(YArchive& Ar, EnumType& Value)
	//{
	//	return Ar << (std::underlying_type<EnumType>&)Value;
	//}
	friend YArchive& operator<<(YArchive& ar, std::string& value);
	friend YArchive& operator<<(YArchive& ar, YVector2& value);
	friend YArchive& operator<<(YArchive& ar, YVector& value);
	friend YArchive& operator<<(YArchive& ar, YVector4& value);
	friend YArchive& operator<<(YArchive& ar, YMatrix& value);
	friend YArchive& operator<<(YArchive& ar, YRotator& value);
	friend YArchive& operator<<(YArchive& ar, YQuat& value);
protected:
	uint8 is_reading_ : 1;
	uint8 is_writing_ : 1;
	/** Whether this archive contains errors. */
	uint8 ArIsError : 1;
};

template <typename T>
typename std::enable_if<std::is_pod<T>::value>::type SFINAE_Operator(YArchive& ar, std::vector<T>& value)
{
	if (ar.IsReading())
	{
		uint32_t value_element_size = 0;
		ar << value_element_size;
		value.resize(value_element_size);
		if (value_element_size > 0)
		{
			ar.Serialize(value.data(), sizeof(T) * value_element_size);
		}
	}
	else
	{
		uint32_t value_element_size = (uint32_t)value.size();
		ar << value_element_size;
		if (value_element_size > 0)
		{
			ar.Serialize(value.data(), sizeof(T) * value_element_size);
		}
	}
}

template <typename T>
typename std::enable_if<!std::is_pod<T>::value>::type SFINAE_Operator(YArchive& ar, std::vector<T>& value)
{
	if (ar.IsReading())
	{
		uint32_t vector_size = 0;
		ar << vector_size;
		value.resize(vector_size);
		for (auto& elem : value)
		{
			ar << elem;
		}
	}
	else
	{
		uint32_t value_element_size = (uint32_t)value.size();
		ar << value_element_size;
		for (auto& elem : value)
		{
			ar << elem;
		}
	}
}

template<class T>
YArchive& operator<<(YArchive& ar, std::vector<T>& value)
{
	SFINAE_Operator(ar, value);
	return ar;
}

template<class k, class v>
YArchive& operator<<(YArchive& ar, std::unordered_map<k, v>& in_map)
{
	if (ar.IsReading())
	{
		uint32_t item_count = 0;
		ar << item_count;
		for (uint32_t i = 0; i < item_count; ++i)
		{
			k k_value;
			ar << k_value;
			v v_value;
			ar << v_value;
			in_map.insert({ k_value, v_value });
		}
	}
	else
	{
		uint32_t item_count = (uint32_t)in_map.size();
		ar << item_count;
		for (auto& kv : in_map)
		{
			ar << (k)(kv.first);
			ar << kv.second;
		}
	}

	return ar;
}