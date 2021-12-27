#pragma once
#include "SObject/SObject.h"
#include <unordered_map>
#include <string>
#include "Engine/YLog.h"
#include <unordered_set>
#include <set>
#include "Engine/YReferenceCount.h"
extern class SObjectManager g_sobject_manager;
class SObjectManager
{
public:
	SObjectManager();
	template<typename ClassType, typename...T>
	static TRefCountPtr<ClassType> ConstructInstance(T&&... Args)
	{
		assert(ClassType::IsInstance());
		TRefCountPtr<ClassType> Obj(new ClassType(Forward<T>(Args)...), true);
		g_sobject_manager.instanced_objects_.insert(TRefCountPtr<SObject>(Obj.GetReference(), true));
		return Obj;
	}

	template<typename ClassType, typename...T>
	static TRefCountPtr<ClassType> ConstructUnique(T&&... Args)
	{
		assert(!ClassType::IsInstance());
		TRefCountPtr<ClassType> Obj(new ClassType(Forward<T>(Args)...), true);
		g_sobject_manager.instanced_objects_.insert(TRefCountPtr<SObject>(Obj.GetReference(), true));
		return Obj;
	}

	template<typename ClassType, typename...T>
	static TRefCountPtr<ClassType> ConstructUnifyFromPackage(const std::string& PackagePath, T&&... Args)
	{
		assert(!ClassType::IsInstance());
		std::string PackagePathNoSuffix = PackagePath;
		FPaths::NormalizeFilename(PackagePathNoSuffix);
		FName PackageFName(*(FPaths::GetBaseFilename(PackagePathNoSuffix, false)));
		auto FindResult = g_sobject_manager.unify_objects_.Find(PackageFName);
		if (FindResult != g_sobject_manager.unify_objects_.end())
		{
			return TRefCountPtr<ClassType>(dynamic_cast<ClassType*>(FindResult.second.GetReference()), true);
		}
		else
		{
			TRefCountPtr<ClassType> Obj((new ClassType(Forward<T>(Args)...)), true);
			if (Obj->LoadFromPackage(PackagePath))
			{
				g_sobject_manager.unify_objects_.insert[PackageFName] = TRefCountPtr<SObject>(Obj.GetReference());
				return Obj;
			}
			else
			{
				return nullptr;
			}
		}
	}

	//template<typename ClassType,typename Pa, typename...T>
	//static TRefCountPtr<ClassType> ConstructUnifyFromPackage(Pa Var, T&&... Args)
	//{
	//	return	ConstructUnifyFromPackage<ClassType>(Var, Forward<T>(Args...));
	//}

	void Destroy();
	void FrameDestroy();

private:
	std::unordered_map<std::string, TRefCountPtr<SObject>> unify_objects_;
	std::unordered_set<TRefCountPtr<SObject>> instanced_objects_;
	std::unordered_set<int> fuck_set;
};