#include "Engine/YCameraController.h"
#include "Engine/YInputManager.h"
#include "Engine/YLog.h"
#include "math/YQuaterion.h"
CameraController::CameraController()
{

}

CameraController::~CameraController()
{

}

void CameraController::SetCamera(CameraBase* camera)
{
	camera_ = camera;
}

void CameraController::RegiesterEventProcess()
{

}

void CameraController::Update(double delta_time)
{

}

FPSCameraController::FPSCameraController()
{

}

void FPSCameraController::RegiesterEventProcess()
{
	if (g_input_manager)
	{
		g_input_manager->key_down_funcs_.push_back([this](char c) { this->OnKeyDown(c); });
		g_input_manager->key_up_funcs_.push_back([this](char c) { this->OnKeyUp(c); });
		g_input_manager->mouse_RButton_down_funcs_.push_back([this](int x, int y) { this->OnRButtonDown(x, y); });
		g_input_manager->mouse_RButton_up_funcs_.push_back([this](int x, int y) { this->OnRButtonUp(x, y); });
		g_input_manager->mouse_move_functions_.push_back([this](int x, int y) {this->OnMouseMove(x, y); });
	}
}

FPSCameraController::~FPSCameraController()
{

}

void FPSCameraController::Update(double delta_time)
{
	float forwad_distance = delta_time * forward_speed;
	float right_distance = delta_time * right_speed;

	YMatrix world_matrix = camera_->GetInvViewMatrix();
	YVector forward_w = world_matrix.TransformVector(YVector::forward_vector);
	YVector right_w = world_matrix.TransformVector(YVector::right_vector);
	YVector origin_w = camera_->GetPosition();
	YVector final_pos = origin_w + forward_w * forwad_distance + right_w * right_distance;
	camera_->SetPosition(final_pos);

	
	smooth_delta_pitch.SmoothAcc(delta_pitch);
	smooth_delta_yaw.SmoothAcc(delta_yaw);

	float pitch_delta = smooth_delta_yaw.Result() * rotation_speed_pitch* delta_time;
	float yaw_delta = smooth_delta_pitch.Result() * rotation_speed_yaw * delta_time;
	YRotator final_rotator = camera_->GetRotator();
	final_rotator.pitch += pitch_delta;
	final_rotator.pitch = YMath::Clamp(-90.f, 90.0f, final_rotator.pitch);
	final_rotator.yaw += yaw_delta;
	camera_->SetRotation(final_rotator);

	//LOG_INFO(" smooth deltx_x: ", smooth_delta_pitch.Result(), " delta y: ", smooth_delta_yaw.Result());
}

void FPSCameraController::OnKeyDown(char c)
{
	if (!right_button_pressed)
	{
		return;
	}
	//LOG_INFO("key down: ", c);
	if (c == 'W')
	{
		forward_speed = speed_base;
	}
	else if (c == 'S')
	{
		forward_speed = -speed_base;
	}
	else if (c == 'A')
	{
		right_speed = -speed_base;
	}
	else if (c == 'D')
	{
		right_speed = speed_base;
	}
}

void FPSCameraController::OnKeyUp(char c)
{
	//LOG_INFO("key up: ", c);
	if (c == 'W')
	{
		forward_speed = 0.f;
	}
	else if (c == 'S')
	{
		forward_speed = 0.f;
	}
	else if (c == 'A')
	{
		right_speed = 0;
	}
	else if (c == 'D')
	{
		right_speed = 0;
	}
}

void FPSCameraController::OnRButtonDown(int x, int y)
{
	//LOG_INFO("R mouse button down: ", x, " ", y);
	right_button_pressed = true;
	last_x = x;
	last_y = y;
	ShowCursor(false);
}

void FPSCameraController::OnRButtonUp(int x, int y)
{
	//LOG_INFO("R mouse button up: ", x, " ", y);
	right_button_pressed = false;
	delta_pitch = 0;
	delta_yaw = 0;
	ShowCursor(true);
}

void FPSCameraController::OnMouseMove(int x, int y)
{

	if (right_button_pressed)
	{
		//LOG_INFO("R mouse move: ", x, " ", y);
		delta_pitch = x - last_x;
		delta_yaw = y - last_y;
		last_x = x;
		last_y = y;
	}
}
