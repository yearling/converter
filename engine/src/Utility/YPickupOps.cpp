#include "Utility/YPickupOps.h"
#include "Engine/YInputManager.h"
#include "SObject/SWorld.h"
#include "Engine/YCanvas.h"
#include <algorithm>
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

struct HitMesh
{
	SStaticMeshComponent* mesh;
	float t;
};

bool operator<(const HitMesh& lhs, const HitMesh& rhs)
{
	return lhs.t < rhs.t;
}
void YPickupShowMove::Update(double delta_time)
{
	
	if (show_aabb_)
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
		ray.origin_ = camera_component->camera_->GetPosition() - camera_component->camera_->GetInvViewMatrix().TransformVector(YVector::forward_vector);

		g_Canvas->DrawRay(ray, YVector4(1.0, 0.0, 0.0, 1.0), false);
		std::vector<HitMesh> raytraced_meshes;
		for (SStaticMeshComponent* static_mesh_component : SWorld::GetWorld()->GetMainScene()->static_meshes_components_)
		{
			float t = 0;
			if (YMath::LineBoxIntersection(static_mesh_component->GetBounds(), ray, t))
			{
				HitMesh hit_result;
				hit_result.mesh = static_mesh_component;
				hit_result.t = t;
				raytraced_meshes.push_back(hit_result);
			}
			else
			{
				g_Canvas->DrawAABB(static_mesh_component->GetBounds(), YVector4(0.0, 0.5, 0.0, 1.0), true);
			}

		}

		std::sort(raytraced_meshes.begin(), raytraced_meshes.end());
		for (int i = 0; i < raytraced_meshes.size(); ++i)
		{
			if (i == 0)
			{
				g_Canvas->DrawAABB(raytraced_meshes[i].mesh->GetBounds(), YVector4(1.0, 0.0, 0.0, 1.0), true);
			}
			else
			{
				g_Canvas->DrawAABB(raytraced_meshes[i].mesh->GetBounds(), YVector4(0.5, 0.5, 0.0, 1.0), true);
			}
		}
	}


	if (ray_cast_to_triangle)
	{
		g_Canvas->DrawLine(last_v0, last_v1, YVector4(0.0, 1.0, 0.0, 1.0), false);
		g_Canvas->DrawLine(last_v1, last_v2, YVector4(0.0, 1.0, 0.0, 1.0), false);
		g_Canvas->DrawLine(last_v2, last_v0, YVector4(0.0, 1.0, 0.0, 1.0), false);
		g_Canvas->DrawLine(last_hit_pos, last_hit_pos + last_hit_normal * 0.5, YVector4(0.0, 0.0, 1.0, 1.0), false);
	}
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
struct HitElement
{
	YVector v0;
	YVector v1;
	YVector v2;
	YVector Normal;
	YVector hit_poition;
	float t;

};
bool operator<(const HitElement& lhs, const HitElement& rhs)
{
	return lhs.t < rhs.t;
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
			//LOG_INFO("pick up mesh:", static_mesh_component->GetMesh()->model_name);
			if (ray_cast_to_triangle)
			{
				YLODMesh& lod_mesh = static_mesh_component->GetMesh()->raw_meshes[0];
				std::vector< HitElement> hit_result;
				for (YMeshPolygonGroup& polygon_group : lod_mesh.polygon_groups)
				{
					for (int polygon_index : polygon_group.polygons)
					{
						YMeshPolygon& polygon = lod_mesh.polygons[polygon_index];
						YMeshVertexInstance& vertex_ins_0 = lod_mesh.vertex_instances[polygon.vertex_instance_ids[0]];
						YMeshVertexInstance& vertex_ins_1 = lod_mesh.vertex_instances[polygon.vertex_instance_ids[1]];
						YMeshVertexInstance& vertex_ins_2 = lod_mesh.vertex_instances[polygon.vertex_instance_ids[2]];
						YVector p0 = lod_mesh.vertex_position[vertex_ins_0.vertex_position_id].position;
						YVector p1 = lod_mesh.vertex_position[vertex_ins_1.vertex_position_id].position;
						YVector p2 = lod_mesh.vertex_position[vertex_ins_2.vertex_position_id].position;
						p0 = static_mesh_component->GetTransformToWorld().TransformPosition(p0);
						p1 = static_mesh_component->GetTransformToWorld().TransformPosition(p1);
						p2 = static_mesh_component->GetTransformToWorld().TransformPosition(p2);
						float t, u, v, w;
						if (RayCastTriangle(ray, p0, p1, p2, t, u, v, w))
						{
							YVector hit_pos = w * p0 + u * p1 + v * p2;
							last_hit_pos = hit_pos;
							YVector normal = ((p0 - p1) ^ (p0 - p2)).GetSafeNormal();
							last_hit_normal = -normal;
							last_v0 = p0;
							last_v1 = p1;
							last_v2 = p2;
							HitElement hit_elem;
							hit_elem.v0 = p0;
							hit_elem.v1 = p1;
							hit_elem.v2 = p2;
							hit_elem.Normal = -normal;
							hit_elem.hit_poition = hit_pos;
							hit_elem.t = t;
							hit_result.push_back(hit_elem);
						}
					}
				}
				std::sort(hit_result.begin(), hit_result.end());
				if (!hit_result.empty())
				{
					last_v0 = hit_result[0].v0;
					last_v1 = hit_result[0].v1;
					last_v2 = hit_result[0].v2;
					last_hit_pos = hit_result[0].hit_poition;
					last_hit_normal = hit_result[0].Normal;
				}
			}
		}
	}
}
