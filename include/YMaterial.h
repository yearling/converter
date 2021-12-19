#pragma once
#include <string>
#include "Fbxsdk.h"
struct YMaterial
{
public:
	YMaterial();
	void InitFromFbx(const FbxSurfaceMaterial* surface_material);
	std::string name;
	const FbxSurfaceMaterial* fbx_material=nullptr;

};

struct YFbxMaterialCombine
{
	YMaterial* material=nullptr;
	FbxSurfaceMaterial* fbx_material=nullptr;
};