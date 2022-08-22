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

//= INCLUDES =====================================
#include <string>
#include <variant>
#include <chrono>
#include "WidgetUtility/IconProvider.h"
#include "imgui.h"
#include "Math/YVector.h"
//================================================

//class EditorHelper
//{
//public:
//
//    static EditorHelper& Get()
//    {
//        static EditorHelper instance;
//        return instance;
//    }
//
//    void Initialize(Spartan::Context* context)
//    {
//        g_context        = context;
//        g_resource_cache = context->GetSubsystem<Spartan::ResourceCache>();
//        g_world          = context->GetSubsystem<Spartan::World>();
//        g_threading      = context->GetSubsystem<Spartan::Threading>();
//        g_renderer       = context->GetSubsystem<Spartan::Renderer>();
//        g_input          = context->GetSubsystem<Spartan::Input>();
//    }
//
//    void LoadModel(const std::string& file_path) const
//    {
//        auto resource_cache = g_resource_cache;
//
//        // Load the model asynchronously
//        g_threading->AddTask([resource_cache, file_path]()
//        {
//            resource_cache->Load<Spartan::Model>(file_path);
//        });
//    }
//
//    void LoadWorld(const std::string& file_path) const
//    {
//        auto world = g_world;
//
//        // Loading a world resets everything so it's important to ensure that no tasks are running
//        g_threading->Flush(true);
//
//        // Load the scene asynchronously
//        g_threading->AddTask([world, file_path]()
//        {
//            world->LoadFromFile(file_path);
//        });
//    }
//
//    void SaveWorld(const std::string& file_path) const
//    {
//        auto world = g_world;
//
//        // Save the scene asynchronously
//        g_threading->AddTask([world, file_path]()
//        {
//            world->SaveToFile(file_path);
//        });
//    }
//
//    void PickEntity()
//    {
//        // If the transform handle hasn't finished editing don't do anything.
//        if (g_world->GetTransformHandle()->IsEditing())
//            return;
//
//        // Get camera
//        const auto& camera = g_renderer->GetCamera();
//        if (!camera)
//            return;
//
//        // Pick the world
//        std::shared_ptr<Spartan::Entity> entity;
//        camera->Pick(entity);
//
//        // Set the transform handle to the selected entity
//        SetSelectedEntity(entity);
//
//        // Fire callback
//        g_on_entity_selected();
//    }
//
//    void SetSelectedEntity(const std::shared_ptr<Spartan::Entity>& entity)
//    {
//        // keep returned entity instead as the transform handle can decide to reject it
//        g_selected_entity = g_world->GetTransformHandle()->SetSelectedEntity(entity);
//    }
//
//    Spartan::Context*              g_context            = nullptr;
//    Spartan::ResourceCache*        g_resource_cache     = nullptr;
//    Spartan::World*                g_world              = nullptr;
//    Spartan::Threading*            g_threading          = nullptr;
//    Spartan::Renderer*             g_renderer           = nullptr;
//    Spartan::Input*                g_input              = nullptr;
//    std::weak_ptr<Spartan::Entity> g_selected_entity;   
//    std::function<void()>          g_on_entity_selected = nullptr;
//};

namespace ImGuiEx
{
    static const ImVec4 default_tint(255, 255, 255, 255);

    inline float GetWindowContentRegionWidth()
    {
        return ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
    }

