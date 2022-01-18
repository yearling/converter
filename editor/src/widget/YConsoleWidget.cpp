#include "widget/YConsoleWidget.h"
#include "YEditor.h"
#include <thread>
#include <chrono>
#include "imgui.h"
#include "Math/YVector.h"
#include "Engine/YLog.h"

Widget_Console::Widget_Console(Editor* editor) :Widget(editor)
{
	m_title = "Console";

	// Create an implementation of EngineLogger
	//m_logger = make_shared<EngineLogger>();
	//m_logger->SetCallback([this](const LogPackage& package) { AddLogPackage(package); });

	// Set the logger implementation for the engine to use
	//Log::SetLogger(m_logger);

	g_verbo_log_funcs_.push_back([this](const std::string& str)
		{
			LogPackage tmp;
			tmp.text = str;
			tmp.error_level = 0;
			AddLogPackage(tmp);
		});

	g_warning_log_funcs_.push_back([this](const std::string& str)
		{
			LogPackage tmp;
			tmp.text = str;
			tmp.error_level = 1;
			AddLogPackage(tmp);
		});

	g_error_log_funcs_.push_back([this](const std::string& str)
		{
			LogPackage tmp;
			tmp.text = str;
			tmp.error_level = 2;
			AddLogPackage(tmp);
		});
	m_padding = YVector2(0.0, 0.0);
}

void Widget_Console::UpdateVisible(double delta_time)
{
	// Clear Button
	if (ImGui::Button("Clear")) { Clear(); } ImGui::SameLine();

	// Lambda for info, warning, error filter buttons
	const auto button_log_type_visibility_toggle = [this](uint32_t index)
	{
		bool& visibility = m_log_type_visibility[index];
		ImGui::PushStyleColor(ImGuiCol_Button, visibility ? ImGui::GetStyle().Colors[ImGuiCol_Button] : ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
		std::string btn_name[3] = { "info","warning","error" };
		if (ImGui::Button(btn_name[index].c_str()))
		{
			visibility = !visibility;
			m_scroll_to_bottom = true;
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::Text("%d", m_log_type_count[index]);
		ImGui::SameLine();
	};

	// Log category visibility buttons
	button_log_type_visibility_toggle(0);
	button_log_type_visibility_toggle(1);
	button_log_type_visibility_toggle(2);


	// Text filter
	const float label_width = 37.0f; //ImGui::CalcTextSize("Filter", nullptr, true).x;
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
	m_log_filter.Draw("Filter", ImGui::GetContentRegionAvail().x - label_width);
	ImGui::PopStyleVar();
	ImGui::Separator();

	// Wait for reading to finish
	while (m_is_reading) {
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	m_is_reading = true;

	// Content properties
	static const ImGuiTableFlags table_flags =
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_BordersOuter |
		ImGuiTableFlags_ScrollX |
		ImGuiTableFlags_ScrollY;
	static const ImVec2 size = ImVec2(-1.0f, -1.0f);

	// Content
	if (ImGui::BeginTable("##widget_console_content", 1, table_flags, size))
	{
		// Logs
		for (uint32_t row = 0; row < m_logs.size(); row++)
		{
			LogPackage& log = m_logs[row];

			// Text and visibility filters
			if (m_log_filter.PassFilter(log.text.c_str()) && m_log_type_visibility[log.error_level])
			{
				// Switch row
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				// Log
				ImGui::PushID(row);
				{
					//ImVec2 itemSize = ImGui::GetTextLineHeight();

					// Text (only add if visible as it's quite expensive)
					//if (ImGui::IsRectVisible(itemSize))
					{
						ImGui::PushStyleColor(ImGuiCol_Text, m_log_type_color[log.error_level]);
						ImGui::TextUnformatted(log.text.c_str());
						ImGui::PopStyleColor(1);
					}
					//else
					{
						//ImGui::Dummy(itemSize);
					}

					// Context menu
					if (ImGui::BeginPopupContextItem("##widget_console_contextMenu"))
					{
						if (ImGui::MenuItem("Copy"))
						{
							ImGui::LogToClipboard();
							ImGui::LogText("%s", log.text.c_str());
							ImGui::LogFinish();
						}

						ImGui::Separator();

						if (ImGui::MenuItem("Search"))
						{
							//FileSystem::OpenDirectoryWindow("https://www.google.com/search?q=" + log.text);
						}

						ImGui::EndPopup();
					}
				}
				ImGui::PopID();
			}
		}

		// Scroll to bottom (if requested)
		if (m_scroll_to_bottom)
		{
			ImGui::SetScrollHereY();
			m_scroll_to_bottom = false;
		}

		ImGui::EndTable();
	}

	m_is_reading = false;
}

void Widget_Console::AddLogPackage(const LogPackage& package)
{
	// Wait for reading to finish
	while (m_is_reading) { std::this_thread::sleep_for(std::chrono::milliseconds(16)); }

	// Save to deque
	m_logs.push_back(package);
	if (static_cast<uint32_t>(m_logs.size()) > m_log_max_count)
	{
		m_logs.pop_front();
	}

	// Update count
	m_log_type_count[package.error_level]++;

	// If the user is displaying this type of messages, scroll to bottom
	if (m_log_type_visibility[package.error_level])
	{
		m_scroll_to_bottom = true;
	}
}

void Widget_Console::Clear()
{
	m_logs.clear();
	m_logs.shrink_to_fit();

	m_log_type_count[0] = 0;
	m_log_type_count[1] = 0;
	m_log_type_count[2] = 0;
}

