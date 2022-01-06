#include "Utility/YPickupOps.h"
#include "Engine/YInputManager.h"
#include "SObject/SWorld.h"
#include "Engine/YCanvas.h"
YPickupOps::YPickupOps()
{

}

YPickupOps::~YPickupOps()
{

}

void YPickupOps::RegiesterEventProcess()
{

}

void YPickupOps::Update(double delta_time)
{
	
}

YPickupShowMove::YPickupShowMove()
{

}

YPickupShowMove::~YPickupShowMove()
{

}

void YPickupShowMove::RegiesterEventProcess()
{
	g_input_manager->mouse_LButton_down_funcs_.push_back([this](int x, int y) {this->OnLButtonDown(x,y); });
	g_input_manager->mouse_move_functions_.push_back([this](int x, int y) {this->OnMouseMove(x,y); });
}

void YPickupShowMove::Update(double delta_time)
{
	const YViewPort& view_port = SWorld::GetWorld()->GetMainScene()->view_port_;
	float ScreenCoordinateX = 1 / float(view_port.width_ / 2) * (float)last_mouse_pos_.x - 1.0f;
	float ScreenCoordinateY = -1 / float(view_port.height_ / 2) * (float)last_mouse_pos_.y + 1.0f;
	SPerspectiveCameraComponent* camera_component = SWorld::GetWorld()->GetMainScene()->GetPerspectiveCameraComponent();
	if (!camera_component)
	{
		return;
	}
	YRay ray = camera_component->camera_->GetWorldRayFromScreen(YVector2(ScreenCoordinateX, ScreenCoordinateY));
	ray.origin_ = camera_component->camera_->GetPosition()- camera_component->camera_->GetInvViewMatrix().TransformVector(YVector::forward_vector);
	g_Canvas->DrawRay(ray, YVector4(1.0, 0.0, 0.0, 1.0));

	g_Canvas->DrawLine(last_v0, last_v1, YVector4(0.0,1.0,0.0,1.0));
	g_Canvas->DrawLine(last_v1, last_v2, YVector4(0.0, 1.0, 0.0, 1.0));
	g_Canvas->DrawLine(last_v2, last_v0, YVector4(0.0, 1.0, 0.0, 1.0));
	g_Canvas->DrawLine(last_hit_pos, last_hit_pos + last_hit_normal, YVector4(0.0, 0.0, 1.0, 1.0));
}

void YPickupShowMove::OnLButtonDown(int x, int y)
{
	RayCast(x, y);
}

void YPickupShowMove::OnMouseMove(int x, int y)
{
	RayCast(x, y);
	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;
}

void YPickupShowMove::RayCast(int x, int y)
{
	const YViewPort& view_port=	SWorld::GetWorld()->GetMainScene()->view_port_;
	float ScreenCoordinateX = 1 / float(view_port.width_ / 2) * (float)x- 1.0f;
	float ScreenCoordinateY = -1 / float(view_port.height_ / 2) * (float)y + 1.0f;
	SPerspectiveCameraComponent* camera_component =  SWorld::GetWorld()->GetMainScene()->GetPerspectiveCameraComponent();
	if (!camera_component)
	{
		return;
	}
    YRay ray = camera_component->camera_->GetWorldRayFromScreen(YVector2(ScreenCoordinateX, ScreenCoordinateY));


	for (SStaticMeshComponent* static_mesh_component : SWorld::GetWorld()->GetMainScene()->static_meshes_components_)
	{
		if (YMath::LineBoxIntersection(static_mesh_component->GetBounds(), ray))
		{
			LOG_INFO("pick up mesh:", static_mesh_component->GetMesh()->model_name);
			YLODMesh& lod_mesh = static_mesh_component->GetMesh()->raw_meshes[0];
			for (YMeshPolygonGroup& polygon_group : lod_mesh.polygon_groups)
			{
				for (int polygon_index : polygon_group.polygons)
				{
					YMeshPolygon& polygon = lod_mesh.polygons[polygon_index];
					YMeshVertexInstance& vertex_ins_0 = lod_mesh.vertex_instances[polygon.vertex_instance_ids[0]];
					YMeshVertexInstance& vertex_ins_1 = lod_mesh.vertex_instances[polygon.vertex_instance_ids[1]];
					YMeshVertexInstance& vertex_ins_2 = lod_mesh.vertex_instances[polygon.vertex_instance_ids[2]];
					YVector p0 = lod_mesh.vertex_position[vertex_ins_0.vertex_id].position;
					YVector p1 = lod_mesh.vertex_position[vertex_ins_1.vertex_id].position;
				    YVector p2 = lod_mesh.vertex_position[vertex_ins_2.vertex_id].position;
					p0 = static_mesh_component->GetTransformToWorld().TransformPosition(p0);
					p1 = static_mesh_component->GetTransformToWorld().TransformPosition(p1);
					p2 = static_mesh_component->GetTransformToWorld().TransformPosition(p2);
					float t, u, v, w;
					if (RayCastTriangle(ray, p0, p1, p2, t, u, v, w))
					{
						YVector hit_pos = w * p0 + u * p1 + v * p2;
						last_hit_pos = hit_pos;
						YVector normal = ((p0 - p1) ^ (p0 - p2)).GetSafeNormal();
						last_hit_normal = - normal;
						last_v0 = p0;
						last_v1 = p1;
						last_v2 = p2;
					}
				}
			}
		}
	}
}
