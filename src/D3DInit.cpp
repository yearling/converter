#include <Windows.h>
#include <string>
#include <D3D11.h>
#include <windowsx.h>
#include "RHI/DirectX11/D3D11Device.h"
#include "RHI/DirectX11/ComPtr.h"
#include "Engine/YLog.h"
#include "Importer/YFbxImporter.h"
#include "Engine/YCamera.h"
#include "Engine/YCanvas.h"
HINSTANCE	g_hInstance(nullptr);
HWND		g_hWnd(nullptr);

std::string		g_clsName("d3d11");
std::string		g_wndTitle("1_D3DInit");

UINT		g_winWidth(1920);
UINT		g_winHeight(1080);

ID3D11DeviceContext* g_deviceContext(nullptr);
IDXGISwapChain* g_swapChain(nullptr);

ID3D11DepthStencilView* g_depthStencilView(nullptr);
ID3D11RenderTargetView* g_renderTargetView(nullptr);
std::unique_ptr<D3D11Device> device = nullptr;
std::vector<std::unique_ptr<YStaticMesh>> g_test_mesh;
std::unique_ptr<PerspectiveCamera> main_camera;
bool CreateWindows();
bool InitD3D();


bool InitD3D()
{
	device = D3D11Device::CreateD3D11Device();
	if (!device->CreateSwapChain((void*)&g_hWnd))
	{
		ERROR_INFO("create swap chain failed!");
	}
	device->OnResize(g_winWidth, g_winHeight);
	main_camera = std::make_unique<PerspectiveCamera>(60.f, (float)g_winWidth / (float)g_winHeight, 2.f, 10000.0f);
	main_camera->SetPosition(YVector(0.0, 0.0, -20.f));
	main_camera->SetRotation(YRotator(0.0, 0.0, 0.f));
	g_Canvas = new YCamvas();
	return true;
}

bool OpenFbx()
{
	std::unique_ptr<YFbxImporter> importer = std::make_unique<YFbxImporter>();
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\cube\maya_tube4.fbx)";
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\nija\nija_head_low.FBX)";
	const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\plane\plane.FBX)";
	if (!importer->ImportFile(file_path))
	{
		return 0;
	}
	const FbxImportSceneInfo* scene_info = importer->GetImportedSceneInfo();
	FbxImportParam import_param;
	if (!scene_info->has_skin)
	{
		import_param.import_as_skelton = false;
	}
	else if (scene_info->has_skin)
	{

	}
	else if (scene_info->has_animation)
	{

	}
	ConvertedResult result;
	if (!importer->ParseFile(import_param, result))
	{

	}
	else
	{
		g_test_mesh = std::move(result.static_meshes);
	}
	return true;
}
void Render();
int	 Run();
void Release();

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	g_hInstance = hInstance;

	if (!CreateWindows())
		return -1;
	if (!InitD3D())
	{
		return -1;
	}
	if (!OpenFbx())
	{
		return -1;
	}
	for (std::unique_ptr<YStaticMesh>& mesh : g_test_mesh)
	{
		if (!mesh->AllocGpuResource())
		{
			return -1;
		}
	}
	return Run();
}

bool CreateWindows()
{
	WNDCLASS wndcls;
	wndcls.cbClsExtra = 0;
	wndcls.cbWndExtra = 0;
	wndcls.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wndcls.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndcls.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wndcls.hInstance = g_hInstance;
	wndcls.lpfnWndProc = WinProc;
	wndcls.lpszClassName = g_clsName.c_str();
	wndcls.lpszMenuName = nullptr;
	wndcls.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClass(&wndcls))
	{
		MessageBox(nullptr, "Register window failed!", "error", MB_OK);
		return FALSE;
	}
	DWORD WindowsWithTitleAndBoderStyle = WS_TILEDWINDOW | WS_SIZEBOX;
	DWORD WindowsPOPStyle = WS_POPUP;
	g_hWnd = CreateWindow(g_clsName.c_str(), g_wndTitle.c_str(), WindowsWithTitleAndBoderStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		g_winWidth, g_winHeight,
		nullptr,
		nullptr,
		g_hInstance,
		nullptr);
	if (!g_hWnd)
	{
		MessageBox(nullptr, "Create window failed!", "error", MB_OK);
		return FALSE;
	}
	else {
		RECT window_rect = { 0, 0, (LONG)g_winWidth,(LONG)g_winHeight };
		// make the call to adjust window_rect
		::AdjustWindowRect(&window_rect, GetWindowStyle(g_hWnd), ::GetMenu(g_hWnd) != nullptr);
		MoveWindow(g_hWnd, 400,                           // x position
			200,                                   // y position
			window_rect.right - window_rect.left,  // width
			window_rect.bottom - window_rect.top,  // height
			FALSE);
	}

	ShowWindow(g_hWnd, SW_SHOW);
	UpdateWindow(g_hWnd);

	return TRUE;
}
void Render()
{
	main_camera->Update();
	// 正式的场景绘制工作
		//device->PreRender();
	TComPtr<ID3D11RenderTargetView> main_rtv = device->GetMainRTV();
	TComPtr<ID3D11DepthStencilView> main_dsv = device->GetMainDSV();
	device->SetRenderTarget(main_rtv, main_dsv);
	device->SetViewPort(0, 0, 1920, 1080);
	if (!device) {
		return;
	}
	TComPtr<ID3D11Device> raw_device = device->GetDevice();
	TComPtr<ID3D11DeviceContext> raw_dc = device->GetDC();
	// 绘制青色背景
	float color[4] = { 0.f, 0.f, 0.f, 1.0f };
	raw_dc->ClearRenderTargetView(main_rtv, reinterpret_cast<float*>(color));
	raw_dc->ClearDepthStencilView(main_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	for (std::unique_ptr<YStaticMesh>& mesh : g_test_mesh)
	{
		if (mesh)
		{
			mesh->Render(main_camera.get());
		}
	}

	device->Present();
}

int Run()
{
	MSG msg = { 0 };
	//主消息循环，也是游戏当中的主循环
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}

	Release();

	return (int)msg.wParam;
}

void Release()
{
	delete g_Canvas;
	g_Canvas = nullptr;
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
