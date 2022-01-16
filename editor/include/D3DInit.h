#include <Windows.h>
#include <string>
#include <chrono>
#include <D3D11.h>
#include <windowsx.h>
#include "RHI/DirectX11/D3D11Device.h"
#include "RHI/DirectX11/ComPtr.h"
#include "Engine/YLog.h"
#include "YFbxImporter.h"
#include "Engine/YCamera.h"
#include "Engine/YCanvas.h"
#include "Engine/YInputManager.h"
#include "Engine/YCameraController.h"
#include "Engine/YCanvasUtility.h"
#include <fstream>
#include "reader.h"
bool InitD3D();
bool OpenFbx();
void Update(double delta_time);
void Render();
void Release();
bool LoadMesh();
LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern int	g_winWidth;
extern int	g_winHeight;
extern HWND		g_hWnd;
