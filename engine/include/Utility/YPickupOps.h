#pragma once
#include "Math/YVector.h"

class YPickupOps
{
public:
	YPickupOps();
	virtual ~YPickupOps();
	virtual void RegiesterEventProcess();
	virtual void Update(double delta_time);
};

class YPickupShowMove :public YPickupOps
{
public:
	YPickupShowMove();
	~YPickupShowMove() override;
	virtual void RegiesterEventProcess();
	virtual void Update(double delta_time);
	void OnLButtonDown(int x, int y);
	void OnMouseMove(int x, int y);
	void RayCast(int x, int y);
protected:

	YVector2 last_mouse_pos_{ 0.0,0.0 };
	YVector last_v0 = YVector::zero_vector;
	YVector last_v1 = YVector::zero_vector;
	YVector last_v2 = YVector::zero_vector;
	YVector last_hit_pos = YVector::zero_vector;
	YVector last_hit_normal = YVector::zero_vector;
};
