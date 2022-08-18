#include "MeshUtility/YPickupOps.h"
#include "Engine/YInputManager.h"
#include "SObject/SWorld.h"
#include "Engine/YCanvas.h"
#include <algorithm>
#include "Engine/YEngine.h"
#include "Utility/YStringFormat.h"
#include <set>
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
	g_input_manager->mouse_LButton_up_funcs_.push_back([this](int x, int y) {this->OnLButtonUp(x,y); });
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

struct FlowFlag
{
    int triangle_id = -1;
    bool visited = false;
    int wedge_id = -1;
};

void RecursiveFindGroup(YLODMesh& lod_mesh, int triangle_id, std::set<int>& out_triangle_group, std::unordered_map<int, FlowFlag>& around_triangle_ids)
{
    if (around_triangle_ids[triangle_id].visited)
    {
        return;
    }
    around_triangle_ids[triangle_id].visited = true;
    out_triangle_group.insert(triangle_id);
    int cur_wedge_index = around_triangle_ids[triangle_id].wedge_id;
    YMeshPolygon& polygon = lod_mesh.polygons[triangle_id];
    int last_id[2] = { -1,-1 };
    if (polygon.wedge_ids[0] == cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[1];
        last_id[1] = polygon.wedge_ids[2];
    }
    else if (polygon.wedge_ids[1] == cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[0];
        last_id[1] = polygon.wedge_ids[2];
    }
    else if (polygon.wedge_ids[2] = cur_wedge_index)
    {
        last_id[0] = polygon.wedge_ids[0];
        last_id[1] = polygon.wedge_ids[1];
    }

    YMeshWedge& cur_wedge = lod_mesh.vertex_instances[cur_wedge_index];
    YMeshWedge& wedge1 = lod_mesh.vertex_instances[last_id[0]];
    YMeshWedge& wedge2 = lod_mesh.vertex_instances[last_id[1]];
    int edge_id01 = lod_mesh.GetVertexPairEdge(cur_wedge.control_point_id, wedge1.control_point_id);
    assert(edge_id01 != -1);
    YMeshEdge& edge_01 = lod_mesh.edges[edge_id01];
    if (!edge_01.edge_hardness)
    {
        int other_triangle_id = -1;
        //有可能在边界
        for (int i_sub_edge = 0; i_sub_edge < edge_01.connected_triangles.size(); ++i_sub_edge)
        {
            int triangle_same_edege = edge_01.connected_triangles[i_sub_edge];
            if (triangle_same_edege == triangle_id)
            {
                continue;
            }
            else
            {
                RecursiveFindGroup(lod_mesh, triangle_same_edege, out_triangle_group, around_triangle_ids);
            }
        }
    }


    int edge_id02 = lod_mesh.GetVertexPairEdge(cur_wedge.control_point_id, wedge2.control_point_id);
    assert(edge_id02 != -1);
    YMeshEdge& edge_02 = lod_mesh.edges[edge_id02];
    if (!edge_02.edge_hardness)
    {
        int other_triangle_id = -1;
        //有可能在边界
        for (int i_sub_edge = 0; i_sub_edge < edge_02.connected_triangles.size(); ++i_sub_edge)
        {
            int triangle_same_edege = edge_02.connected_triangles[i_sub_edge];
            if (triangle_same_edege == triangle_id)
            {
                continue;
            }
            else
            {
                RecursiveFindGroup(lod_mesh, triangle_same_edege, out_triangle_group, around_triangle_ids);
            }
        }
    }
}
std::vector<std::set<int>> GetSplitTriangleGroupBySoftEdge(YLODMesh& lod_mesh, int wedge_index)
{
    YMeshWedge& wedge = lod_mesh.vertex_instances[wedge_index];
    int triangle_id = wedge.connected_triangles[0];
    YMeshPolygon& start_triangle = lod_mesh.polygons[triangle_id];

    YMeshControlPoint& control_point_search = lod_mesh.vertex_position[wedge.control_point_id];
    std::unordered_map<int,FlowFlag> around_triangle_ids;
    for (int wedge_id : control_point_search.wedge_ids)
    {
        FlowFlag tmp_flag;
        tmp_flag.triangle_id = lod_mesh.vertex_instances[wedge_id].connected_triangles[0];
        tmp_flag.wedge_id = wedge_id;
        tmp_flag.visited = false;
        around_triangle_ids[tmp_flag.triangle_id] = tmp_flag;
    }
    std::vector<std::set<int>> result;
    std::set<int> first_group;
    RecursiveFindGroup(lod_mesh, triangle_id, first_group, around_triangle_ids);
    result.push_back(first_group);

    auto func = [&]() {
        for (auto& left_triangle_pair : around_triangle_ids)
        {
            if (!left_triangle_pair.second.visited)
            {
                return left_triangle_pair.first;
            }
        }
        return -1;
    };
    int left_triangle_id = func();
    while (left_triangle_id != -1)
    {
        std::set<int> second_group;
        RecursiveFindGroup(lod_mesh, left_triangle_id, second_group, around_triangle_ids);
        result.push_back(second_group);
        left_triangle_id = func();
    }
    return result;

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
	/*	g_Canvas->DrawLine(last_v0, last_v1, YVector4(0.0, 1.0, 0.0, 1.0), false);
		g_Canvas->DrawLine(last_v1, last_v2, YVector4(0.0, 1.0, 0.0, 1.0), false);
		g_Canvas->DrawLine(last_v2, last_v0, YVector4(0.0, 1.0, 0.0, 1.0), false);*/
		g_Canvas->DrawLine(last_hit_pos, last_hit_pos + last_hit_normal * 1.0, YVector4(0.0, 0.0, 1.0, 1.0), false);
	}

    auto func_draw_triangle = [&](int triangle_id,const YVector4& color)
    {
        YEngine* engine = YEngine::GetEngine();
        YStaticMesh* static_mesh = engine->static_mesh_->GetStaticMesh();
        YLODMesh& lod_mesh = static_mesh->raw_meshes[0];
        YMeshPolygon& triangle = lod_mesh.polygons[triangle_id];
        YMeshWedge& wedge0 = lod_mesh.vertex_instances[triangle.wedge_ids[0]];
        YMeshWedge& wedge1 = lod_mesh.vertex_instances[triangle.wedge_ids[1]];
        YMeshWedge& wedge2 = lod_mesh.vertex_instances[triangle.wedge_ids[2]];
        int control_point[3] = { wedge0.control_point_id, wedge1.control_point_id,wedge2.control_point_id };
        YVector triangle_pos[3] = { lod_mesh.vertex_position[control_point[0]].position,
        lod_mesh.vertex_position[control_point[1]].position ,
        lod_mesh.vertex_position[control_point[2]].position };
        g_Canvas->DrawLine(triangle_pos[0], triangle_pos[1], color, false);
        g_Canvas->DrawLine(triangle_pos[1], triangle_pos[2], color, false);
        g_Canvas->DrawLine(triangle_pos[2], triangle_pos[0], color, false);
    };
    if (select_wedge_id != INVALID_ID)
    {
        YEngine* engine = YEngine::GetEngine();
        YStaticMesh* static_mesh = engine->static_mesh_->GetStaticMesh();
        YLODMesh& lod_mesh = static_mesh->raw_meshes[0];
        YMeshWedge& wedge = lod_mesh.vertex_instances[select_wedge_id];
        int triangle_id = wedge.connected_triangles[0];
        YMeshPolygon& triangle = lod_mesh.polygons[triangle_id];
        int other_wedge_id0 = -1;
        int other_wedge_id1 = -1;
        if (triangle.wedge_ids[0] == select_wedge_id)
        {
            other_wedge_id0 = triangle.wedge_ids[1];
            other_wedge_id1 = triangle.wedge_ids[2];
        }
        if (triangle.wedge_ids[1] == select_wedge_id)
        {
            other_wedge_id0 = triangle.wedge_ids[0];
            other_wedge_id1 = triangle.wedge_ids[2];
        }
        if (triangle.wedge_ids[2] == select_wedge_id)
        {
            other_wedge_id0 = triangle.wedge_ids[0];
            other_wedge_id1 = triangle.wedge_ids[1];
        }
        YMeshWedge& wedge1 = lod_mesh.vertex_instances[other_wedge_id0];
        YMeshWedge& wedge2 = lod_mesh.vertex_instances[other_wedge_id1];
        int control_point[3] = { wedge.control_point_id, wedge1.control_point_id,wedge2.control_point_id };
        //YVector triangle
        YMeshControlPoint& control_point_search = lod_mesh.vertex_position[wedge.control_point_id];
        std::vector<int> around_triangle_ids;
        for (int wedge_id : control_point_search.wedge_ids)
        {
            around_triangle_ids.push_back(lod_mesh.vertex_instances[wedge_id].connected_triangles[0]);
        }


        for (int triangle_id : around_triangle_ids)
        {
            //func_draw_triangle(triangle_id);
        }

        std::vector<std::set<int>> triangle_group = GetSplitTriangleGroupBySoftEdge(lod_mesh, select_wedge_id);
        YVector4 pallet[5] = { YVector4(1.0,0.0,1.0,1.0),YVector4(0.0,0.0,1.0,1.0), YVector4(1.0,1.0,0.0,1.0),YVector4(0.0,1.0,1.0,1.0), YVector4(1.0,1.0,1.0,1.0) };
        for (int group_id = 0; group_id < triangle_group.size(); ++group_id)
        {
            int color_index = group_id % 5;
            std::set<int>& triangles = triangle_group[group_id];
            for (int triangle_final_id_in_a_set : triangles)
            {
                func_draw_triangle(triangle_final_id_in_a_set,pallet[color_index]);
            }
        }
    }
}

