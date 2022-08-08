#pragma once
#include "Math/YMath.h"
#include <vector>
#include <unordered_map>
#include <set>
#include <string>
#include "fbxsdk.h"
#include "fbxsdk/core/math/fbxquaternion.h"
#include "fbxsdk/core/math/fbxaffinematrix.h"
#include "Math/YVector.h"
#include "Math/YQuaterion.h"
#include "Math/YRotator.h"
#include "Math/YMatrix.h"
#include "Engine/YMaterial.h"
#include "Math/YTransform.h"
#include "Engine/YRawMesh.h"
class FbxDataConverter
{
public:
	// fbx to our
	static YVector ConvertPos(const FbxVector4& vector);
	static YVector ConvertDir(const FbxVector4& vector);
	static YRotator ConvertEuler(const FbxVector4& vector);
	static YVector ConvertScale(const FbxDouble3& vector);
	static YVector ConvertScale(const FbxVector4& vector);
	static YRotator ConvertRotation(const FbxQuaternion& quaternion);
	static YQuat ConvertFbxQutaToQuat(const FbxQuaternion& quaternion);
	static YMatrix ConvertFbxMatrix(const FbxAMatrix& matrix);
	static YTransform ConverterFbxTransform(const FbxAMatrix& matrix);

	//our to fbx
	static FbxVector4 ConvertToFbxPos(const YVector& vector);
	static FbxVector4 ConvertToFbxRot(const YVector& vector);
	static FbxVector4 ConvertToFbxScale(const YVector& vector);

	static FbxString ConvertToFbxString(const std::string& str);
};
class YFbxImporter;
struct FbxUVs
{
	FbxUVs(YFbxImporter* importer, FbxMesh* mesh);
	void Phase2(YFbxImporter* importer, FbxMesh* mesh);
	std::vector<std::string> uv_set;
	std::vector<FbxLayerElementUV const*> layer_element_uv;
	std::vector<FbxLayerElement::EReferenceMode> uv_reference_mode;
	std::vector<FbxLayerElement::EMappingMode> uv_mapping_mode;
	int unique_count;
};


struct FImportedMaterialData
{
public:
	//void AddImportedMaterial(const FbxSurfaceMaterial* fbx)
	std::unordered_map<const FbxSurfaceMaterial*, std::shared_ptr<YFbxMaterial>> fbx_material_to_us_material;
	std::set<std::string> imported_material_names;

};