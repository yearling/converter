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
	memset(key_down_, 0, sizeof(key_down_));
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
	if (key_down_['W'])
	{
		forward_speed_local += speed_base_;
	}

	if (key_down_['S'])
	{
		forward_speed_local -= speed_base_;
	}

	if (key_down_['A'])
	{
		right_speed_local -= speed_base_;
	}

	if (key_down_['D'])
	{
		right_speed_local += speed_base_;
	}

	float forwad_distance = (float)delta_time * forward_speed_local + (float)delta_time * wheel_speed_ * wheel_forward_base_;
	float right_distance = (float)delta_time * right_speed_local;
	if (right_button_pressed_)
	{
		YMatrix world_matrix = camera_->GetInvViewMatrix();
		YVector forward_w = world_matrix.TransformVector(YVector::forward_vector);
		YVector right_w = world_matrix.TransformVector(YVector::right_vector);
		YVector origin_w = camera_->GetPosition();
		YVector final_pos = origin_w + forward_w * forwad_distance + right_w * right_distance;
		camera_->SetPosition(final_pos);
	}

	smooth_delta_pitch_.SmoothAcc(delta_pitch_screen_);
	smooth_delta_yaw_.SmoothAcc(delta_yaw_screen_);
	float pitch_delta = delta_yaw_screen_ * rotation_speed_pitch_;
	float yaw_delta = delta_pitch_screen_ * rotation_speed_yaw_;
	YRotator final_rotator = camera_->GetRotator();
	final_rotator.pitch += pitch_delta;
	final_rotator.pitch = YMath::Clamp(-90.f, 90.0f, final_rotator.pitch);
	final_rotator.yaw += yaw_delta;
	camera_->SetRotation(final_rotator);
	wheel_speed_ = 0.0;
	delta_pitch_screen_ = 0.0;
	delta_yaw_screen_ = 0.0;
}

void FPSCameraController::OnKeyDown(char c)
{
	key_down_[c] = true;
}

void FPSCameraController::OnKeyUp(char c)
{
	key_down_[c] = false;
}

void FPSCameraController::OnRButtonDown(int x, int y)
{
	right_button_pressed_ = true;
	last_x_ = x;
	last_y_ = y;
#ifdef _WIN32
	ShowCursor(FALSE);
#endif
}

void FPSCameraController::OnRButtonUp(int x, int y)
{
	right_button_pressed_ = false;
	delta_pitch_screen_ = 0;
	delta_yaw_screen_ = 0;
#ifdef _WIN32
	ShowCursor(true);
#endif
}

void FPSCameraController::OnMouseMove(int x, int y)
{
	if (right_button_pressed_)
	{
		delta_pitch_screen_ = (float)x - last_x_;
		delta_yaw_screen_ = (float)y - last_y_;
		last_x_ = x;
		last_y_ = y;
	}
}

void FPSCameraController::OnMouseWheel(int x, int y, float z)
{
	wheel_speed_ = z;
}
