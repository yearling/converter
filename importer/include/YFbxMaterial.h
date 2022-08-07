#pragma once
#include <string>
#include "Fbxsdk.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include "Engine/YRawMesh.h"

std::shared_ptr<YFbxMaterial> GenerateFbxMaterial(const FbxSurfaceMaterial* surface_material, const std::vector<std::string>& uv_set);


struct YFbxMaterialCombine
{
	YFbxMaterial* material=nullptr;
	FbxSurfaceMaterial* fbx_material=nullptr;
};