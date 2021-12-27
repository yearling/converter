#pragma once
#include "Engine/YStaticMesh.h"
#include "Math/YMatrix.h"
struct PrimitiveElement
{
public:
	PrimitiveElement();
	YStaticMesh* mesh_{ nullptr };
	YMatrix local_to_world = YMatrix::Identity;
};