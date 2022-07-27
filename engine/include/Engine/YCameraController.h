#pragma once
#include "Engine/YCamera.h"
#include <algorithm>
#include <numeric>
#include <deque>
#include "Utility/YAverageSmooth.h"
class CameraController
{
public:
	CameraController();
	virtual ~CameraController();
	void SetCamera(CameraBase* camera);
	virtual void RegiesterEventProcess();
	virtual void Update(double delta_time);
protected:
	CameraBase* camera_;
};


class FPSCameraController :public CameraController
{
public:
	FPSCameraController();
	void RegiesterEventProcess()override;
	virtual ~FPSCameraController();
	virtual void Update(double delta_time);

protected:
	void OnKeyDown(char c);
	void OnKeyUp(char c);
	void OnRButtonDown(int x, int y);
	void OnRButtonUp(int x, int y);
	void OnMouseMove(int x, int y);
	void OnMouseWheel(int x, int y, float z);
	float speed_base_ = 20.0;
	float rotation_speed_pitch_ = 200.f;
	float rotation_speed_yaw_ = 200.f;
	bool right_button_pressed_ = false;
	int last_x_ = 0;
	int last_y_ = 0;
	float delta_pitch_screen_ = 0;
	float delta_yaw_screen_ = 0;
	AverageSmooth<float> smooth_delta_pitch_ = { 5 };
	AverageSmooth<float> smooth_delta_yaw_ = { 5 };
	float wheel_forward_base_ = 800.f;
	float wheel_speed_ = 0.0;
	bool key_down_[255];
};