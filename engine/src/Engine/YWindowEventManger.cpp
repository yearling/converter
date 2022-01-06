#include "Engine/YWindowEventManger.h"

WindowEventManager* g_windows_event_manager = nullptr;

WindowEventManager::WindowEventManager()
{

}

void WindowEventManager::OnWindowSizeChange(int x, int y)
{
	for (auto& func : windows_size_changed_funcs_)
	{
		func(x, y);
	}
}

