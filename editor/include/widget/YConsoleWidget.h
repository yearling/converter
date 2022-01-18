#pragma once

#include "YWidget.h"
#include <memory>
#include <functional>
#include <deque>
#include <atomic>

struct LogPackage
{
	std::string text;
	unsigned int error_level = 0;
};

// Implementation of Spartan::ILogger so the engine can log into the editor
class EngineLogger
{
public:
	typedef std::function<void(LogPackage)> log_func;
	void SetCallback(log_func&& func)
	{
		m_log_func = std::forward<log_func>(func);
	}

	void Log(const std::string& text, const unsigned int error_level) 
	{
		LogPackage package;
		package.text = text;
		package.error_level = error_level;
		m_log_func(package);
	}

private:
	log_func m_log_func;
};

class Widget_Console : public Widget
{
public:
	Widget_Console(Editor* editor);

	void UpdateVisible(double delta_time) override;
	void AddLogPackage(const LogPackage& package);
	void Clear();

private:
	bool m_scroll_to_bottom = false;
	uint32_t m_log_max_count = 1000;
	bool m_log_type_visibility[3] = { true, true, true };
	uint32_t m_log_type_count[3] = { 0, 0, 0 };
	const std::vector<YVector4> m_log_type_color =
	{
		YVector4(0.76f, 0.77f, 0.8f, 1.0f),    // Info
		YVector4(0.7f, 0.75f, 0.0f, 1.0f),    // Warning
		YVector4(0.7f, 0.3f, 0.3f, 1.0f)        // Error
	};
	std::atomic<bool> m_is_reading = false;
	std::shared_ptr<EngineLogger> m_logger;
	std::deque<LogPackage> m_logs;
	ImGuiTextFilter m_log_filter;
};