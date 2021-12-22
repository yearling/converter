#pragma once
#include "Engine/YCamera.h"
#include <algorithm>
#include <numeric>
#include <deque>
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


class FPSCameraController:public CameraController
{
public:
	FPSCameraController();
	void RegiesterEventProcess()override;
	virtual ~FPSCameraController();
	virtual void Update(double delta_time);
		
	struct SmoothStep
	{
	public:
		float Result()
		{
			float acc = std::accumulate(sequence.begin(), sequence.end(), 0.f);
			if (sequence.empty())
			{
				return 0.f;
			}
			return acc / (float) sequence.size();
		}
		void SmoothAcc(float v)
		{
			while(sequence.size() > smooth_frame_count)
			{
				sequence.pop_front();
			}
			sequence.push_back(v);
		}
		std::deque<float> sequence;
		int smooth_frame_count = 50;
	};
protected:
	void OnKeyDown(char c);
	void OnKeyUp(char c);
	void OnRButtonDown(int x, int y);
	void OnRButtonUp(int x, int y);
	void OnMouseMove(int x, int y);
	void OnMouseWheel(int x, int y,float z);
	float forward_speed = 0.0;
	float right_speed = 0.0;
	float speed_base = 20.0;
	float rotation_speed_pitch = 5.f;
	float rotation_speed_yaw = 5.f;
	bool right_button_pressed = false;
	int last_x=0;
	int last_y=0;
	float delta_pitch_screen = 0;
	float delta_yaw_screen = 0;
	SmoothStep smooth_delta_pitch;
	SmoothStep smooth_delta_yaw;
	float wheel_forward_base = 2000.f;
	float wheel_speed = 0.0;
};