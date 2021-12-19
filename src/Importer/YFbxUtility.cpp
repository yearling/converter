#include <string>
#include "fbxsdk/scene/geometry/fbxlayer.h"
#include "fbxsdk/fileio/fbximporter.h"
#include "Importer/YFbxUtility.h"
#include "Math/YMath.h"
#include "Engine/YLog.h"

YVector FbxDataConverter::ConvertPos(const FbxVector4& vector)
{
	YVector result;
	result[0] = (float)vector[0];
	result[1] = (float)vector[1];
	result[2] = (float)-vector[2];
	return result;
}

YVector FbxDataConverter::ConvertDir(const FbxVector4& vector)
{
	YVector result;
	result[0] = (float)vector[0];
	result[1] = (float)vector[1];
	result[2] = (float)-vector[2];
	return result;
}

YRotator FbxDataConverter::ConvertEuler(const FbxVector4& vector)
{
	return	YRotator::MakeFromEuler(YVector((float)-vector[0], (float)-vector[1], (float)vector[2]));
}

YVector FbxDataConverter::ConvertScale(const FbxDouble3& vector)
{
	YVector result;
	result[0] = (float)vector[0];
	result[1] = (float)vector[1];
	result[2] = (float)vector[2];
	return result;
}

YVector FbxDataConverter::ConvertScale(const FbxVector4& vector)
{
	YVector result;
	result[0] = (float)vector[0];
	result[1] = (float)vector[1];
	result[2] = (float)vector[2];
	return result;
}

YRotator FbxDataConverter::ConvertRotation(const FbxQuaternion& quaternion)
{
	YRotator out(ConvertFbxQutaToQuat(quaternion));
	return out;
}

YQuat FbxDataConverter::ConvertFbxQutaToQuat(const FbxQuaternion& quaternion)
{
	YQuat quat;
	quat.x = (float)quaternion[0];
	quat.y = (float)quaternion[1];
	quat.z = (float)-quaternion[2];
	quat.w = (float)-quaternion[3];
	return quat;
}

YMatrix FbxDataConverter::ConvertFbxMatrix(const FbxAMatrix& matrix)
{
// 替换euler，us.pitch = - fbx.x , us.yaw = -fbx.y us.z = fbx.z
// 观察euler ,xyz轴向到矩阵变换，替换可得
// us.matrix[0][2] = - fbx.m[0][2]
// us.matrix[1][2] = -fbx.m[1][2]
// us.matrix[2][0] = -fbx.m[2][0]
// us.matrix[2][1] = -fbx.m[2][1]
// us.matrix[3][2] = - fbx.m[3][2]

	YMatrix us_matrix;
	us_matrix.m[0][0] = (float)matrix[0][0];
	us_matrix.m[0][1] = (float)matrix[0][1];
	us_matrix.m[0][2] = (float)- matrix[0][2];
	us_matrix.m[0][3] = (float)matrix[0][3];
	us_matrix.m[1][0] = (float)matrix[1][0];
	us_matrix.m[1][1] = (float)matrix[1][1];
	us_matrix.m[1][2] = (float)- matrix[1][2];
	us_matrix.m[1][3] = (float)matrix[1][3];
	us_matrix.m[2][0] = (float)- matrix[2][0];
	us_matrix.m[2][1] = (float)- matrix[2][1];
	us_matrix.m[2][2] = (float)matrix[2][2];
	us_matrix.m[2][3] = (float)matrix[2][3];
	us_matrix.m[3][0] = (float)matrix[3][0];
	us_matrix.m[3][1] = (float)matrix[3][1];
	us_matrix.m[3][2] = (float)- matrix[3][2];
	us_matrix.m[3][3] = (float)matrix[3][3];
	return us_matrix;
}

FbxVector4 FbxDataConverter::ConvertToFbxPos(const YVector& vector)
{
	FbxVector4 fbx_pos(vector.x, vector.y, vector.z);
	return fbx_pos;
}

