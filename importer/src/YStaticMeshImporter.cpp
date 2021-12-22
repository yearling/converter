#include <cassert>
#include "fbxsdk/scene/geometry/fbxlayer.h"
#include "YFbxImporter.h"
#include "Engine/YRawMesh.h"
#include "Math/YMath.h"
#include "YFbxUtility.h"
#include "Engine/YMaterial.h"
#include "Engine/YLog.h"

bool YFbxImporter::BuildStaticMeshFromGeometry(FbxNode* node, YLODMesh* raw_mesh, std::vector<YFbxMaterial*>& existing_materials)
{
	FbxMesh* fbx_mesh = node->GetMesh();
	std::string node_name = node->GetName();
	FbxUVs fbx_uvs(this, fbx_mesh);

	//create material

	int material_count = 0;
	int material_index_offset = (int)existing_materials.size();
	std::vector<YFbxMaterial*> materials;
	FindOrImportMaterialsFromNode(node, materials, fbx_uvs.uv_set);
	material_count = node->GetMaterialCount();
	assert(material_count == materials.size());
	for (int material_index = 0; material_index < material_count; ++material_index)
	{
		existing_materials.push_back(materials[material_index]);
	}
	if (material_count == 0)
	{
		existing_materials.push_back(new YFbxMaterial());
		material_count = 1;
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
	//	get the "material index" layer.  Do this AFTER the triangulation step as that may reorder material indices

	FbxLayerElementMaterial* layer_element_material = base_layer->GetMaterials();
	FbxLayerElement::EMappingMode material_mapping_model = layer_element_material ? layer_element_material->GetMappingMode() : FbxLayerElement::eByPolygon;

	fbx_uvs.Phase2(this, fbx_mesh);

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

	bool has_no_degenerated_polygons = false;
	// Construct the matrices for the conversion from right handed to left handed system
	FbxAMatrix total_matrix;
	FbxAMatrix total_matrix_for_normal;
	total_matrix = ComputeTotalMatrix(node);
	total_matrix_for_normal = total_matrix.Inverse();
	total_matrix_for_normal = total_matrix_for_normal.Transpose();

	int polygon_count = fbx_mesh->GetPolygonCount();
	if (polygon_count == 0)
	{
		ERROR_INFO("No polygon were found on mesh ", fbx_mesh->GetName());
		return false;
	}

	int vertex_count = fbx_mesh->GetControlPointsCount();
	bool odd_negative_scale = IsOddNegativeScale(total_matrix);

	int vertex_offset = (int)raw_mesh->vertex_position.size();
	int vertex_instance_offset = (int)raw_mesh->vertex_instances.size();
	int polygon_offset = (int)raw_mesh->polygons.size();
	std::unordered_map<int, int> polygon_group_mapping;
	// When importing multiple mesh pieces to the same static mesh.  Ensure each mesh piece has the same number of Uv's
	//int existing_UV_count = lod_mesh->vertex_instance_uvs.size();
	//int uv_num = YMath::Max(existing_UV_count, fbx_uvs.unique_count);
	//uv_num = YMath::Min(1, uv_num);
	//lod_mesh->vertex_instance_uvs.resize(uv_num);


	for (int vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
	{
		int real_vertex_index = vertex_offset + vertex_index;
		FbxVector4 fbx_position = fbx_mesh->GetControlPoints()[vertex_index];
		fbx_position = total_matrix.MultT(fbx_position);
		const YVector vertex_position = converter_.ConvertPos(fbx_position);
		YMeshVertex new_mesh_vertex;
		new_mesh_vertex.position = vertex_position;
		raw_mesh->vertex_position.push_back(new_mesh_vertex);
		if (raw_mesh->vertex_position.size() != (real_vertex_index + 1))
		{
			ERROR_INFO("Cannot create valid vertex for mesh ", fbx_mesh->GetName());
			return false;
		}
	}

	std::unordered_map<uint64_t, int> remap_edge_id;
	fbx_mesh->BeginGetMeshEdgeVertices();
	int fbx_edge_count = fbx_mesh->GetMeshEdgeCount();
	for (int fbx_edge_index = 0; fbx_edge_index < fbx_edge_count; ++fbx_edge_index)
	{
		int edge_start_vertex_index = -1;
		int edge_end_vertex_index = -1;
		fbx_mesh->GetMeshEdgeVertices(fbx_edge_index, edge_start_vertex_index, edge_end_vertex_index);
		int edge_vertex_start = vertex_offset + edge_start_vertex_index;
		int edge_vertex_end = vertex_offset + edge_end_vertex_index;
		uint64_t compacted_key = static_cast<uint64_t>(edge_vertex_start) << 32 | static_cast<uint64_t>(edge_vertex_end);
		remap_edge_id[compacted_key] = fbx_edge_index;
		uint64_t compacted_key_reverse = static_cast<uint64_t>(edge_vertex_end) << 32 | static_cast<uint64_t>(edge_vertex_start);
		remap_edge_id[compacted_key_reverse] = fbx_edge_index;
	}
	fbx_mesh->EndGetMeshEdgeVertices();

	// Compute and reserve memory to be used for vertex instances
	{
		int total_vertex_count = 0;
		for (int polygon_index = 0; polygon_index < polygon_count; ++polygon_index)
		{
			total_vertex_count += fbx_mesh->GetPolygonSize(polygon_index);
		}
		raw_mesh->polygons.reserve(raw_mesh->polygons.size() + polygon_count);
		raw_mesh->vertex_instances.reserve(raw_mesh->vertex_instances.size() + total_vertex_count);
		raw_mesh->edges.reserve(raw_mesh->edges.size() + total_vertex_count);

		bool  bBeginGetMeshEdgeIndexForPolygonCalled = false;
		bool  bBeginGetMeshEdgeIndexForPolygonRequired = true;
		int current_vertex_instace_index = 0;
		int skipped_vertex_instance = 0;
		// keep those for all iterations to avoid heap allocations
		std::vector<int> corner_instance_ids;
		std::vector<int> corner_vertices_ids;
		std::vector<YVector> P;

		for (int polygon_index = 0; polygon_index < polygon_count; ++polygon_index)
		{
			int polygon_vertex_count = fbx_mesh->GetPolygonSize(polygon_index);
			//Verify if the polygon is degenerate, in this case do not add them
			{
				float comparision_threshold = (float)import_param_->remove_degenerate_triangles ? SMALL_NUMBER : 0.f;
				P.clear();
				P.resize(polygon_vertex_count);
				for (int corner_index = 0; corner_index < polygon_vertex_count; ++corner_index)
				{
					const int control_point_index = fbx_mesh->GetPolygonVertex(polygon_index, corner_index);
					const int vertex_id = vertex_offset + control_point_index;
					P[corner_index] = raw_mesh->vertex_position[vertex_id].position;
				}
				const YVector normal = (P[1] - P[2]) ^ (P[0] - P[2]).GetSafeNormal(comparision_threshold);
				if (normal.IsNearlyZero(comparision_threshold) || normal.ContainsNaN())
				{
					skipped_vertex_instance += polygon_vertex_count;
					continue;
				}

			}

			int real_polygon_index = polygon_offset + polygon_index;
			corner_instance_ids.clear();
			corner_instance_ids.resize(polygon_vertex_count);
			corner_vertices_ids.clear();
			corner_vertices_ids.resize(polygon_vertex_count);
			for (int corner_index = 0; corner_index < polygon_vertex_count; ++corner_index, ++current_vertex_instace_index)
			{
				int vertex_indstance_index = vertex_instance_offset + current_vertex_instace_index;
				int real_fbx_vertex_index = skipped_vertex_instance + current_vertex_instace_index;
				corner_instance_ids[corner_index] = vertex_indstance_index;
				const int control_point_index = fbx_mesh->GetPolygonVertex(polygon_index, corner_index);
				int vertex_id = vertex_offset + control_point_index;
				corner_vertices_ids[corner_index] = vertex_id;
				const YVector vertex_position = raw_mesh->vertex_position[vertex_id].position;
				YMeshVertexInstance new_vertex_instance;
				// vertex_ins 到 vertex的索引
				new_vertex_instance.vertex_id = vertex_id;
				// vertex 到 vertex_ins的索引，双向
				raw_mesh->vertex_position[vertex_id].AddVertexInstance(vertex_indstance_index);
				raw_mesh->vertex_instances.push_back(new_vertex_instance);
				if ((vertex_indstance_index + 1) != raw_mesh->vertex_instances.size())
				{
					ERROR_INFO("Cannot create valid vertex instance for mesh ", fbx_mesh->GetName());
					return false;
				}
				YMeshVertexInstance& cur_vertex_instance = raw_mesh->vertex_instances[vertex_indstance_index];

				//uv
				for (int uv_layer_index = 0; uv_layer_index < fbx_uvs.unique_count; ++uv_layer_index)
				{
					YVector2 final_uv_vector(0.0, 0.0);
					if (fbx_uvs.layer_element_uv[uv_layer_index] != nullptr)
					{
						int uv_map_index = fbx_uvs.uv_mapping_mode[uv_layer_index] == FbxLayerElement::eByControlPoint ? control_point_index : real_fbx_vertex_index;
						int uv_index = fbx_uvs.uv_reference_mode[uv_layer_index] == FbxLayerElement::eDirect ? uv_map_index : fbx_uvs.layer_element_uv[uv_layer_index]->GetIndexArray().GetAt(uv_map_index);
						FbxVector2 uv_vector = fbx_uvs.layer_element_uv[uv_layer_index]->GetDirectArray().GetAt(uv_index);
						final_uv_vector.x = static_cast<float>(uv_vector[0]);
						final_uv_vector.y = 1.f - static_cast<float>(uv_vector[1]);   //flip the Y of UVs for DirectX
					}
					cur_vertex_instance.vertex_instance_uvs[uv_layer_index] = final_uv_vector;
				}

				// color 
				if (vertex_color)
				{
					int vertex_color_mapping_index = vertex_color_reference_mode == FbxLayerElement::eByControlPoint ? control_point_index : real_fbx_vertex_index;
					int vertex_color_index = vertex_color_reference_mode == FbxLayerElement::eDirect ? vertex_color_mapping_index : vertex_color->GetIndexArray().GetAt(vertex_color_mapping_index);
					FbxColor vertex_color_value = vertex_color->GetDirectArray().GetAt(vertex_color_index);
					cur_vertex_instance.vertex_instance_color = YVector4((float)vertex_color_value.mRed, (float)vertex_color_value.mGreen, (float)vertex_color_value.mBlue, (float)vertex_color_value.mAlpha);
				}

				// normal
				if (normal_layer)
				{
					//normals may have different reference and mapping mode than tangents and binormals
					int normal_map_index = (normal_mapping_mode == FbxLayerElement::eByControlPoint) ? control_point_index : real_fbx_vertex_index;
					int normal_value_index = (normal_reference_mode == FbxLayerElement::eDirect) ? normal_map_index : normal_layer->GetIndexArray().GetAt(normal_map_index);

					FbxVector4 temp_value = normal_layer->GetDirectArray().GetAt(normal_value_index);
					temp_value = total_matrix_for_normal.MultT(temp_value);
					YVector tangent_z = converter_.ConvertDir(temp_value);
					cur_vertex_instance.vertex_instance_normal = tangent_z;

					if (has_NTB_information)
					{
						int tangent_map_index = (tangent_mapping_mode == FbxLayerElement::eByControlPoint) ? control_point_index : real_fbx_vertex_index;
						int tangent_value_index = (tangent_reference_mode == FbxLayerElement::eDirect) ? tangent_map_index : tangent_layer->GetIndexArray().GetAt(tangent_map_index);
						FbxVector4 tangent_value = tangent_layer->GetDirectArray().GetAt(tangent_value_index);
						tangent_value = total_matrix_for_normal.MultT(tangent_value);
						YVector tangent_x = converter_.ConvertDir(tangent_value);
						cur_vertex_instance.vertex_instance_tangent = tangent_x;

						int binormal_map_index = (binormal_mapping_mode == FbxLayerElement::eByControlPoint) ? control_point_index : real_fbx_vertex_index;
						int binormal_value_index = (binormal_reference_mode == FbxLayerElement::eDirect) ? binormal_map_index : binormal_layer->GetIndexArray().GetAt(binormal_map_index);
						FbxVector4 binormal_value = binormal_layer->GetDirectArray().GetAt(binormal_value_index);
						binormal_value = total_matrix_for_normal.MultT(binormal_value);
						// 保持手性
						YVector tanget_y = -converter_.ConvertDir(binormal_value);
						cur_vertex_instance.vertex_instance_binormal_sign = YMath::GetBasisDeterminantSign(tangent_x, tanget_y, tangent_z);
					}
				}
			}
			// Check if the polygon just discovered is non-degenerate if we haven't found one yet
			//TODO check all polygon vertex, not just the first 3 vertex
			if (!has_no_degenerated_polygons)
			{
				float triagnle_comparsion_threshold = (float)import_param_->remove_degenerate_triangles ? THRESH_POINTS_ARE_SAME : 0.f;
				YVector vertex_position[3];
				vertex_position[0] = raw_mesh->vertex_position[corner_vertices_ids[0]].position;
				vertex_position[1] = raw_mesh->vertex_position[corner_vertices_ids[1]].position;
				vertex_position[2] = raw_mesh->vertex_position[corner_vertices_ids[2]].position;

				if (!(vertex_position[0].Equals(vertex_position[1])
					|| vertex_position[0].Equals(vertex_position[2])
					|| vertex_position[1].Equals(vertex_position[2])))
				{
					has_no_degenerated_polygons = true;
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
			int real_material_index = material_index_offset + material_index;
			if (!polygon_group_mapping.count(real_material_index))
			{
				YFbxMaterial* material = existing_materials[real_material_index];
				std::string material_name = material->name;
				int existing_polygon_group_id = INVALID_ID;
				for (int group_id = 0; group_id < raw_mesh->polygon_groups.size(); ++group_id)
				{
					if (raw_mesh->polygon_group_imported_material_slot_name.count(group_id) && raw_mesh->polygon_group_imported_material_slot_name[group_id] == material_name)
					{
						existing_polygon_group_id = group_id;
						break;
					}
				}
				if (existing_polygon_group_id == INVALID_ID)
				{
					YMeshPolygonGroup tmp_group;
					existing_polygon_group_id = (int)raw_mesh->polygon_groups.size();
					raw_mesh->polygon_groups.push_back(tmp_group);
					raw_mesh->polygon_group_imported_material_slot_name[existing_polygon_group_id] = material_name;
				}
				polygon_group_mapping[real_material_index] = existing_polygon_group_id;
			}

			// create polygon edeges
			{
				// add the deges of this polygon
				for (uint32_t polygon_edge_number = 0; polygon_edge_number < (uint32_t)polygon_vertex_count; ++polygon_edge_number)
				{
					// find the marching edeg id
					uint32_t corner_indices[2];
					corner_indices[0] = (polygon_edge_number + 0) % polygon_vertex_count;
					corner_indices[1] = (polygon_edge_number + 1) % polygon_vertex_count;

					int edge_vertex_ids[2];
					edge_vertex_ids[0] = corner_vertices_ids[corner_indices[0]];
					edge_vertex_ids[1] = corner_vertices_ids[corner_indices[1]];
					int match_edge_id = raw_mesh->GetVertexPairEdge(edge_vertex_ids[0], edge_vertex_ids[1]);
					if (match_edge_id == INVALID_ID)
					{
						match_edge_id = raw_mesh->CreateEdge(edge_vertex_ids[0], edge_vertex_ids[1]);
					}
					//RawMesh do not have edges, so by ordering the edge with the triangle construction we can ensure back and forth conversion with RawMesh
					//When raw mesh will be completely remove we can create the edges right after the vertex creation.
					int fbx_edge_index = INVALID_ID;
					uint64_t compacted_key = ((uint64_t)edge_vertex_ids[0]) << 32 | ((uint64_t)edge_vertex_ids[1]);
					if (remap_edge_id.count(compacted_key))
					{
						fbx_edge_index = remap_edge_id[compacted_key];
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

						fbx_edge_index = fbx_mesh->GetMeshEdgeIndexForPolygon(polygon_index, polygon_edge_number);
					}
					float edge_crease = (float)fbx_mesh->GetEdgeCreaseInfo(fbx_edge_index);
					YMeshEdge& cur_edge = raw_mesh->edges[match_edge_id];
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
			int polygon_group_id = polygon_group_mapping[real_material_index];
			std::vector<int> created_edges;
			raw_mesh->CreatePolygon(polygon_group_id, corner_instance_ids, created_edges);
			assert(created_edges.size() == 0);
		}
		if (bBeginGetMeshEdgeIndexForPolygonCalled)
		{
			fbx_mesh->EndGetMeshEdgeIndexForPolygon();
		}
		if (skipped_vertex_instance > 0)
		{

		}


	}
	if (!has_no_degenerated_polygons)
	{
		WARNING_INFO(fbx_mesh->GetName(), "has degenerate triangle");
	}

	bool bIsValidMesh = has_no_degenerated_polygons;

	return bIsValidMesh;
}

