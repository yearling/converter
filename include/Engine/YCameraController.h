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
		int smooth_frame_count = 15;
	};
protected:
	void OnKeyDown(char c);
	void OnKeyUp(char c);
	void OnRButtonDown(int x, int y);
	void OnRButtonUp(int x, int y);
	void OnMouseMove(int x, int y);
	float forward_speed = 0.0;
	float right_speed = 0.0;
	float speed_base = 20.0;
	float rotation_speed_pitch = 5.f;
	float rotation_speed_yaw = 3.f;
	bool right_button_pressed = false;
	int last_x;
	int last_y;
	float delta_pitch = 0;
	float delta_yaw = 0;
	SmoothStep smooth_delta_pitch;
	SmoothStep smooth_delta_yaw;
	float decom = 3;
};