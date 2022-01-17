#include "YEditorApplication.h"
#include "Engine/YCanvas.h"
#include "Engine/YInputManager.h"
#include "Engine/YWindowEventManger.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "Engine/YCanvasUtility.h"
#include "SObject/SWorld.h"
#include "Render/YForwardRenderer.h"
#include "SObject/SObjectManager.h"


EditorApplication::EditorApplication()
{

}

EditorApplication::~EditorApplication()
{

}
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT EditorApplication::MyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) throw()
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	int x = (int)(short)LOWORD(lParam);
	int y = (int)(short)HIWORD(lParam);

	switch (msg)
	{
	case WM_LBUTTONDOWN:
		g_input_manager->OnEventLButtonDown(x, y);
		break;
	case WM_RBUTTONDOWN:
		SetCapture(hwnd);
		g_input_manager->OnEventRButtonDown(x, y);
		break;

	case WM_RBUTTONUP:
		g_input_manager->OnEventRButtonUp(x, y);
		ReleaseCapture();
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
	case WM_SIZE:
	{
		int g_winWidth = LOWORD(lParam);
		int g_winHeight = HIWORD(lParam);
		if (device)
		{
			switch (wParam)
			{
			case SIZE_MINIMIZED:
			{
				break;
			}
			case SIZE_MAXIMIZED:
			{
				g_windows_event_manager->OnWindowSizeChange(g_winWidth, g_winHeight);
				break;
			}
			case SIZE_RESTORED:
			{
				g_windows_event_manager->OnWindowSizeChange(g_winWidth, g_winHeight);
				break;
			}
			default:
				break;
			}
		}
		break;
	}
	case WM_ENTERSIZEMOVE:
	{
		break;
	}
	case WM_EXITSIZEMOVE:
	{
		break;
	}
	case WM_SYSCOMMAND:
	{
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return YApplication::MyProc(hwnd, msg, wParam, lParam);
}


void EditorApplication::Render()
{
	double delta_time = 0.0;
	double game_time = 0.0;
	{
		std::chrono::time_point<std::chrono::high_resolution_clock> current_time = std::chrono::high_resolution_clock::now();
		delta_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_frame_time).count();
		delta_time *= 0.000001; // to second
		last_frame_time = current_time;
		fps.SmoothAcc((float)(1.0f / delta_time));

		game_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(current_time - game_start_time).count();
		game_time *= 0.000001;
	}
	//LOG_INFO("fps: ", fps.Average());

	//update
	//Update(delta_time);
	camera_controller->Update(delta_time);
	SWorld::GetWorld()->Update(delta_time);
	pickup->Update(delta_time);

	DrawUtility::DrawGrid();
	DrawUtility::DrawWorldCoordinate(SWorld::GetWorld()->GetMainScene());
	//render
	std::unique_ptr<YRenderScene> render_scene = SWorld::GetWorld()->GenerateRenderScene();
	render_scene->deta_time = delta_time;
	render_scene->game_time = game_time;
	renderer->Render(std::move(render_scene));
	// 正式的场景绘制工作
	//DrawUI();
	g_editor->Update(delta_time);
	device->Present();
}

bool EditorApplication::Initial()
{
	device = D3D11Device::CreateD3D11Device();
	const int defaut_windows_width = 1920;
	const int defaut_windows_height = 1080;
	//inmput manager
	g_input_manager = new InputManger();

	// windows_event_manager
	g_windows_event_manager = new WindowEventManager();

	WindowCreate(defaut_windows_width, defaut_windows_height);
	YWindow* main_window = windows_[0].get();
	//create swap chain
	HWND current_window_hwnd = main_window->GetHWND();
	if (!device->CreateSwapChain((void*)&current_window_hwnd))
	{
		ERROR_INFO("create swap chain failed!");
	}
	//resize
	device->OnResize(defaut_windows_width, defaut_windows_height);
	//canvas
	g_Canvas = new YCamvas();



	device->RegisterEvents();

	pickup = std::make_unique<YPickupShowMove>();
	pickup->RegiesterEventProcess();
	renderer = std::make_unique<YForwardRenderer>();
	if (!renderer->Init())
	{
		ERROR_INFO("forward render init failed");
		return false;
	}
	game_start_time = std::chrono::high_resolution_clock::now();
	g_editor = std::make_unique<Editor>();
	g_editor->Init(current_window_hwnd);

	std::string world_map_path = "map/world.json";
	TRefCountPtr<SWorld> new_world = SObjectManager::ConstructUnifyFromPackage<SWorld>(world_map_path);
	SWorld::SetWorld(new_world);
	new_world->PostLoadOp();
	new_world->GetMainScene()->RegisterEvents();
	{
		SPerspectiveCameraComponent* camera_component = SWorld::GetWorld()->GetMainScene()->GetPerspectiveCameraComponent();
		if (camera_component)
		{
			camera_controller = std::make_unique<FPSCameraController>();
			camera_controller->SetCamera(camera_component->camera_.get());
			camera_controller->RegiesterEventProcess();
		}
	}

	g_windows_event_manager->OnWindowSizeChange(defaut_windows_width, defaut_windows_height);
}

void EditorApplication::Exit()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete g_Canvas;
	g_Canvas = nullptr;
	delete g_input_manager;
	g_input_manager = nullptr;
	renderer->Clearup();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	EditorApplication app;
	app.SetInstance(hInstance);
	app.Initial();
	app.Run();
	app.Exit();
}

