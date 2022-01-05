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
	memset(key_down, 0, sizeof(key_down));
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
		g_input_manager->mouse_wheel_functions_.push_back([this](int x, int y, float z) { this->OnMouseWheel(x, y, z); });
	}
}

FPSCameraController::~FPSCameraController()
{

}

void FPSCameraController::Update(double delta_time)
{
	float forward_speed_local = 0.f;
	float right_speed_local = 0.f;
	if (key_down['W'])
	{
		forward_speed_local += speed_base;
	}
	
	if (key_down['S'])
	{
		forward_speed_local -= speed_base;
	}

	if (key_down['A'])
	{
		right_speed_local -= speed_base;
	}

	if (key_down['D'])
	{
		right_speed_local += speed_base;
	}

	float forwad_distance = (float)delta_time * forward_speed_local + (float)delta_time * wheel_speed * wheel_forward_base;
	float right_distance = (float)delta_time * right_speed_local;
	if (right_button_pressed)
	{
		YMatrix world_matrix = camera_->GetInvViewMatrix();
		YVector forward_w = world_matrix.TransformVector(YVector::forward_vector);
		YVector right_w = world_matrix.TransformVector(YVector::right_vector);
		YVector origin_w = camera_->GetPosition();
		YVector final_pos = origin_w + forward_w * forwad_distance + right_w * right_distance;
		camera_->SetPosition(final_pos);
	}

	smooth_delta_pitch.SmoothAcc(delta_pitch_screen);
	smooth_delta_yaw.SmoothAcc(delta_yaw_screen);
	float pitch_delta = smooth_delta_yaw.Average() * rotation_speed_pitch * (float)delta_time;
	float yaw_delta = smooth_delta_pitch.Average() * rotation_speed_yaw * (float)delta_time;
	YRotator final_rotator = camera_->GetRotator();
	final_rotator.pitch += pitch_delta;
	final_rotator.pitch = YMath::Clamp(-90.f, 90.0f, final_rotator.pitch);
	final_rotator.yaw += yaw_delta;
	camera_->SetRotation(final_rotator);
	wheel_speed = 0.0;
	delta_pitch_screen = 0.0;
	delta_yaw_screen = 0.0;
}

void FPSCameraController::OnKeyDown(char c)
{
	key_down[c] = true;
}

void FPSCameraController::OnKeyUp(char c)
{
	key_down[c] = false;
}

void FPSCameraController::OnRButtonDown(int x, int y)
{
	right_button_pressed = true;
	last_x = x;
	last_y = y;
#ifdef _WIN32
	ShowCursor(FALSE);
#endif
}

void FPSCameraController::OnRButtonUp(int x, int y)
{
	right_button_pressed = false;
	delta_pitch_screen = 0;
	delta_yaw_screen = 0;
#ifdef _WIN32
	ShowCursor(true);
#endif
}

void FPSCameraController::OnMouseMove(int x, int y)
{
	if (right_button_pressed)
	{
		delta_pitch_screen = (float)x - last_x;
		delta_yaw_screen = (float)y - last_y;
		last_x = x;
		last_y = y;
	}
}

void FPSCameraController::OnMouseWheel(int x, int y, float z)
{
	wheel_speed = z;
}
