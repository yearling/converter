#include "Engine/YArchive.h"
#include <forward_list>
YArchive::YArchive()
{

}

YArchive::~YArchive()
{

}

void YArchive::ByteSwap(void* V, int32 Length)
{
	uint8* Ptr = (uint8*)V;
	int32 Top = Length - 1;
	int32 Bottom = 0;
	while (Bottom < Top)
	{
		std::swap(Ptr[Top--], Ptr[Bottom++]);
	}
}

YArchive& operator<<(YArchive& ar, std::string& value)
{
	if (ar.IsReading())
	{
		uint32 str_size = 0;
		ar << str_size;
		if (!str_size)
		{
			value.clear();
		}
		else
		{
			value.resize(str_size);
			ar.Serialize(value.data(), sizeof(char)*str_size);
		}
	}
	else
	{
		uint32 str_len = (uint32)value.size();
		ar << str_len;
		ar.Serialize(value.data(), sizeof(char) * value.size());
	}
	return ar;
}

YArchive& operator<<(YArchive& ar, YVector2& value)
{
	return ar << value.x << value.y;
}
YArchive& operator<<(YArchive& ar, YVector& value)
{
	return 	ar << value.x << value.y << value.z;
}
YArchive& operator<<(YArchive& ar, YVector4& value)
{
	return ar << value.x << value.y << value.z << value.w;
}
YArchive& operator<<(YArchive& ar, YMatrix& value)
{
	ar.Serialize(&value.m[0][0],sizeof(float)*16);
	return ar;
}
YArchive& operator<<(YArchive& ar, YRotator& value)
{
	return ar << value.pitch << value.yaw << value.roll;
}
YArchive& operator<<(YArchive& ar, YQuat& value)
{
	return ar << value.x << value.y << value.z << value.w;
}