void YPickupShowMove::OnLButtonDown(int x, int y)
{
    begin_search = true;
    select_wedge_id = INVALID_ID;
	RayCast(x, y);
}

void YPickupShowMove::OnLButtonUp(int x, int y)
{
   
    begin_search = false;
    LOG_INFO("select wedge id:", select_wedge_id);
   
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
    int polygon_index;
    float w;
    float u;
    float v;
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
	

    return;
    YEngine* engine = YEngine::GetEngine();
    YStaticMesh* static_mesh = engine->static_mesh_->GetStaticMesh();

    std::unique_ptr<ImportedRawMesh>& lod_mesh_ptr = static_mesh->imported_raw_meshes_[0];
    ImportedRawMesh& lod_mesh = *(lod_mesh_ptr);
    //YLODMesh& lod_mesh = static_mesh->raw_meshes[0];
    std::vector< HitElement> hit_result;
    for (YMeshPolygonGroup& polygon_group : lod_mesh.polygon_groups)
    {
        for (int polygon_index : polygon_group.polygons)
        {
            YMeshPolygon& polygon = lod_mesh.polygons[polygon_index];
            YMeshWedge& vertex_ins_0 = lod_mesh.wedges[polygon.wedge_ids[0]];
            YMeshWedge& vertex_ins_1 = lod_mesh.wedges[polygon.wedge_ids[1]];
            YMeshWedge& vertex_ins_2 = lod_mesh.wedges[polygon.wedge_ids[2]];
            YVector p0 = lod_mesh.control_points[vertex_ins_0.control_point_id].position;
            YVector p1 = lod_mesh.control_points[vertex_ins_1.control_point_id].position;
            YVector p2 = lod_mesh.control_points[vertex_ins_2.control_point_id].position;
            /* p0 = static_mesh_component->GetTransformToWorld().TransformPosition(p0);
             p1 = static_mesh_component->GetTransformToWorld().TransformPosition(p1);
             p2 = static_mesh_component->GetTransformToWorld().TransformPosition(p2);*/
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
                hit_elem.w = w;
                hit_elem.u = u;
                hit_elem.v = v;
                hit_elem.polygon_index = polygon_index;
                hit_result.push_back(hit_elem);
                //LOG_INFO(StringFormat("HIT w.p0 %f, u.p1 %f, v.p2 %f", w, u, v));
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
        float w = hit_result[0].w;
        float u = hit_result[0].u;
        float v = hit_result[0].v;
        if (begin_search)
        {
            int select_point_id = -1;
            if (w > u && w > v)
            {
                select_point_id = 0;
            }
            else if (u > w && u > v)
            {
                select_point_id = 1;
            }
            else if (v > w && v > u)
            {
                select_point_id = 2;
            }
            if (select_point_id != -1)
            {
                YMeshPolygon& polygon = lod_mesh.polygons[hit_result[0].polygon_index];
                select_wedge_id = polygon.wedge_ids[select_point_id];
                LOG_INFO("select polygon :",hit_result[0].polygon_index, "  wedge index: ", select_wedge_id);
            }
        }
    }
    else
    {
        if (begin_search)
        {
            select_wedge_id = INVALID_ID;
            select_triangle_id = INVALID_ID;
        }
    }
#if 0
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
						YMeshVertexWedge& vertex_ins_0 = lod_mesh.vertex_instances[polygon.wedge_ids[0]];
						YMeshVertexWedge& vertex_ins_1 = lod_mesh.vertex_instances[polygon.wedge_ids[1]];
						YMeshVertexWedge& vertex_ins_2 = lod_mesh.vertex_instances[polygon.wedge_ids[2]];
						YVector p0 = lod_mesh.vertex_position[vertex_ins_0.control_point_id].position;
						YVector p1 = lod_mesh.vertex_position[vertex_ins_1.control_point_id].position;
						YVector p2 = lod_mesh.vertex_position[vertex_ins_2.control_point_id].position;
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
#endif
}
