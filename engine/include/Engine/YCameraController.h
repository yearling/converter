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
	float forward_speed = 0.0;
	float right_speed = 0.0;
	float speed_base = 20.0;
	float rotation_speed_pitch = 200.f;
	float rotation_speed_yaw = 200.f;
	bool right_button_pressed = false;
	int last_x = 0;
	int last_y = 0;
	float delta_pitch_screen = 0;
	float delta_yaw_screen = 0;
	//SmoothStep smooth_delta_pitch;
	//SmoothStep smooth_delta_yaw;
	AverageSmooth<float> smooth_delta_pitch = { 30 };
	AverageSmooth<float> smooth_delta_yaw = { 30 };
	float wheel_forward_base = 200.f;
	float wheel_speed = 0.0;
};