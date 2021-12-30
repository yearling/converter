#pragma once
#include "Engine/YStaticMesh.h"
#include "Math/YMatrix.h"


struct PrimitiveElementProxy
{
public:
	PrimitiveElementProxy();
	YStaticMesh* mesh_{ nullptr };
	YMatrix local_to_world_ = YMatrix::Identity;
};

struct DirectLightElementProxy
{
public:
	DirectLightElementProxy();
	YVector light_dir=YVector::forward_vector;
	YVector4 light_color= YVector4(1.0f,1.0f,1.0f,1.0f);
	float light_strength = 1.0f;
};