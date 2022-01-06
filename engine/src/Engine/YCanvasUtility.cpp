#include "Engine/YCanvas.h"
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "Engine/YLog.h"
#include <fstream>
#include "Engine/YCanvasUtility.h"
#include "Render/YRenderInterface.h"
#include "Engine/YRenderScene.h"
void DrawUtility::DrawGrid()
{
	// DrawGrids
	float xStart = -100.0f;
	float xEnd = 100.0f;
	float zStart = -100.0f;
	float zEnd = 100.0f;
	float Grid = 10.0f;

	for (float xCurrent = xStart; xCurrent < xEnd + 1.0f; xCurrent += Grid)
	{
		g_Canvas->DrawLine(YVector(xCurrent, 0, zStart), YVector(xCurrent, 0, zEnd), YVector4(1.0f, 1.0f, 1.0f, 0.3f));
	}
	for (float zCurrent = zStart; zCurrent < zEnd + 1.0f; zCurrent += Grid)
	{
		g_Canvas->DrawLine(YVector(xStart, 0, zCurrent), YVector(xEnd, 0, zCurrent), YVector4(1.0f, 1.0f, 1.0f, 0.3f));
	}
}

void DrawUtility::DrawWorldCoordinate(CameraBase* camera)
{
	float pos_x = -0.99f * camera->GetNearPlane();
	float pos_y = -0.98f * camera->GetNearPlane();
	YVector4 projection_pos = YVector4(pos_x, pos_y, 0, camera->GetNearPlane());
	YMatrix inv_projection_matrix = camera->GetProjectionMatrix().Inverse();
	YMatrix inv_view_matrix = camera->GetInvViewMatrix();

	YVector view_position = inv_projection_matrix.TransformVector4(projection_pos).AffineTransform();
	view_position.z += 0.1f;
	YVector world_position = inv_view_matrix.TransformPosition(view_position);
	const float coordinate_scale = 0.2f;
	g_Canvas->DrawLine(world_position, world_position + YVector::right_vector * coordinate_scale, YVector4(1, 0, 0, 1));
	g_Canvas->DrawLine(world_position, world_position + YVector::up_vector * coordinate_scale, YVector4(0, 1, 0, 1));
	g_Canvas->DrawLine(world_position, world_position + YVector::forward_vector * coordinate_scale, YVector4(0, 0, 1, 1));

}


void DrawUtility::DrawWorldCoordinate(class YScene* render_param)
{
	if (!render_param->perspective_camera_components_)
	{	
		return;
	}
	CameraBase* camera = render_param->perspective_camera_components_->camera_.get();
	float pos_x = -0.99f;
	float pos_y = -0.98f;
	YVector4 projection_pos = YVector4(pos_x, pos_y, 0, 1.0);
	YMatrix inv_projection_matrix = camera->GetProjectionMatrix().Inverse();
	YMatrix inv_view_matrix = camera->GetInvViewMatrix();

	YVector view_position = inv_projection_matrix.TransformVector4(projection_pos).AffineTransform();
	view_position.z += camera->GetNearPlane()* 0.1f;
	YVector world_position = inv_view_matrix.TransformPosition(view_position);
	const float coordinate_scale = 0.07f*camera->GetNearPlane();
	g_Canvas->DrawLine(world_position, world_position + YVector::right_vector * coordinate_scale, YVector4(1, 0, 0, 1));
	g_Canvas->DrawLine(world_position, world_position + YVector::up_vector * coordinate_scale, YVector4(0, 1, 0, 1));
	g_Canvas->DrawLine(world_position, world_position + YVector::forward_vector * coordinate_scale, YVector4(0, 0, 1, 1));
}
