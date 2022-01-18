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
#include "Engine/YLog.h"


EditorApplication::EditorApplication()
{
	YApplication::is_editor = true;
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
	//WARNING_INFO("run into Editor msg process");
	POINT p;
	static POINT last_p = {};
	const BOOL b = GetCursorPos(&p);
	// under some unknown (permission denied...) rare circumstances GetCursorPos fails, we returns last known position
	if (!b) p = last_p;
	last_p = p;
	int x = p.x;
	int y = p.y;
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
		{
			switch (wParam)
			{
			case SIZE_MINIMIZED:
			{
				break;
			}
			case SIZE_MAXIMIZED:
			case SIZE_RESTORED:
			{
				if (g_windows_event_manager)
				{
					g_windows_event_manager->OnWindowSizeChange(g_winWidth, g_winHeight);
				}
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

bool EditorApplication::Initial()
{

	WindowCreate(defaut_windows_width, defaut_windows_height);
	//create engine
	YEngine* engine = YEngine::GetEngine();
	engine->SetApplication(this);
	engine->Init();
	// create editor
	g_editor = std::make_unique<Editor>(engine);
	g_editor->Init(windows_[0]->GetHWND());
	engine->TestLoad();
	return true;
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
		game_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(current_time - game_start_time).count();
		game_time *= 0.000001;
	}
	g_editor->Update(delta_time);
}

void EditorApplication::Exit()
{
	g_editor->Close();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	EditorApplication app;
	app.SetInstance(hInstance);
	app.Initial();
	app.Run();
	app.Exit();
}

