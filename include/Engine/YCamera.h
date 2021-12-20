#pragma once
#include "Math/YMatrix.h"
#include "Math/YRotator.h"
class CameraBase
{
public:
	CameraBase();
	virtual ~CameraBase();
	
	YMatrix GetViewMatrix() const;
	YMatrix GetProjectionMatrix() const;
	YMatrix GetViewProjectionMatrix() const;
	YMatrix GetInvViewProjectionMatrix() const;
	float GetNearPlane() const;
	float GetFarPlane() const;
	float GetFovY() const; //degree
	float GetAspect() const;
	bool IsPerspectiveCamera() const;


	void SetNearPlane(float near_plane);
	void SetFarPlane(float far_plane);
	void SetFovY(float fov_y); //degree
	void SetAspect(float aspect); 
	void SetPosition(const YVector& position);
	void SetRotation(const YRotator& rotator);
	virtual void Update();
protected:
	YVector position_;
	YRotator rotaion_;
	
	YMatrix inv_view_matrix;
	YMatrix view_matrix_;
	YMatrix projection_matrix_;
	YMatrix view_proj_matrix_;
	YMatrix inv_view_proj_matrix_;
	float near_plane_ = 0;
	float far_plane_ = 0;
	float fov_y_ = 45.0;
	float aspect_ = 1.f;
	bool perspective_camera_ = true;
};

class PerspectiveCamera:public CameraBase
{
public:
	PerspectiveCamera();
	PerspectiveCamera(float fov, float aspec,float near_plane, float far_plane);
	void Update() override;
	static YMatrix GetPerspectiveMatrix(float radian_y_fov, float aspec, float near_plane, float far_plane);
};