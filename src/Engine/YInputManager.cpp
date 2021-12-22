#include "Engine/YInputManager.h"

InputManger* g_input_manager = nullptr;

InputManger::InputManger()
{

}

void InputManger::OnEventRButtonDown(int x, int y)
{
	for (auto& func : mouse_RButton_down_funcs_)
	{
		func(x, y);
	}
}

void InputManger::OnEventRButtonUp(int x, int y)
{
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
	for (auto& func : mouse_move_functions_)
	{
		func(x, y);
	}
}

