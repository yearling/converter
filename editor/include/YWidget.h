#pragma once
#include <string>
#include <functional>
#include "imgui.h"
#include "Math/YVector.h"
class Editor;
struct ImGuiWindow;
const float k_widget_default_propery = -1.0f;
const float k_widget_position_screen_center = -2.0f;
class Widget
{
public:
	Widget(Editor* editor);
	virtual ~Widget() = default;

	virtual void Update(double delta_time);
	virtual void UpdateAlways(double delta_time); // Called always
	virtual void UpdateVisible(double delta_time); // Called only when the widget is visible
	virtual void OnShow(); // Called when the window becomes visible
	virtual void OnHide(); // Called when the window becomes invisible
	virtual void OnPushStyleVar();  // Called just before ImGui::Begin()

   // Use this to push style variables. They will be automatically popped.
	template<typename T>
	void PushStyleVar(ImGuiStyleVar idx, T val) { ImGui::PushStyleVar(idx, val); m_var_pushes++; }

	// Properties
	float GetHeight()           const { return m_height; }
	ImGuiWindow* GetWindow()    const { return m_window; }
	const auto& GetTitle()      const { return m_title; }
	bool& GetVisible() { return m_is_visible; }
	void SetVisible(bool is_visible) { m_is_visible = is_visible; }

protected:
	bool m_is_window = true;
	bool m_is_visible = true;
	int m_flags = ImGuiWindowFlags_NoCollapse;
	float m_height = 0;
	float m_alpha = -1.0f;
	YVector2 m_position = { k_widget_default_propery,k_widget_default_propery };
	YVector2 m_size = { k_widget_default_propery,k_widget_default_propery };
	YVector2 m_size_min = { k_widget_default_propery,k_widget_default_propery };
	YVector2 m_size_max = { k_widget_default_propery,k_widget_default_propery };
	YVector2 m_padding = { k_widget_default_propery,k_widget_default_propery };
	Editor* m_editor = nullptr;
	//Spartan::Context* m_context = nullptr;
	//Spartan::Profiler* m_profiler = nullptr;
	ImGuiWindow* m_window = nullptr;
	std::string m_title = "Title";


private:
	uint8_t m_var_pushes = 0;
};