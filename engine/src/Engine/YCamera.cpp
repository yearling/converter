#include "Engine/YCamera.h"

PerspectiveCamera::PerspectiveCamera()
{
	perspective_camera_ = true;
}

PerspectiveCamera::PerspectiveCamera(float fov, float aspec, float near_plane, float far_plane)
{
	perspective_camera_ = true;
	fov_y_ = fov;
	aspect_ = aspec;
	near_plane_ = near_plane;
	far_plane_ = far_plane;
}

void PerspectiveCamera::Update()
{
	CameraBase::Update();
	float radian_y = YMath::DegreesToRadians(fov_y_);
	projection_matrix_ = GetPerspectiveMatrix(radian_y, aspect_, near_plane_, far_plane_);
	inv_view_proj_matrix_ = projection_matrix_.Inverse();
	view_proj_matrix_ = view_matrix_ * projection_matrix_;
	inv_view_proj_matrix_ = view_proj_matrix_.Inverse();
}

YMatrix PerspectiveCamera::GetPerspectiveMatrix(float radian_y_fov, float aspec, float near_plane, float far_plane)
{
	float half_y_fov = 0.5f * radian_y_fov;
	YMatrix tmp;
	tmp.m[0][0] = 1.f / aspec / YMath::Tan(half_y_fov);
	tmp.m[0][1] = 0.f;
	tmp.m[0][2] = 0.f;
	tmp.m[0][3] = 0.f;

	tmp.m[1][0] = 0.f;
	tmp.m[1][1] = 1.f / YMath::Tan(half_y_fov);
	tmp.m[1][2] = 0.f;
	tmp.m[1][3] = 0.f;

	tmp.m[2][0] = 0.f;
	tmp.m[2][1] = 0.f;
	tmp.m[2][2] = ((near_plane == far_plane) ? 1.0f : far_plane / (far_plane - near_plane));
	tmp.m[2][3] = 1.f; //z;

	tmp.m[3][0] = 0.f;
	tmp.m[3][1] = 0.f;
	tmp.m[3][2] = -near_plane * ((near_plane == far_plane) ? 1.0f : far_plane / (far_plane - near_plane));
	tmp.m[3][3] = 0.f;
	return tmp;
}

CameraBase::CameraBase()
{
	position_ = YVector(0.0, 0.0, 0.0);
	rotaion_ = YRotator(0.0, 0.0, 0.0);
	CameraBase::Update();
}

CameraBase::~CameraBase()
{

}

YMatrix CameraBase::GetViewMatrix()const
{
	return view_matrix_;
}

YMatrix CameraBase::GetInvViewMatrix() const
{
	return inv_view_matrix_;
}

YMatrix CameraBase::GetProjectionMatrix()const
{
	return projection_matrix_;
}

YMatrix CameraBase::GetViewProjectionMatrix() const
{
	return view_proj_matrix_;
}

YMatrix CameraBase::GetInvViewProjectionMatrix() const
{
	return inv_view_proj_matrix_;
}

float CameraBase::GetNearPlane() const
{
	return near_plane_;
}

float CameraBase::GetFarPlane() const
{
	return far_plane_;
}

float CameraBase::GetFovY() const
{
	return fov_y_;
}

float CameraBase::GetAspect() const
{
	return aspect_;
}

bool CameraBase::IsPerspectiveCamera() const
{
	return perspective_camera_;
}

YVector CameraBase::GetPosition() const
{
	return position_;
}

YRotator CameraBase::GetRotator() const
{
	return rotaion_;
}

void CameraBase::SetNearPlane(float near_plane)
{
	near_plane_ = near_plane;
}

void CameraBase::SetFarPlane(float far_plane)
{
	far_plane_ = far_plane;
}

void CameraBase::SetFovY(float fov_y)
{
	fov_y_ = fov_y;
}

void CameraBase::SetAspect(float aspect)
{
	aspect_ = aspect;
}

void CameraBase::SetPosition(const YVector& position)
{
	position_ = position;
	Update();
}

void CameraBase::SetRotation(const YRotator& rotator)
{
	rotaion_ = rotator;
	Update();
}

void CameraBase::Update()
{
	inv_view_matrix_ = YMatrix(rotaion_, position_);
	view_matrix_ = inv_view_matrix_.Inverse();
}

std::unique_ptr<CameraElementProxy> CameraBase::GetProxy()
{
	std::unique_ptr<CameraElementProxy> proxy = std::make_unique<CameraElementProxy>();
	proxy->position_ = position_;
	proxy->rotation_ = rotaion_;
	proxy->inv_view_matrix_ = inv_view_matrix_;
	proxy->view_matrix_ = view_matrix_;
	proxy->projection_matrix_ = projection_matrix_;
	proxy->view_proj_matrix_ = view_proj_matrix_;
	proxy->inv_view_proj_matrix_ = inv_view_proj_matrix_;
	proxy->near_plane_ = near_plane_;
	proxy->far_plane_ = far_plane_;
	proxy->fov_y_ = fov_y_;
	proxy->aspect_ = aspect_;
	proxy->perspective_camera_ = perspective_camera_;
	return proxy;
}
