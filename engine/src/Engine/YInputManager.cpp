#include "Engine/YInputManager.h"

InputManger* g_input_manager = nullptr;

InputManger::InputManger()
{
	relative_to_viewport = YVector2(0.0, 0.0);
}

void InputManger::OnEventRButtonDown(int x, int y)
{
	x -= relative_to_viewport.x;
	y -= relative_to_viewport.y;
	for (auto& func : mouse_RButton_down_funcs_)
	{
		func(x, y);
	}
}

void InputManger::OnEventRButtonUp(int x, int y)
{
	x -= relative_to_viewport.x;
	y -= relative_to_viewport.y;
	for (auto& func : mouse_RButton_up_funcs_)
	{
		func(x, y);
	}
}

void InputManger::OnEventKeyDown(char c)
{
	for (auto& func : key_down_funcs_)
	{
		func(c);
	}
}

void InputManger::OnEventKeyUp(char c)
{
	for (auto& func : key_up_funcs_)
	{
		func(c);
	}
}

void InputManger::OnMouseMove(int x, int y)
{
	x -= relative_to_viewport.x;
	y -= relative_to_viewport.y;
	for (auto& func : mouse_move_functions_)
	{
		func(x, y);
	}
}

void InputManger::OnMouseWheel(int x, int y, float z_delta)
{
	x -= relative_to_viewport.x;
	y -= relative_to_viewport.y;
	for (auto& func : mouse_wheel_functions_)
	{
		func(x, y, z_delta);
	}
}

void InputManger::OnEventLButtonDown(int x, int y)
{
	x -= relative_to_viewport.x;
	y -= relative_to_viewport.y;
	for (auto& func : mouse_LButton_down_funcs_)
	{
		func(x, y);
	}
}

void InputManger::SetEditorViewportOffset(YVector2 offset)
{
	relative_to_viewport = offset;
}

