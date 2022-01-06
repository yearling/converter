#pragma once
#include "Math/YVector.h"
#include "RHI/DirectX11/D3D11VertexFactory.h"
#include "Engine/YCamera.h"
class DrawUtility
{
public:
	static void DrawGrid();
	static void DrawWorldCoordinate(CameraBase* camera);
	static void DrawWorldCoordinate(class YScene* render_param);
};