FbxVector4 FbxDataConverter::ConvertToFbxRot(const YVector& vector)
{
	FbxVector4 out;
	out[0] = -vector[0];
	out[1] = -vector[1];
	out[2] = vector[2];

	return out;
}

FbxVector4 FbxDataConverter::ConvertToFbxScale(const YVector& vector)
{
	FbxVector4 out;
	out[0] = vector[0];
	out[1] = vector[1];
	out[2] = vector[2];

	return out;
}

FbxString FbxDataConverter::ConvertToFbxString(const std::string& str)
{
	FbxString out_string(str.c_str());
	return out_string;
}

FbxUVs::FbxUVs(YFbxImporter* importer, FbxMesh* mesh)
	:unique_count(0)
{
	//
	//	store the UVs in arrays for fast access in the later looping of triangles 
	//
	// mapping from UVSets to Fbx LayerElementUV
	// Fbx UVSets may be duplicated, remove the duplicated UVSets in the mapping 

	int layer_count = mesh->GetLayerCount();
	if (layer_count > 0)
	{
		for (int uv_layer_index = 0; uv_layer_index < layer_count; ++uv_layer_index)
		{
			FbxLayer* layer = mesh->GetLayer(uv_layer_index);
			int uv_set_count = layer->GetUVSetCount();
			if (uv_set_count)
			{
				FbxArray<FbxLayerElementUV const*> ele_uvs = layer->GetUVSets();
				for (int uv_set_index = 0; uv_set_index < uv_set_count; ++uv_set_index)
				{
					FbxLayerElementUV const* element_uv = ele_uvs[uv_set_index];
					if (element_uv)
					{
						std::string uv_set_name = element_uv->GetName();
						if (uv_set_name.empty())
						{
							uv_set_name = "UVmap_" + std::to_string(uv_layer_index);
						}
						if (std::find(uv_set.begin(), uv_set.end(), uv_set_name) == uv_set.end())
						{
							uv_set.push_back(uv_set_name);
						}
					}
				}
			}
		}
	}

}

void FbxUVs::Phase2(YFbxImporter* importer, FbxMesh* mesh)
{
//	store the UVs in arrays for fast access in the later looping of triangles
	unique_count = uv_set.size();
	if (unique_count > 0)
	{
		layer_element_uv.resize(unique_count);
		uv_reference_mode.resize(unique_count);
		uv_mapping_mode.resize(unique_count);
	}

	for (int uv_index = 0; uv_index < uv_set.size(); ++uv_index)
	{
		for (int uv_layer_index = 0; uv_layer_index < mesh->GetLayerCount(); ++uv_layer_index)
		{
			FbxLayer* layer = mesh->GetLayer(uv_layer_index);
			int uv_set_count = layer->GetUVSetCount();
			if (uv_set_count)
			{
				FbxArray<FbxLayerElementUV const*> ele_uvs = layer->GetUVSets();
				for (int uv_set_index = 0; uv_set_index < uv_set_count; ++uv_set_index)
				{
					FbxLayerElementUV const* element_uv = ele_uvs[uv_set_index];
					if (element_uv)
					{
						std::string uv_set_name = element_uv->GetName();
						if (uv_set_name.empty())
						{
							uv_set_name = "UVmap_" + std::to_string(uv_layer_index);
						}
						if (uv_set_name == uv_set[uv_index])
						{
							layer_element_uv[uv_index] = element_uv;
							uv_reference_mode[uv_index] = element_uv->GetReferenceMode();
							uv_mapping_mode[uv_index] = element_uv->GetMappingMode();
							break;
						}
					}
				}
			}
		}
	}
	if(unique_count> MAX_MESH_TEXTURE_COORDS)
	{
		WARNING_INFO("Reached the maximum number of UV Channels for a Static Mesh ", mesh->GetName());
	}
	unique_count = YMath::Min(unique_count, MAX_MESH_TEXTURE_COORDS);

}
