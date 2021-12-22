#include "YFbxMaterial.h"
YFbxMaterial::YFbxMaterial()
{

}

void YFbxMaterial::InitFromFbx(const FbxSurfaceMaterial* surface_material)
{
	fbx_material = surface_material;
	name = surface_material->GetName();

}

