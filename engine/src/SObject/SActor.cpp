#include "SObject/SActor.h"
#include "SObject/SComponent.h"

SActor::SActor()
{
}

SActor::~SActor()
{

}


bool SActor::LoadFromJson(Json::Value& RootJson)
{
	return true;
}

bool SActor::PostLoadOp()
{
	bool bSuccess = true;
	for (TRefCountPtr<SComponent>& Component : Components)
	{
		bSuccess &= Component->PostLoadOp();
	}
	return bSuccess;
}
