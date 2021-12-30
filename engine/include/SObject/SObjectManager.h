#pragma once
#include "SObject/SObject.h"
#include <unordered_map>
#include <string>
#include "Engine/YLog.h"
#include <unordered_set>
#include <set>
#include "Engine/YReferenceCount.h"
class SObjectManager
{
public:
	SObjectManager();
	template<typename ClassType, typename...T>
	static TRefCountPtr<ClassType> ConstructInstance(T&&... Args)
	{
		assert(ClassType::IsInstance());
		TRefCountPtr<ClassType> Obj(new ClassType(Forward<T>(Args)...), true);
		GetManager().instanced_objects_.insert(TRefCountPtr<SObject>(Obj.GetReference(), true));
		return Obj;
	}

	template<typename ClassType, typename...T>
	static TRefCountPtr<ClassType> ConstructUnique(T&&... Args)
	{
		assert(!ClassType::IsInstance());
		TRefCountPtr<ClassType> Obj(new ClassType(Forward<T>(Args)...), true);
		GetManager().instanced_objects_.insert(TRefCountPtr<SObject>(Obj.GetReference(), true));
		return Obj;
	}

	template<typename ClassType, typename...T>
	static TRefCountPtr<ClassType> ConstructUnifyFromPackage(const std::string& PackagePath, T&&... Args)
	{
		assert(!ClassType::IsInstance());
		std::string PackagePathNoSuffix = PackagePath;
		// todo
		//FPaths::NormalizeFilename(PackagePathNoSuffix);
		std::string package_name = YPath::GetBaseFilename(PackagePathNoSuffix, false);
		auto find_result = GetManager().unify_objects_.find(package_name);
		if (find_result == GetManager().unify_objects_.end())
		{
			TRefCountPtr<ClassType> Obj((new ClassType(Forward<T>(Args)...)), true);
			if (Obj->LoadFromPackage(package_name))
			{
				GetManager().unify_objects_[package_name] = TRefCountPtr<SObject>(Obj.GetReference());
				return Obj;
			}
			return nullptr;
		}
		else
		{
			return TRefCountPtr<ClassType>(dynamic_cast<ClassType*>(find_result->second.GetReference()), true);
		}

		return nullptr;
	}

	void Destroy();
	void FrameDestroy();
	static SObjectManager& GetManager();
private:
	std::unordered_map<std::string, TRefCountPtr<SObject>> unify_objects_;
	std::unordered_set<TRefCountPtr<SObject>> instanced_objects_;
};