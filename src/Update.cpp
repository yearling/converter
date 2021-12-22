#include "D3DInit.h"
ID3D11DeviceContext* g_deviceContext(nullptr);
IDXGISwapChain* g_swapChain(nullptr);

ID3D11DepthStencilView* g_depthStencilView(nullptr);
ID3D11RenderTargetView* g_renderTargetView(nullptr);
std::unique_ptr<D3D11Device> device = nullptr;
std::vector<std::unique_ptr<YStaticMesh>> g_test_mesh;
std::unique_ptr<PerspectiveCamera> main_camera;
std::unique_ptr<CameraController> camera_controller;
std::chrono::time_point<std::chrono::high_resolution_clock> last_frame_time;

bool InitD3D()
{
	// create device
	device = D3D11Device::CreateD3D11Device();

	//create swap chain
	if (!device->CreateSwapChain((void*)&g_hWnd))
	{
		ERROR_INFO("create swap chain failed!");
	}
	//resize
	device->OnResize(g_winWidth, g_winHeight);

	//camera
	{
		main_camera = std::make_unique<PerspectiveCamera>(60.f, (float)g_winWidth / (float)g_winHeight, 2.f, 10000.0f);
		main_camera->SetPosition(YVector(0.0, 5.0, -10.f));
		main_camera->SetRotation(YRotator(0.0, 0.0, 0.f));
	}

	//canvas
	g_Canvas = new YCamvas();

	//inmput manager
	g_input_manager = new InputManger();

	// camera controller
	{
		camera_controller = std::make_unique<FPSCameraController>();
		camera_controller->SetCamera(main_camera.get());
		camera_controller->RegiesterEventProcess();
	}

	// import fbx
	if (!OpenFbx())
	{
		return false;
	}

	for (std::unique_ptr<YStaticMesh>& mesh : g_test_mesh)
	{
		if (!mesh->AllocGpuResource())
		{
			return -1;
		}
	}
	return true;
}

bool OpenFbx()
{
	std::unique_ptr<YFbxImporter> importer = std::make_unique<YFbxImporter>();
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\cube\maya_tube4.fbx)";
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\nija\nija_head_low.FBX)";
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\plane\plane.FBX)";
	const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\shader_ball_ue\shader_ball.FBX)";
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

void Update(double delta_time)
{
	camera_controller->Update(delta_time);

	DrawUtility::DrawGrid();
	DrawUtility::DrawWorldCoordinate(main_camera.get());
	g_Canvas->Update();

}

void Render()
{
	std::chrono::time_point<std::chrono::high_resolution_clock> current_time = std::chrono::high_resolution_clock::now();
	double delta_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_frame_time).count();
	delta_time *= 0.000001; // to second
	last_frame_time = current_time;
	//LOG_INFO("fps: ", 1.0 / delta_time);
	Update(delta_time);
	// 正式的场景绘制工作
	ID3D11RenderTargetView* main_rtv = device->GetMainRTV();
	ID3D11DepthStencilView* main_dsv = device->GetMainDSV();
	device->SetRenderTarget(main_rtv, main_dsv);
	device->SetViewPort(0, 0, g_winWidth, g_winHeight);
	ID3D11Device* raw_device = device->GetDevice();
	ID3D11DeviceContext* raw_dc = device->GetDC();
	float color[4] = { 0.f, 0.f, 0.f, 1.0f };
	raw_dc->ClearRenderTargetView(main_rtv, reinterpret_cast<float*>(color));
	raw_dc->ClearDepthStencilView(main_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	g_Canvas->Render(main_camera.get());

	for (std::unique_ptr<YStaticMesh>& mesh : g_test_mesh)
	{
		if (mesh)
		{
			mesh->Render(main_camera.get());
		}
	}

	device->Present();
}
void Release()
{
	delete g_Canvas;
	g_Canvas = nullptr;
	delete g_input_manager;
	g_input_manager = nullptr;
}


LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int x = (int)(short)LOWORD(lParam);
	int y = (int)(short)HIWORD(lParam);

	switch (msg)
	{
	case WM_RBUTTONDOWN:
		g_input_manager->OnEventRButtonDown(x, y);
		break;

	case WM_RBUTTONUP:
		g_input_manager->OnEventRButtonUp(x, y);
		break;

	case WM_MOUSEMOVE:
		g_input_manager->OnMouseMove(x, y);
		break;

	case WM_KEYDOWN:
	{
		char key = (char)wParam;
		g_input_manager->OnEventKeyDown(key);
		break;
	}

	case WM_KEYUP:
		g_input_manager->OnEventKeyUp((char)wParam);
		break;
	case WM_MOUSEWHEEL:
	{
		int z_delta = (int)GET_WHEEL_DELTA_WPARAM(wParam);
		float z_normal = (float)z_delta / (float)WHEEL_DELTA;
		g_input_manager->OnMouseWheel(x, y, z_normal);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
