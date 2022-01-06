#pragma once
#include <vector>
#include <functional>
class WindowEventManager
{
public:
	WindowEventManager();
	std::vector<std::function<void(int x, int y)>> windows_size_changed_funcs_;
	void OnWindowSizeChange(int x, int y);
};
extern WindowEventManager* g_windows_event_manager;