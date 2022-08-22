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

#pragma once

//= INCLUDES ==================
#include <string>
#include <utility>
#include <vector>
#include <memory>
#include "Engine/YReferenceCount.h"
#include "SObject/STexture.h"
#include "Engine/YEngine.h"
//=============================

enum class IconType
{
    NotAssigned,
    Component_Options,
    Component_AudioListener,
    Component_AudioSource,
    Component_ReflectionProbe,
    Component_Camera,
    Component_Collider,
    Component_Light,
    Component_Material,
    Component_Material_RemoveTexture,
    Component_MeshCollider,
    Component_Renderable,
    Component_RigidBody,
    Component_SoftBody,
    Component_Script,
    Component_Terrain,
    Component_Environment,
    Component_Transform,
    Console_Info,
    Console_Warning,
    Console_Error,
    Button_Play,
    Button_Profiler,
    Button_ResourceCache,
    Directory_Folder,
    Directory_File_Audio,
    Directory_File_World,
    Directory_File_Model,
    Directory_File_Default,
    Directory_File_Material,
    Directory_File_Shader,
    Directory_File_Xml,
    Directory_File_Dll,
    Directory_File_Txt,
    Directory_File_Ini,
    Directory_File_Exe,
    Directory_File_Script,
    Directory_File_Font,
    Directory_File_Texture
};

struct Thumbnail
{
    Thumbnail() = default;
    Thumbnail(IconType type, TRefCountPtr<STexture> in_texture, const std::string& in_filePath)
    {
        this->type     = type;
        this->texture  = in_texture;
        this->file_path = in_filePath;
    }

    IconType type = IconType::NotAssigned;
    TRefCountPtr<STexture> texture;
    std::string file_path;
};

class IconProvider
{
public:
    static IconProvider& Get()
    {
        static IconProvider instance;
        return instance;
    }

    IconProvider();
    ~IconProvider();

    void Initialize(YEngine* in_engine);

    TRefCountPtr<STexture> GetTextureByType(IconType type);
    TRefCountPtr<STexture> GetTextureByFilePath(const std::string& file_path);
    //TRefCountPtr<STexture> GetTextureByThumbnail(const Thumbnail& thumbnail);

protected:
    const Thumbnail& LoadFromFile(const std::string& filePath, IconType type = IconType::NotAssigned, const uint32_t size = 100);
    const Thumbnail& GetThumbnailByType(IconType type);
    std::vector<Thumbnail> m_thumbnails;
    std::unordered_map<IconType, Thumbnail> icon_type_to_thumbnails;
    YEngine* engine_ =nullptr;
};
