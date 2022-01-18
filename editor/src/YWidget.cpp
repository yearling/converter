#include "YWidget.h"
#include "YEditor.h"
#include "imgui_internal.h"

Widget::Widget(Editor* editor)
	:m_editor(editor), m_window(nullptr)
{

}

void Widget::Update(double delta_time)
{
	UpdateAlways(delta_time);
	if (!m_is_window)
	{
		return;
	}

	//ImGui::Begin()
	bool begin = false;
	{
		if (!m_is_visible)
			return;

		//Size
		if (m_size_initial != YVector2(k_widget_default_propery, k_widget_default_propery))
		{
			ImGui::SetNextWindowSize(m_size_initial, ImGuiCond_FirstUseEver);
		}

		// max size
		if (m_size_min != YVector2(k_widget_default_propery, k_widget_default_propery) || m_size_max != YVector2(k_widget_default_propery, k_widget_default_propery))
		{
			ImGui::SetNextWindowSizeConstraints(m_size_min, m_size_max);
		}

		// Padding
		if (m_padding != YVector2(k_widget_default_propery, k_widget_default_propery))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, m_padding);
			m_var_pushes++;
		}

		//Alpha
		if (m_alpha != k_widget_default_propery)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
			m_var_pushes++;
		}

		//position
		if (m_position != YVector2(k_widget_default_propery, k_widget_default_propery))
		{
			/*	if (m_position == YVector2(k_widget_position_screen_center, k_widget_position_screen_center))
				{
					m_position.x =
				}*/
			ImVec2 pivot_center = ImVec2(0.5f, 0.5f);
			ImGui::SetNextWindowPos(m_position, ImGuiCond_FirstUseEver, pivot_center);
		}
		// Callback
		OnPushStyleVar();

		//Begin
		if (ImGui::Begin(m_title.c_str(), &m_is_visible, m_flags))
		{
			m_window = ImGui::GetCurrentWindow();
			m_height = ImGui::GetWindowHeight();
			begin = true;
		}
		else if (m_window && m_window->Hidden)
		{
			// Enters here if the window is hidden as part of an unselected tab.
					   // ImGui::Begin() makes the window but returns false, then ImGui still expects ImGui::End() to be called.
					   // So we make sure that when Widget::End() is called, ImGui::End() get's called as well.
					   // Note: ImGui's docking is in beta, so maybe it's at fault here ?
			begin = true;
		}

		//Callbacks
		if (m_window && m_window->Appearing)
		{
			OnShow();
		}
		else if (!m_is_visible)
		{
			OnHide();
		}

		if (begin)
		{
			UpdateVisible(delta_time);
			{
				// End
				ImGui::End();

				// Pop style variables
				ImGui::PopStyleVar(m_var_pushes);
				m_var_pushes = 0;

			}
		}
	}


}

void Widget::UpdateAlways(double delta_time)
{

}

void Widget::UpdateVisible(double delta_time)
{

}

void Widget::OnShow()
{

}

void Widget::OnHide()
{

}

void Widget::OnPushStyleVar()
{

}

