/*
Copyright(c) 2016-2022 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES =================
#include "WidgetUtility/IconProvider.h"
#include "Utility/YPath.h"
#include "SObject/SObjectManager.h"
//============================

//= NAMESPACES ==========
using namespace std;
//=======================

static Thumbnail g_no_thumbnail;

IconProvider::IconProvider()
{
}

IconProvider::~IconProvider()
{
    m_thumbnails.clear();
}

void IconProvider::Initialize(YEngine* in_engine)
{
    engine_ = in_engine;
    //const string data_dir = m_context->GetSubsystem<ResourceCache>()->GetResourceDirectory() + "/";
    const string data_dir = YPath::GetEngineResourceDataDirPath()+'/';

    // Load standard icons
    LoadFromFile(data_dir + "icons/component_componentOptions.png",       IconType::Component_Options);
    LoadFromFile(data_dir + "icons/component_audioListener.png",          IconType::Component_AudioListener);
    LoadFromFile(data_dir + "icons/component_audioSource.png",            IconType::Component_AudioSource);
    LoadFromFile(data_dir + "icons/component_reflectionProbe.png",        IconType::Component_ReflectionProbe);
    LoadFromFile(data_dir + "icons/component_camera.png",                 IconType::Component_Camera); 
    LoadFromFile(data_dir + "icons/component_collider.png",               IconType::Component_Collider);
    LoadFromFile(data_dir + "icons/component_light.png",                  IconType::Component_Light);
    LoadFromFile(data_dir + "icons/component_material.png",               IconType::Component_Material);
    LoadFromFile(data_dir + "icons/component_material_removeTexture.png", IconType::Component_Material_RemoveTexture);
    LoadFromFile(data_dir + "icons/component_meshCollider.png",           IconType::Component_MeshCollider);
    LoadFromFile(data_dir + "icons/component_renderable.png",             IconType::Component_Renderable);
    LoadFromFile(data_dir + "icons/component_rigidBody.png",              IconType::Component_RigidBody);
    LoadFromFile(data_dir + "icons/component_softBody.png",               IconType::Component_SoftBody);
    LoadFromFile(data_dir + "icons/component_script.png",                 IconType::Component_Script);
    LoadFromFile(data_dir + "icons/component_transform.png",              IconType::Component_Transform);
    LoadFromFile(data_dir + "icons/component_terrain.png",                IconType::Component_Terrain);
    LoadFromFile(data_dir + "icons/component_environment.png",            IconType::Component_Environment);
    LoadFromFile(data_dir + "icons/console_info.png",                     IconType::Console_Info);
    //LoadFromFile(data_dir + "icons/uv.png",                               IconType::Console_Info);
    LoadFromFile(data_dir + "icons/console_warning.png",                  IconType::Console_Warning);
    LoadFromFile(data_dir + "icons/console_error.png",                    IconType::Console_Error);
    LoadFromFile(data_dir + "icons/button_play.png",                      IconType::Button_Play);
    LoadFromFile(data_dir + "icons/profiler.png",                         IconType::Button_Profiler);
    LoadFromFile(data_dir + "icons/resource_cache.png",                   IconType::Button_ResourceCache);
    LoadFromFile(data_dir + "icons/file.png",                             IconType::Directory_File_Default);
    LoadFromFile(data_dir + "icons/folder.png",                           IconType::Directory_Folder);
    LoadFromFile(data_dir + "icons/audio.png",                            IconType::Directory_File_Audio);
    LoadFromFile(data_dir + "icons/model.png",                            IconType::Directory_File_Model);
    LoadFromFile(data_dir + "icons/world.png",                            IconType::Directory_File_World);
    LoadFromFile(data_dir + "icons/material.png",                         IconType::Directory_File_Material);
    LoadFromFile(data_dir + "icons/shader.png",                           IconType::Directory_File_Shader);
    LoadFromFile(data_dir + "icons/xml.png",                              IconType::Directory_File_Xml);
    LoadFromFile(data_dir + "icons/dll.png",                              IconType::Directory_File_Dll);
    LoadFromFile(data_dir + "icons/txt.png",                              IconType::Directory_File_Txt);
    LoadFromFile(data_dir + "icons/ini.png",                              IconType::Directory_File_Ini);
    LoadFromFile(data_dir + "icons/exe.png",                              IconType::Directory_File_Exe);
    LoadFromFile(data_dir + "icons/script.png",                           IconType::Directory_File_Script);
    LoadFromFile(data_dir + "icons/font.png",                             IconType::Directory_File_Font);
    LoadFromFile(data_dir + "icons/texture.png",                          IconType::Directory_File_Texture);
    g_no_thumbnail.file_path = "default";
    g_no_thumbnail.texture = STexture::GenerateDefaultTexture();
    g_no_thumbnail.type = IconType::NotAssigned;
    icon_type_to_thumbnails[IconType::NotAssigned] = g_no_thumbnail;
}

TRefCountPtr<STexture> IconProvider::GetTextureByType(IconType type)
{
    //return LoadFromFile("", type).texture.get();
   return GetThumbnailByType(type).texture;
}

TRefCountPtr<STexture> IconProvider::GetTextureByFilePath(const string& filePath)
{
    IconType type = IconType::NotAssigned;
    std::string file_extension = YPath::GetFileExtension(filePath);
    file_extension = YPath::PathTolowerCharactor(file_extension);
    if (file_extension == "png" || file_extension == "jpg" || file_extension == "jpeg" || file_extension == "dds")
    {
        type = IconType::Directory_File_Texture;
    }
    else if (file_extension == ".json")
    {
        type = IconType::Directory_File_Txt;
    }
    else if (file_extension == ".yasset")
    {
        type = IconType::Directory_File_Script;
    }
    else
    {
        type = IconType::Directory_File_World;
    }
    return GetTextureByType(type);
    //return LoadFromFile(filePath).texture.get();
}

//TRefCountPtr<STexture> IconProvider::GetTextureByThumbnail(const Thumbnail& thumbnail)
//{
//    for (const auto& thumbnailTemp : m_thumbnails)
//    {
//        if (thumbnailTemp.texture->IsLoading())
//            continue;
//
//        if (thumbnailTemp.texture->GetObjectId() == thumbnail.texture->GetObjectId())
//        {
//            return thumbnailTemp.texture.get();
//        }
//    }
//
//    return nullptr;
//}

const Thumbnail& IconProvider::LoadFromFile(const string& file_path, IconType type /*NotAssigned*/, const uint32_t size /*100*/)
{
    // Check if we already have this thumbnail
    if (icon_type_to_thumbnails.find(type) == icon_type_to_thumbnails.end())
    {
        icon_type_to_thumbnails[type].file_path = file_path;
        icon_type_to_thumbnails[type].type = type;
        TRefCountPtr<STexture> texture = SObjectManager::ConstructFromPackage<STexture>(file_path, nullptr);
        if (!texture)
        {
            texture = STexture::GenerateDefaultTexture();
        }
        texture->UploadGPUBuffer();
        icon_type_to_thumbnails[type].texture = texture;
    }
    return icon_type_to_thumbnails[type];
}

const Thumbnail& IconProvider::GetThumbnailByType(IconType type)
{
    if (icon_type_to_thumbnails.count(type))
    {
        return icon_type_to_thumbnails[type];
    }
    return g_no_thumbnail;
}
