#include "YMaterial.h"
YMaterial::YMaterial()
{

}

void YMaterial::InitFromFbx(const FbxSurfaceMaterial* surface_material)
{
	fbx_material = surface_material;
	name = surface_material->GetName();

}

