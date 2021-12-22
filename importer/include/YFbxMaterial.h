#pragma once
#include <string>
#include "Fbxsdk.h"
struct YFbxMaterial
{
public:
	YFbxMaterial();
	void InitFromFbx(const FbxSurfaceMaterial* surface_material);
	std::string name;
	const FbxSurfaceMaterial* fbx_material=nullptr;

};

struct YFbxMaterialCombine
{
	YFbxMaterial* material=nullptr;
	FbxSurfaceMaterial* fbx_material=nullptr;
};