#include "SObject/SObject.h"

SObject::~SObject()
{

}

SObject::SObject()
{

}

bool SObject::LoadFromPackage(const std::string& Path)
{
	return true;
}

void SObject::SaveToPackage(const std::string& Path)
{

}

bool SObject::LoadFromJson(const Json::Value& RootJson)
{
	return true;
}

bool SObject::PostLoadOp()
{
	return true;
}
