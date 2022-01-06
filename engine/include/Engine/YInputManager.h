#pragma once
#include <vector>
#include <functional>

class InputManger
{
public:
	InputManger();
	std::vector<std::function<void(int x, int y)>> mouse_LButton_down_funcs_;
	std::vector<std::function<void(int x, int y)>> mouse_LButton_double_click_funcs_;
	std::vector<std::function<void(int x, int y)>> mouse_LButton_up_funcs_;
	std::vector<std::function<void(int x, int y)>> mouse_MButton_down_funcs_;
	std::vector<std::function<void(int x, int y)>> mouse_MButton_double_click_funcs_;
	std::vector<std::function<void(int x, int y)>> mouse_MButton_up_funcs_;
	std::vector<std::function<void(int x, int y)>> mouse_RButton_down_funcs_;
	std::vector<std::function<void(int x, int y)>> mouse_RButton_double_click_funcs_;
	std::vector<std::function<void(int x, int y)>> mouse_RButton_up_funcs_;
	std::vector<std::function<void(int x, int y, int value)>> mouse_wheel_funcs_;
	std::vector<std::function<void(char c)>> key_down_funcs_;
	std::vector<std::function<void(char c)>> key_up_funcs_;
	std::vector < std::function<void(int x, int y)>> mouse_move_functions_;
	std::vector < std::function<void(int x, int y, float z_delta)>> mouse_wheel_functions_;
	void OnEventRButtonDown(int x, int y);
	void OnEventRButtonUp(int x, int y);
	void OnEventKeyDown(char c);
	void OnEventKeyUp(char c);
	void OnMouseMove(int x, int y);
	void OnMouseWheel(int x, int y, float z_delta);
	void OnEventLButtonDown(int x, int y);
};

extern InputManger* g_input_manager;