    // Collapsing header
    inline bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        bool result = ImGui::CollapsingHeader(label, flags);
        ImGui::PopStyleVar();
        return result;
    }

    // Button
    inline bool Button(const char* label, const YVector2& size = YVector2(0, 0))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        bool result = ImGui::Button(label, size);
        ImGui::PopStyleVar();
        return result;
    }

    // Images & Image buttons
    inline bool ImageButton(TRefCountPtr<STexture> texture, const ImVec2& size)
    {
        return ImGui::ImageButton
        (
            static_cast<ImTextureID>(texture->texture_2d_->srv_),
            size,
            ImVec2(0, 0),        // uv0
            ImVec2(1, 1),        // uv1
            -1,                  // frame padding
            ImColor(0, 0, 0, 0), // background
            default_tint         // tint
        );
    }

    inline bool ImageButton(const IconType icon, const float size, bool border = false)
    {
        if (!border)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        }

        bool result = ImGui::ImageButton(
            static_cast<ImTextureID>(IconProvider::Get().GetTextureByType(icon)->texture_2d_->srv_),
            ImVec2(size, size),
            ImVec2(0, 0),           // uv0
            ImVec2(1, 1),           // uv1
            -1,                     // frame padding
            ImColor(0, 0, 0, 0),    // background
            default_tint            // tint
        );

        if (!border)
        {
            ImGui::PopStyleVar();
        }

        return result;
    }

    inline bool ImageButton(const char* id, const IconType icon, const float size, bool border = false)
    {
        if (!border)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        }

        ImGui::PushID(id);
        const auto pressed = ImGui::ImageButton(
            static_cast<ImTextureID>(IconProvider::Get().GetTextureByType(icon)->texture_2d_->srv_),
            ImVec2(size, size),
            ImVec2(0, 0),           // uv0
            ImVec2(1, 1),           // uv1
            -1,                     // frame padding
            ImColor(0, 0, 0, 0),    // background
            default_tint            // tint
        );
        ImGui::PopID();

        if (!border)
        {
            ImGui::PopStyleVar();
        }

        return pressed;
    }

    inline void Image(const Thumbnail& thumbnail, const float size)
    {
        ImGui::Image(
            static_cast<ImTextureID>(thumbnail.texture->texture_2d_->srv_),
            ImVec2(size, size),
            ImVec2(0, 0),
            ImVec2(1, 1),
            default_tint,       // tint
            ImColor(0, 0, 0, 0) // border
        );
    }

    inline void Image(TRefCountPtr<STexture> texture, const YVector2& size, bool border = false)
    {
        if (!border)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        }

        ImGui::Image(
            static_cast<ImTextureID>(texture),
            size,
            ImVec2(0, 0),
            ImVec2(1, 1),
            default_tint,       // tint
            ImColor(0, 0, 0, 0) // border
        );

        if (!border)
        {
            ImGui::PopStyleVar();
        }
    }

    inline void Image(TRefCountPtr<STexture> texture , const ImVec2& size, const ImColor& tint = default_tint, const ImColor& border = ImColor(0, 0, 0, 0))
    {
        ImGui::Image(
            static_cast<ImTextureID>(texture->texture_2d_->srv_),
            size,
            ImVec2(0, 0),
            ImVec2(1, 1),
            tint,
            border
        );
    }

    inline void Image(const IconType icon, const float size)
    {
        ImGui::Image(
            static_cast<void*>(IconProvider::Get().GetTextureByType(icon)),
            ImVec2(size, size),
            ImVec2(0, 0),
            ImVec2(1, 1),
            default_tint,       // tint
            ImColor(0, 0, 0, 0) // border
        );
    }

    // Drag & Drop
    enum class DragPayloadType
    {
        DragPayload_Unknown,
        DragPayload_Texture,
        DragPayload_Entity,
        DragPayload_Model,
        DragPayload_Audio,
        DragPayload_Script,
        DragPayload_Material
    };

    struct DragDropPayload
    {
        typedef std::variant<const char*, uint64_t> dataVariant;
        DragDropPayload(const DragPayloadType type = DragPayloadType::DragPayload_Unknown, const dataVariant data = nullptr)
        {
            this->type = type;
            this->data = data;
        }
        DragPayloadType type;
        dataVariant data;
    };

    inline void CreateDragPayload(const DragDropPayload& payload)
    {
        ImGui::SetDragDropPayload(reinterpret_cast<const char*>(&payload.type), reinterpret_cast<const void*>(&payload), sizeof(payload), ImGuiCond_Once);
    }

    inline DragDropPayload* ReceiveDragPayload(DragPayloadType type)
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const auto payload_imgui = ImGui::AcceptDragDropPayload(reinterpret_cast<const char*>(&type)))
            {
                return static_cast<DragDropPayload*>(payload_imgui->Data);
            }
            ImGui::EndDragDropTarget();
        }

        return nullptr;
    }

    // Image slot
    inline void ImageSlot(TRefCountPtr<STexture>  image, const std::function<void(TRefCountPtr<STexture>)> setter)
    {
        const ImVec2 slot_size  = ImVec2(80, 80);
        const float button_size = 15.0f;

        // Image
        ImGui::BeginGroup();
        {
            //Spartan::RHI_Texture* texture = image.get();
            const ImVec2 pos_image        = ImGui::GetCursorPos();
            const ImVec2 pos_button       = ImVec2(ImGui::GetCursorPosX() + slot_size.x - button_size * 2.0f + 6.0f, ImGui::GetCursorPosY() + 1.0f);

            // Remove button
            if (image != nullptr)
            {
                ImGui::SetCursorPos(pos_button);
                ImGui::PushID(static_cast<int>(pos_button.x + pos_button.y));
                if (ImGuiEx::ImageButton("", IconType::Component_Material_RemoveTexture, button_size, true))
                {
                    image = nullptr;
                    setter(nullptr);
                }
                ImGui::PopID();
            }

            // Image
            ImGui::SetCursorPos(pos_image);
            ImGuiEx::Image
            (
                image,
                slot_size,
                ImColor(255, 255, 255, 255),
                ImColor(255, 255, 255, 128)
            );

            // Remove button - Does nothing, drawn again just to be visible
            if (image != nullptr)
            {
                ImGui::SetCursorPos(pos_button);
                ImGuiEx::ImageButton("", IconType::Component_Material_RemoveTexture, button_size, true);
            }
        }
        ImGui::EndGroup();

        // Drop target
      /*  if (auto payload = ImGuiEx::ReceiveDragPayload(ImGuiEx::DragPayloadType::DragPayload_Texture))
        {
            try
            {
                if (const auto tex = EditorHelper::Get().g_resource_cache->Load<Spartan::RHI_Texture2D>(std::get<const char*>(payload->data)))
                {
                    setter(tex);
                }
            }
            catch (const std::bad_variant_access& e) { LOG_ERROR("%s", e.what()); }
        }*/
    }

    inline void Tooltip(const char* text)
    {
        if (!text)
            return;

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text(text);
            ImGui::EndTooltip();
        }
    }

    // A drag float which will wrap the mouse cursor around the edges of the screen
    inline void DragFloatWrap(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", const ImGuiSliderFlags flags = 0)
    {
        // Drag
        ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, flags);
#if 0
        // Wrap
        if (ImGui::IsItemEdited() && ImGui::IsMouseDown(0))
        {
            YVector2 pos  = EditorHelper::Get().g_input->GetMousePosition();
            uint32_t edge_padding       = 5;

            bool wrapped = false;
            if (pos.x >= Spartan::Display::GetWidth() - edge_padding)
            {
                pos.x = static_cast<float>(edge_padding + 1);
                wrapped = true;
            }
            else if (pos.x <= edge_padding)
            {
                pos.x = static_cast<float>(Spartan::Display::GetWidth() - edge_padding - 1);
                wrapped = true;
            }

            if (wrapped)
            {
                ImGuiIO& imgui_io           = ImGui::GetIO();
                imgui_io.MousePos           = pos;
                imgui_io.MousePosPrev       = pos; // set previous position as well so that we eliminate a huge mouse delta, which we don't want for the drag float
                imgui_io.WantSetMousePos    = true;
            }
        }
#endif
    }

    inline bool ComboBox(const char* label, const std::vector<std::string>& options, uint32_t* selection_index)
    {
        // Clamp the selection index in case it's larger than the actual option count.
        const uint32_t option_count = static_cast<uint32_t>(options.size());
        if (*selection_index >= option_count)
        {
            *selection_index = option_count - 1;
        }

        bool selection_made          = false;
        std::string selection_string = options[*selection_index];

        if (ImGui::BeginCombo(label, selection_string.c_str()))
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(options.size()); i++)
            {
                const bool is_selected = *selection_index == i;

                if (ImGui::Selectable(options[i].c_str(), is_selected))
                {
                    *selection_index    = i;
                    selection_made      = true;
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        return selection_made;
    }
}
