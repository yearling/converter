#include "YFbxMaterial.h"
#include "fbxsdk/scene/shading/fbxsurfacematerial.h"
#include "Engine/YLog.h"
#include <xutility>
#include "fbxsdk/scene/shading/fbxfiletexture.h"

std::shared_ptr<YFbxMaterial> GenerateFbxMaterial(const FbxSurfaceMaterial* surface_material, const std::vector<std::string>& uv_set)
{

    auto func_load_material_property = [](const FbxSurfaceMaterial* fbx_material, const char* material_property, const std::vector<std::string>& uv_set, bool is_normal_map, YFbxMaterial::ParamTexture& param_tex)
    {
        FbxProperty fbx_property = fbx_material->FindProperty(material_property);
        if (fbx_property.IsValid())
        {
            int layer_texture_count = fbx_property.GetSrcObjectCount<FbxLayeredTexture>();
            if (layer_texture_count > 0)
            {
                //WARNING_INFO("Layered textures are not supported ", fbx_material->GetName());
                WARNING_INFO("Layered textures are not supported ", (const char*)fbx_material->GetName());
            }
            else
            {
                int texture_count = fbx_property.GetSrcObjectCount<FbxFileTexture>();
                for (int texture_index = 0; texture_index < texture_count; ++texture_index)
                {
                    FbxFileTexture* fbx_texture = fbx_property.GetSrcObject<FbxFileTexture>(texture_index);
                    std::string texture_file_path = fbx_texture->GetFileName();
                    FbxString uv_set_name = fbx_texture->UVSet.Get();
                    if (uv_set_name.IsEmpty())
                    {
                        uv_set_name = "UVmap_0";
                    }
                    std::string str_uv_set_name = (const char*)uv_set_name;
                    auto iter_set = std::find(uv_set.begin(), uv_set.end(), str_uv_set_name);
                    int uv_index = 0;
                    if (iter_set != uv_set.end())
                    {
                        uv_index = (int)std::distance(iter_set, uv_set.begin());
                    }
                    param_tex.texture_path = texture_file_path;
                    param_tex.is_normal_map = is_normal_map;
                    param_tex.uv_index = uv_index;
                    return true;
                }
            }

        }
        return false;
    };

    std::shared_ptr<YFbxMaterial> material = std::make_shared<YFbxMaterial>();
    material->name = surface_material->GetName();

    //diffuse
    {
        YFbxMaterial::ParamTexture tmp;
        tmp.param_name = "diffuse";
        if (func_load_material_property(surface_material,FbxSurfaceMaterial::sDiffuse, uv_set, false, tmp))
        {
            material->param_textures[tmp.param_name] = tmp;
        }
    }
    //emissive
    {
        YFbxMaterial::ParamTexture tmp;
        tmp.param_name = "emissive";
        if (func_load_material_property(surface_material,FbxSurfaceMaterial::sEmissive, uv_set, false, tmp))
        {
            material->param_textures[tmp.param_name] = tmp;
        }
    }

    //specular
    {
        YFbxMaterial::ParamTexture tmp;
        tmp.param_name = "specular";
        if (func_load_material_property(surface_material, FbxSurfaceMaterial::sSpecular, uv_set, false, tmp))
        {
            material->param_textures[tmp.param_name] = tmp;
        }
    }

    //roughness
    {
        YFbxMaterial::ParamTexture tmp;
        tmp.param_name = "roughness";
        if (func_load_material_property(surface_material, FbxSurfaceMaterial::sSpecularFactor, uv_set, false, tmp))
        {
            material->param_textures[tmp.param_name] = tmp;
        }
    }

    //metallic
    {
        YFbxMaterial::ParamTexture tmp;
        tmp.param_name = "metallic";
        if (func_load_material_property(surface_material, FbxSurfaceMaterial::sShininess, uv_set, false, tmp))
        {
            material->param_textures[tmp.param_name] = tmp;
        }
    }

    //normal
    {
        YFbxMaterial::ParamTexture tmp;
        tmp.param_name = "normal";
        if (func_load_material_property(surface_material, FbxSurfaceMaterial::sNormalMap, uv_set, true, tmp))
        {
            material->param_textures[tmp.param_name] = tmp;
        }
        else
        {
            if (func_load_material_property(surface_material, FbxSurfaceMaterial::sBump, uv_set, true, tmp))
            {
                material->param_textures[tmp.param_name] = tmp;
            }
        }
    }

    //transpaarent
    {
        YFbxMaterial::ParamTexture tmp;
        tmp.param_name = "transparent";
        if (func_load_material_property(surface_material, FbxSurfaceMaterial::sTransparentColor, uv_set, false, tmp))
        {
            material->param_textures[tmp.param_name] = tmp;
        }
    }

    //transpaarent
    {
        YFbxMaterial::ParamTexture tmp;
        tmp.param_name = "opaque_mask";
        if (func_load_material_property(surface_material, FbxSurfaceMaterial::sTransparencyFactor, uv_set, false, tmp))
        {
            material->param_textures[tmp.param_name] = tmp;
        }
    }

    return material;
}
