#include <cassert>
#include "fbxsdk/scene/geometry/fbxlayer.h"
#include "YFbxImporter.h"
#include "Engine/YRawMesh.h"
#include "Math/YMath.h"
#include "YFbxUtility.h"
#include "Engine/YMaterial.h"
#include "Engine/YLog.h"
#include <unordered_set>
#include "YFbxPostProcess.h"

bool YFbxImporter::BuildStaticMeshFromGeometry(FbxNode* node, ImportedRawMesh& raw_mesh)
{
    FbxMesh* fbx_mesh = node->GetMesh();
    std::string node_name = node->GetName();
    raw_mesh.name = node_name;
    FbxUVs fbx_uvs(this, fbx_mesh);

    int material_count = node->GetMaterialCount();
    std::unordered_map<int, std::shared_ptr<YImportedMaterial>> materials;
    //create material
    {
        FindOrImportMaterialsFromNode(node, materials, fbx_uvs.uv_set);
        if (material_count == 0)
        {
            materials[0] = std::make_shared<YImportedMaterial>();
            material_count = 1;
        }
    }


    // Must do this before triangulating the mesh due to an FBX bug in TriangulateMeshAdvance
    int layer_smoothing_cout = fbx_mesh->GetLayerCount(FbxLayerElement::eSmoothing);
    for (int i = 0; i < layer_smoothing_cout; i++)
    {
        FbxLayerElementSmoothing const* SmoothingInfo = fbx_mesh->GetLayer(0)->GetSmoothing();
        if (SmoothingInfo && SmoothingInfo->GetMappingMode() != FbxLayerElement::eByPolygon)
        {
            fbx_geometry_converter_->ComputePolygonSmoothingFromEdgeSmoothing(fbx_mesh, i);
        }
    }

    if (!fbx_mesh->IsTriangleMesh())
    {
        WARNING_INFO("Triangulation static mesh", node->GetName());

        const bool bReplace = true;
        FbxNodeAttribute* ConvertedNode = fbx_geometry_converter_->Triangulate(fbx_mesh, bReplace);

        if (ConvertedNode != NULL && ConvertedNode->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            fbx_mesh = (fbxsdk::FbxMesh*)ConvertedNode;
        }
        else
        {
            ERROR_INFO("Unable to triangulate mesh ", node->GetName());
            return false;
        }
    }

    // renew the base layer
    FbxLayer* base_layer = fbx_mesh->GetLayer(0);
    {
        fbx_uvs.Phase2(this, fbx_mesh);
    }
    if (fbx_uvs.unique_count == 0)
    {
        ERROR_INFO("mesh ", raw_mesh.name, " does not have uvs");
        return false;
    }
    //	get the "material index" layer.  Do this AFTER the triangulation step as that may reorder material indices
    FbxLayerElementMaterial* layer_element_material = base_layer->GetMaterials();
    FbxLayerElement::EMappingMode material_mapping_model = layer_element_material ? layer_element_material->GetMappingMode() : FbxLayerElement::eByPolygon;


    // get the smoothing group layer
    bool is_smoothing_avaliable = false;
    FbxLayerElementSmoothing* layer_element_smoothing = base_layer->GetSmoothing();
    FbxLayerElement::EReferenceMode smoothing_reference_mode = FbxLayerElement::eDirect;
    FbxLayerElement::EMappingMode smoothing_mapping_mode = FbxLayerElement::eByEdge;
    if (layer_element_smoothing)
    {
        if (layer_element_smoothing->GetMappingMode() == FbxLayerElement::eByPolygon)
        {
            // convert the base layer to edge smoothing
            fbx_geometry_converter_->ComputeEdgeSmoothingFromPolygonSmoothing(fbx_mesh, 0);
            base_layer = fbx_mesh->GetLayer(0);
            layer_element_smoothing = base_layer->GetSmoothing();
        }

        if (layer_element_smoothing->GetMappingMode() == FbxLayerElement::eByEdge)
        {
            is_smoothing_avaliable = true;
        }

        
        smoothing_mapping_mode = layer_element_smoothing->GetMappingMode();
        smoothing_reference_mode = layer_element_smoothing->GetReferenceMode();
    }

    if (!is_smoothing_avaliable)
    {
        //不是maya或者是max导出的，有可能是obj模型~
        fbx_geometry_converter_->ComputeEdgeSmoothingFromNormals(fbx_mesh);
        layer_element_smoothing = base_layer->GetSmoothing();
        smoothing_reference_mode = FbxLayerElement::eDirect;
        smoothing_mapping_mode = FbxLayerElement::eByEdge;
        if (layer_element_smoothing)
        {
            if (layer_element_smoothing->GetMappingMode() == FbxLayerElement::eByPolygon)
            {
                // convert the base layer to edge smoothing
                fbx_geometry_converter_->ComputeEdgeSmoothingFromPolygonSmoothing(fbx_mesh, 0);
                base_layer = fbx_mesh->GetLayer(0);
                layer_element_smoothing = base_layer->GetSmoothing();
            }

            if (layer_element_smoothing->GetMappingMode() == FbxLayerElement::eByEdge)
            {
                is_smoothing_avaliable = true;
            }


            smoothing_mapping_mode = layer_element_smoothing->GetMappingMode();
            smoothing_reference_mode = layer_element_smoothing->GetReferenceMode();
        }
    }

    // get the first vertex color layer
    FbxLayerElementVertexColor* vertex_color = base_layer->GetVertexColors();
    FbxLayerElement::EReferenceMode vertex_color_reference_mode(FbxLayerElement::eDirect);
    FbxLayerElement::EMappingMode vertex_color_mapping_mode(FbxLayerElement::eByControlPoint);
    if (vertex_color)
    {
        vertex_color_reference_mode = vertex_color->GetReferenceMode();
        vertex_color_mapping_mode = vertex_color->GetMappingMode();
    }

    //get normal layer
    FbxLayerElementNormal* normal_layer = base_layer->GetNormals();
    FbxLayerElementTangent* tangent_layer = base_layer->GetTangents();
    FbxLayerElementBinormal* binormal_layer = base_layer->GetBinormals();
    bool has_NTB_information = normal_layer && tangent_layer && binormal_layer;

    FbxLayerElement::EReferenceMode normal_reference_mode(FbxLayerElement::eDirect);
    FbxLayerElement::EMappingMode normal_mapping_mode(FbxLayerElement::eByControlPoint);
    if (normal_layer)
    {
        normal_reference_mode = normal_layer->GetReferenceMode();
        normal_mapping_mode = normal_layer->GetMappingMode();
    }
    FbxLayerElement::EReferenceMode tangent_reference_mode(FbxLayerElement::eDirect);
    FbxLayerElement::EMappingMode tangent_mapping_mode(FbxLayerElement::eByControlPoint);
    if (tangent_layer)
    {
        tangent_reference_mode = tangent_layer->GetReferenceMode();
        tangent_mapping_mode = tangent_layer->GetMappingMode();
    }

    FbxLayerElement::EReferenceMode binormal_reference_mode(FbxLayerElement::eDirect);
    FbxLayerElement::EMappingMode binormal_mapping_mode(FbxLayerElement::eByControlPoint);
    if (binormal_layer)
    {
        binormal_reference_mode = binormal_layer->GetReferenceMode();
        binormal_mapping_mode = binormal_layer->GetMappingMode();
    }

    bool has_degenerated_polygons = false;
    // Construct the matrices for the conversion from right handed to left handed system
    FbxAMatrix total_matrix;
    FbxAMatrix total_matrix_for_normal;
    total_matrix = ComputeTotalMatrix(node);
    total_matrix_for_normal = total_matrix.Inverse();
    total_matrix_for_normal = total_matrix_for_normal.Transpose();

    const int polygon_count = fbx_mesh->GetPolygonCount();
    if (polygon_count == 0)
    {
        ERROR_INFO("No polygon were found on mesh ", fbx_mesh->GetName());
        return false;
    }

    const int vertex_count = fbx_mesh->GetControlPointsCount();
    const bool odd_negative_scale = IsOddNegativeScale(total_matrix);
    
    //保存contorl points
    for (int vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
    {
        FbxVector4 fbx_position = fbx_mesh->GetControlPoints()[vertex_index];
        fbx_position = total_matrix.MultT(fbx_position);
        const YVector vertex_position = converter_.ConvertPos(fbx_position);
        YMeshControlPoint new_mesh_vertex;
        new_mesh_vertex.position = vertex_position;
        raw_mesh.control_points.push_back(new_mesh_vertex);
        if (raw_mesh.control_points.size() != (vertex_index + 1))
        {
            ERROR_INFO("Cannot create valid vertex for mesh ", fbx_mesh->GetName());
            return false;
        }
    }
    //speed up structure
    std::unordered_map<uint64_t, int> control_point_to_fbx_edge; //internal
    fbx_mesh->BeginGetMeshEdgeVertices();
    int fbx_edge_count = fbx_mesh->GetMeshEdgeCount();
    for (int fbx_edge_index = 0; fbx_edge_index < fbx_edge_count; ++fbx_edge_index)
    {
        int edge_start_control_point_index = -1;
        int edge_end_control_point_index = -1;
        fbx_mesh->GetMeshEdgeVertices(fbx_edge_index, edge_start_control_point_index, edge_end_control_point_index);
        uint64_t compacted_key = static_cast<uint64_t>(edge_start_control_point_index) << 32 | static_cast<uint64_t>(edge_end_control_point_index);
        control_point_to_fbx_edge[compacted_key] = fbx_edge_index;
        uint64_t compacted_key_reverse = static_cast<uint64_t>(edge_end_control_point_index) << 32 | static_cast<uint64_t>(edge_start_control_point_index);
        control_point_to_fbx_edge[compacted_key_reverse] = fbx_edge_index;
    }
    fbx_mesh->EndGetMeshEdgeVertices();

    // Compute and reserve memory to be used for vertex instances
    {
        int total_vertex_count = 0;
        for (int polygon_index = 0; polygon_index < polygon_count; ++polygon_index)
        {
            total_vertex_count += fbx_mesh->GetPolygonSize(polygon_index);
        }
        assert(total_vertex_count == polygon_count * 3);
        raw_mesh.polygons.reserve(polygon_count);
        raw_mesh.wedges.reserve(total_vertex_count);
        raw_mesh.edges.reserve(total_vertex_count);

        bool  bBeginGetMeshEdgeIndexForPolygonCalled = false;
        bool  bBeginGetMeshEdgeIndexForPolygonRequired = true;
        int current_wedge_index = 0;
        int skipped_wedges = 0;
        // keep those for all iterations to avoid heap allocations
        std::vector<int> wedges;
        std::vector<int> control_point_ids;
        std::vector<YVector> p;
        std::unordered_map<int, int> material_to_polygon_group;
        for (int polygon_index = 0; polygon_index < polygon_count; ++polygon_index)
        {
            int polygon_vertex_count_3 = fbx_mesh->GetPolygonSize(polygon_index);
            assert(polygon_vertex_count_3 == 3);
            //Verify if the polygon is degenerate, in this case do not add them
            {
                float comparision_threshold = (float)import_param_->remove_degenerate_triangles ? SMALL_NUMBER : 0.f;
                p.clear();
                p.resize(polygon_vertex_count_3);
                int control_point_ids[3];
                for (int corner_index = 0; corner_index < polygon_vertex_count_3; ++corner_index)
                {
                    const int control_point_index = fbx_mesh->GetPolygonVertex(polygon_index, corner_index);
                    control_point_ids[corner_index] = control_point_index;
                    const int vertex_id = control_point_index;
                    p[corner_index] = raw_mesh.control_points[vertex_id].position;
                }
                const YVector normal = (p[1] - p[2]) ^ (p[0] - p[2]).GetSafeNormal(comparision_threshold);
                if (normal.IsNearlyZero(comparision_threshold) || normal.ContainsNaN())
                {
                    has_degenerated_polygons = true;
                    skipped_wedges += polygon_vertex_count_3;
                    WARNING_INFO(fbx_mesh->GetName(), " has degenerate traingle, control point id is ", control_point_ids[0], "  ,", control_point_ids[1], "  ,", control_point_ids[2]);
                    continue;
                }
                const float triagnle_comparsion_threshold = (float)import_param_->remove_degenerate_triangles ? THRESH_POINTS_ARE_SAME : 0.f;
                if ((p[0].Equals(p[1], triagnle_comparsion_threshold)
                    || p[0].Equals(p[2], triagnle_comparsion_threshold)
                    || p[1].Equals(p[2], triagnle_comparsion_threshold)))
                {
                    WARNING_INFO(fbx_mesh->GetName(), " has degenerate traingle, control point id is ", control_point_ids[0], "  ,", control_point_ids[1], "  ,", control_point_ids[2]);
                    skipped_wedges += polygon_vertex_count_3;
                    continue;
                }

            }

            int real_polygon_index = polygon_index;
            wedges.clear();
            wedges.resize(polygon_vertex_count_3);
            control_point_ids.clear();
            control_point_ids.resize(polygon_vertex_count_3);
            for (int triangle_cornor_index_0_1_2 = 0; triangle_cornor_index_0_1_2 < polygon_vertex_count_3; ++triangle_cornor_index_0_1_2, ++current_wedge_index)
            {
                int odd_neative_sacle_mapping[3] = { 2, 1, 0 };
                int polygon_vertex_index = skipped_wedges + current_wedge_index;
                int correct_triangle_cornor_index = triangle_cornor_index_0_1_2;
                if (odd_negative_scale) 
                {
                    if (triangle_cornor_index_0_1_2 == 0)
                    {
                        polygon_vertex_index += 2;
                    }
                    else if (triangle_cornor_index_0_1_2 == 2)
                    {
                        polygon_vertex_index -= 2;
                    }
                    correct_triangle_cornor_index = odd_neative_sacle_mapping[triangle_cornor_index_0_1_2];
                    
                }

                wedges[triangle_cornor_index_0_1_2] = current_wedge_index;
                const int control_point_index = fbx_mesh->GetPolygonVertex(polygon_index, correct_triangle_cornor_index);
                control_point_ids[triangle_cornor_index_0_1_2] = control_point_index;
                YMeshWedge new_wedge;
                new_wedge.control_point_id = control_point_index;
                new_wedge.position = raw_mesh.control_points[control_point_index].position;;
                raw_mesh.control_points[control_point_index].AddWedge(current_wedge_index);
                raw_mesh.wedges.push_back(new_wedge);
                if ((current_wedge_index + 1) != raw_mesh.wedges.size())
                {
                    ERROR_INFO("Cannot create valid vertex instance for mesh ", fbx_mesh->GetName());
                    return false;
                }
                YMeshWedge& wedge = raw_mesh.wedges[current_wedge_index];

                //uv
                wedge.uvs.resize(fbx_uvs.unique_count, YVector2(0,0));
                for (int uv_layer_index = 0; uv_layer_index < fbx_uvs.unique_count; ++uv_layer_index)
                {
                    YVector2 final_uv_vector(0.0, 0.0);
                    if (fbx_uvs.layer_element_uv[uv_layer_index] != nullptr)
                    {
                        int uv_map_index = fbx_uvs.uv_mapping_mode[uv_layer_index] == FbxLayerElement::eByControlPoint ? control_point_index : polygon_vertex_index;
                        int uv_index = fbx_uvs.uv_reference_mode[uv_layer_index] == FbxLayerElement::eDirect ? uv_map_index : fbx_uvs.layer_element_uv[uv_layer_index]->GetIndexArray().GetAt(uv_map_index);
                        FbxVector2 uv_vector = fbx_uvs.layer_element_uv[uv_layer_index]->GetDirectArray().GetAt(uv_index);
                        int uv_counts = fbx_uvs.layer_element_uv[uv_layer_index]->GetDirectArray().GetCount();
                        final_uv_vector.x = static_cast<float>(uv_vector[0]);
                        final_uv_vector.y = 1.f - static_cast<float>(uv_vector[1]);   //flip the Y of UVs for DirectX
                    }
                    wedge.uvs[uv_layer_index] = final_uv_vector;
                }

                // color 
                if (vertex_color)
                {
                    int vertex_color_mapping_index = vertex_color_reference_mode == FbxLayerElement::eByControlPoint ? control_point_index : polygon_vertex_index;
                    int vertex_color_index = vertex_color_reference_mode == FbxLayerElement::eDirect ? vertex_color_mapping_index : vertex_color->GetIndexArray().GetAt(vertex_color_mapping_index);
                    FbxColor vertex_color_value = vertex_color->GetDirectArray().GetAt(vertex_color_index);
                    wedge.color = YVector4((float)vertex_color_value.mRed, (float)vertex_color_value.mGreen, (float)vertex_color_value.mBlue, (float)vertex_color_value.mAlpha);
                }
                else
                {
                    wedge.color = YVector4((float)0.0, (float)0.0, (float)0.0, (float)0.0);
                }

                // normal
                if (normal_layer)
                {
                    //normals may have different reference and mapping mode than tangents and binormals
                    int normal_map_index = (normal_mapping_mode == FbxLayerElement::eByControlPoint) ? control_point_index : polygon_vertex_index;
                    int normal_value_index = (normal_reference_mode == FbxLayerElement::eDirect) ? normal_map_index : normal_layer->GetIndexArray().GetAt(normal_map_index);

                    FbxVector4 temp_value = normal_layer->GetDirectArray().GetAt(normal_value_index);
                    temp_value = total_matrix_for_normal.MultT(temp_value);
                    YVector tangent_z = converter_.ConvertDir(temp_value);
                    wedge.normal = tangent_z.GetSafeNormal();

                    if (has_NTB_information)
                    {
                        int tangent_map_index = (tangent_mapping_mode == FbxLayerElement::eByControlPoint) ? control_point_index : polygon_vertex_index;
                        int tangent_value_index = (tangent_reference_mode == FbxLayerElement::eDirect) ? tangent_map_index : tangent_layer->GetIndexArray().GetAt(tangent_map_index);
                        FbxVector4 tangent_value = tangent_layer->GetDirectArray().GetAt(tangent_value_index);
                        tangent_value = total_matrix_for_normal.MultT(tangent_value);
                        YVector tangent_x = converter_.ConvertDir(tangent_value);
                        wedge.tangent = tangent_x.GetSafeNormal();

                        int binormal_map_index = (binormal_mapping_mode == FbxLayerElement::eByControlPoint) ? control_point_index : polygon_vertex_index;
                        int binormal_value_index = (binormal_reference_mode == FbxLayerElement::eDirect) ? binormal_map_index : binormal_layer->GetIndexArray().GetAt(binormal_map_index);
                        FbxVector4 binormal_value = binormal_layer->GetDirectArray().GetAt(binormal_value_index);
                        binormal_value = total_matrix_for_normal.MultT(binormal_value);
                        // 保持手性
                        YVector tanget_y = -converter_.ConvertDir(binormal_value);
                        wedge.bitangent = tanget_y.GetSafeNormal();
                        wedge.binormal_sign = YMath::GetBasisDeterminantSign(tangent_x, tanget_y, tangent_z);
                    }
                }
            }
           
            //material index
            int material_index = 0;
            if (material_count > 0)
            {
                if (layer_element_material)
                {
                    switch (material_mapping_model)
                    {
                    case FbxLayerElement::eAllSame:
                    {
                        material_index = layer_element_material->GetIndexArray().GetAt(0);
                    }
                    break;

                    case FbxLayerElement::eByPolygon:
                    {
                        material_index = layer_element_material->GetIndexArray().GetAt(polygon_index);
                        break;
                    }
                    }
                }
            }
            if (material_index >= material_count || material_index < 0)
            {
                ERROR_INFO("Face material index inconsistency - forcing to 0");
                material_index = 0;
            }
            //Create a polygon with the 3 vertex instances Add it to the material group
            if (!material_to_polygon_group.count(material_index))
            {
                std::shared_ptr<YImportedMaterial> material = materials[material_index];
                assert(material);
                YMeshPolygonGroup tmp_group;
                int new_polygon_group_id = (int)raw_mesh.polygon_groups.size();
                raw_mesh.polygon_groups.push_back(tmp_group);
                raw_mesh.polygon_group_to_material.push_back( material);
                material_to_polygon_group[material_index] = new_polygon_group_id;
            }
            int polygon_group_id = material_to_polygon_group[material_index];

            // create polygon edeges
            {
                // add the deges of this polygon
                for (uint32_t polygon_edge_number = 0; polygon_edge_number < (uint32_t)polygon_vertex_count_3; ++polygon_edge_number)
                {
                    // find the marching edeg id
                    uint32_t corner_indices[2];
                    corner_indices[0] = (polygon_edge_number + 0) % polygon_vertex_count_3;
                    corner_indices[1] = (polygon_edge_number + 1) % polygon_vertex_count_3;

                    int control_points[2];
                    control_points[0] = control_point_ids[corner_indices[0]];
                    control_points[1] = control_point_ids[corner_indices[1]];
                    int match_edge_id = raw_mesh.GetVertexPairEdge(control_points[0], control_points[1]);
                    if (match_edge_id == INVALID_ID)
                    {
                        match_edge_id = raw_mesh.CreateEdge(control_points[0], control_points[1]);
                    }
                    //RawMesh do not have edges, so by ordering the edge with the triangle construction we can ensure back and forth conversion with RawMesh
                    //When raw mesh will be completely remove we can create the edges right after the vertex creation.
                    int fbx_edge_index = INVALID_ID;
                    uint64_t compacted_key = ((uint64_t)control_points[0]) << 32 | ((uint64_t)control_points[1]);
                    if (control_point_to_fbx_edge.count(compacted_key))
                    {
                        fbx_edge_index = control_point_to_fbx_edge[compacted_key];
                    }
                    else
                    {
                        // Call BeginGetMeshEdgeIndexForPolygon lazily only if we enter the code path calling GetMeshEdgeIndexForPolygon
                        if (bBeginGetMeshEdgeIndexForPolygonRequired)
                        {
                            //Call this before all GetMeshEdgeIndexForPolygon for optimization purpose.
                            //But do not spend time precomputing stuff if the mesh has no edge since 
                            //GetMeshEdgeIndexForPolygon is going to always returns -1 anyway without going into the slow path.
                            if (fbx_mesh->GetMeshEdgeCount() > 0)
                            {
                                fbx_mesh->BeginGetMeshEdgeIndexForPolygon();
                                bBeginGetMeshEdgeIndexForPolygonCalled = true;
                            }
                            bBeginGetMeshEdgeIndexForPolygonRequired = false;
                        }
                        int correct_polygon_edge_number = polygon_edge_number;
                        if (odd_negative_scale)
                        {
                            int odd_neative_sacle_mapping[3] = { 2, 1, 0 };
                            correct_polygon_edge_number = odd_neative_sacle_mapping[polygon_edge_number];
                        }
                        fbx_edge_index = fbx_mesh->GetMeshEdgeIndexForPolygon(polygon_index, correct_polygon_edge_number);
                    }
                    if (fbx_edge_index == INVALID_ID)
                    {
                        
                    }
                    float edge_crease = (float)fbx_mesh->GetEdgeCreaseInfo(fbx_edge_index);
                    YMeshEdge& cur_edge = raw_mesh.edges[match_edge_id];
                    cur_edge.edge_crease_sharpness = edge_crease;
                    if (!cur_edge.edge_hardness)
                    {
                        if (is_smoothing_avaliable && layer_element_smoothing)
                        {
                            if (smoothing_mapping_mode == FbxLayerElement::eByEdge)
                            {
                                int smoothing_index = (smoothing_reference_mode == FbxLayerElement::eDirect) ? fbx_edge_index : layer_element_smoothing->GetIndexArray().GetAt(fbx_edge_index);
                                cur_edge.edge_hardness = (layer_element_smoothing->GetDirectArray().GetAt(smoothing_index) == 0);
                            }
                            else
                            {
                                WARNING_INFO("Unsupported Smoothing group mapping mode on mesh ", fbx_mesh->GetName());
                            }
                        }
                        else
                        {
                            cur_edge.edge_hardness = true;
                        }
                    }
                }
            }
            std::vector<int> created_edges;
            raw_mesh.CreatePolygon(polygon_group_id, wedges, created_edges);
            assert(created_edges.size() == 0);
        }
        if (bBeginGetMeshEdgeIndexForPolygonCalled)
        {
            fbx_mesh->EndGetMeshEdgeIndexForPolygon();
        }
        if (skipped_wedges > 0)
        {
            raw_mesh.CompressControlPoint();
        }
    }
    if (has_degenerated_polygons)
    {
        WARNING_INFO(fbx_mesh->GetName(), "has degenerate triangle");
        
    }

    raw_mesh.CompressMaterials();
    bool bIsValidMesh = raw_mesh.Valid();

    return bIsValidMesh;
}

bool YFbxImporter::BuildStaicMesh(YLODMesh* raw_mesh, std::vector<std::shared_ptr< ImportedRawMesh>>& raw_meshes)
{
    if (raw_meshes.empty())
    {
        return false;
    }
    std::shared_ptr<ImportedRawMesh> new_copyed_mesh = std::make_shared<ImportedRawMesh>();
    *new_copyed_mesh = *raw_meshes[0];
    LOG_INFO("Bengin merge scene");
    int all_control_point_size = 0;
    int all_wedge_size = 0;
    int all_polygon_size = 0;
    int all_edge_size = 0;
    int all_polygon_group_size = 0;

    for (std::shared_ptr<ImportedRawMesh>& impored_raw_mesh : raw_meshes)
    {
        all_control_point_size += (int)impored_raw_mesh->control_points.size();
        all_wedge_size += (int)impored_raw_mesh->wedges.size();
        all_polygon_size += (int)impored_raw_mesh->polygons.size();
        all_edge_size += (int)impored_raw_mesh->edges.size();
        all_polygon_group_size += (int)impored_raw_mesh->polygon_groups.size();
    }

    new_copyed_mesh->control_points.reserve(all_control_point_size);
    new_copyed_mesh->wedges.reserve(all_wedge_size);
    new_copyed_mesh->polygons.reserve(all_polygon_size);
    new_copyed_mesh->edges.reserve(all_edge_size);
    new_copyed_mesh->polygon_groups.reserve(all_polygon_group_size);
    new_copyed_mesh->polygon_group_to_material.reserve(all_polygon_group_size);


    for (int mesh_index = 1; mesh_index < (int)raw_meshes.size(); ++mesh_index)
    {
        new_copyed_mesh->Merge(*raw_meshes[mesh_index]);
    }
    LOG_INFO("End merge scene");
    //new_copyed_mesh->ComputeTriangleNormalAndTangent(Caculate, Mikkt);
    new_copyed_mesh->ComputeWedgeNormalAndTangent(ImportNormal, Mikkt, import_param_->remove_degenerate_triangles);
    new_copyed_mesh->GenerateLightMapUV();
    raw_mesh->vertex_position = new_copyed_mesh->control_points;
    raw_mesh->vertex_instances = new_copyed_mesh->wedges;
    raw_mesh->polygons = new_copyed_mesh->polygons;
    raw_mesh->edges = new_copyed_mesh->edges;
    raw_mesh->polygon_groups = new_copyed_mesh->polygon_groups;
    //raw_mesh->polygon_group_to_material = new_copyed_mesh->polygon_group_to_material;
#if 0
    raw_mesh->vertex_position = raw_meshes[0]->control_points;
    raw_mesh->vertex_instances = raw_meshes[0]->wedges;
    raw_mesh->polygons = raw_meshes[0]->polygons;
    raw_mesh->edges = raw_meshes[0]->edges;
    raw_mesh->polygon_groups = raw_meshes[0]->polygon_groups;
    raw_mesh->polygon_group_to_material = raw_meshes[0]->polygon_group_to_material;

    for (int mesh_index = 1; mesh_index < (int)raw_meshes.size(); ++mesh_index)
    {
       int  control_points_size = (int)raw_mesh->vertex_position.size();
       int wedge_size = (int)raw_mesh->vertex_instances.size();
       int pogygone_size = (int)raw_mesh->polygons.size();
       int polygon_group_size = raw_mesh->polygon_groups.size();
        
       ImportedRawMesh& import_raw_mesh = *raw_meshes[mesh_index];
       raw_mesh->vertex_position.insert(raw_mesh->vertex_position.end(),import_raw_mesh.control_points.begin(), import_raw_mesh.control_points.end());
       for (YMeshVertexWedge& wedge : import_raw_mesh.wedges)
       {
           wedge.control_point_id += control_points_size;
       }
       raw_mesh->vertex_instances.insert(raw_mesh->vertex_instances.end(), import_raw_mesh.wedges.begin(), import_raw_mesh.wedges.end());
       
       for (YMeshPolygon& polygon : import_raw_mesh.polygons)
       {
           polygon.wedge_ids[0] += wedge_size;
           polygon.wedge_ids[1] += wedge_size;
           polygon.wedge_ids[2] += wedge_size;
       }
       raw_mesh->polygons.insert(raw_mesh->polygons.end(), import_raw_mesh.polygons.begin(), import_raw_mesh.polygons.end());

       for (YMeshPolygonGroup& group : import_raw_mesh.polygon_groups)
       {
           for (int& polygon_index : group.polygons)
           {
               polygon_index += pogygone_size;
            }
       }

       raw_mesh->polygon_groups.insert(raw_mesh->polygon_groups.end(), import_raw_mesh.polygon_groups.begin(), import_raw_mesh.polygon_groups.end());
    }
   /* return true;


    std::vector<std::shared_ptr<YImportedMaterial>> all_materials;
    for (int mesh_index = 0; mesh_index < (int)raw_meshes.size(); ++mesh_index)
    {
        const ImportedRawMesh& raw_mesh = *raw_meshes[mesh_index];
        for (auto& item : raw_mesh.polygon_group_to_material)
        {
            std::shared_ptr<YImportedMaterial> material = item.second;
        }
    }*/
#endif
    return true;
}

bool YFbxImporter::BuildStaicMeshRenderData(YStaticMesh* static_mesh, std::vector<std::shared_ptr< ImportedRawMesh>>& raw_meshes)
{
    if (raw_meshes.empty())
    {
        return false;
    }
    std::shared_ptr<ImportedRawMesh> new_copyed_mesh = std::make_shared<ImportedRawMesh>();
    *new_copyed_mesh = *raw_meshes[0];
    LOG_INFO("Bengin merge scene");
    int all_control_point_size = 0;
    int all_wedge_size = 0;
    int all_polygon_size = 0;
    int all_edge_size = 0;
    int all_polygon_group_size = 0;

    for (std::shared_ptr<ImportedRawMesh>& impored_raw_mesh : raw_meshes)
    {
        all_control_point_size += impored_raw_mesh->control_points.size();
        all_wedge_size += impored_raw_mesh->wedges.size();
        all_polygon_size += impored_raw_mesh->polygons.size();
        all_edge_size += impored_raw_mesh->edges.size();
        all_polygon_group_size += impored_raw_mesh->polygon_groups.size();
    }

    new_copyed_mesh->control_points.reserve(all_control_point_size);
    new_copyed_mesh->wedges.reserve(all_wedge_size);
    new_copyed_mesh->polygons.reserve(all_polygon_size);
    new_copyed_mesh->edges.reserve(all_edge_size);
    new_copyed_mesh->polygon_groups.reserve(all_polygon_group_size);
    new_copyed_mesh->polygon_group_to_material.reserve(all_polygon_group_size);


    for (int mesh_index = 1; mesh_index < (int)raw_meshes.size(); ++mesh_index)
    {
        new_copyed_mesh->Merge(*raw_meshes[mesh_index]);
    }
    LOG_INFO("End merge scene");
    new_copyed_mesh->ComputeWedgeNormalAndTangent(ImportNormal, Mikkt, import_param_->remove_degenerate_triangles);
    
    PostProcessRenderMesh process(new_copyed_mesh.get());
    //static_mesh->lod_render_data_.push_back(process.GenerateHiStaticVertexData());
    static_mesh->lod_render_data_.push_back(process.GenerateMediumStaticVertexData());
    return true;
}

