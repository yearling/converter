#include "SObject/SObjectManager.h"
#include "SObject/SObject.h"
SObjectManager g_sobject_manager;

SObjectManager::SObjectManager()
{

}

void SObjectManager::Destroy()
{
	int DestroyUnitfyCount = 0;
	int UnifyLeft = 0;
	int DestroyInstanceCount = 0;
	int InstanceLeft = 0;
	do
	{
		UnifyLeft = 0;
		DestroyUnitfyCount = 0;
		for (auto iter = unify_objects_.begin(); iter!= unify_objects_.end();)
		{
			if (iter->second.GetRefCount() == 1)
			{
				iter = unify_objects_.erase(iter);
				DestroyUnitfyCount++;
			}
			else
			{
				UnifyLeft++;
				iter++;
			}
		}

		DestroyInstanceCount = 0;
		InstanceLeft = 0;
		for(auto iter=instanced_objects_.begin();iter!= instanced_objects_.end();)
		{
				if ((*iter).GetRefCount() == 1)
				{
					iter = instanced_objects_.erase(iter);
					DestroyInstanceCount++;
				}
				else
				{
					iter++;
					InstanceLeft++;
				}
		}

	} while (DestroyUnitfyCount || DestroyInstanceCount);
	assert(!UnifyLeft);
	assert(!InstanceLeft);
}

void SObjectManager::FrameDestroy()
{
	for (auto iter = unify_objects_.begin(); iter != unify_objects_.end();)
	{
		if (iter->second.GetRefCount() == 1)
		{
			iter = unify_objects_.erase(iter);
		}
		else
		{
			iter++;
		}
	}

	for (std::unordered_set< TRefCountPtr<SObject>>::iterator iter = instanced_objects_.begin(); iter != instanced_objects_.end();)
	{
		if ((*iter).GetRefCount() == 1)
		{
			iter = instanced_objects_.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}
