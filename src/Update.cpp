#include "D3DInit.h"
#include "Platform/Windows/YSysUtility.h"
#include "Utility/YAverageSmooth.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "Utility/YPath.h"
#include "Engine/YReferenceCount.h"
#include "SObject/SWorld.h"
#include "SObject/SObjectManager.h"
#include "Engine/YRenderScene.h"
#include "Render/YRenderInterface.h"
#include "Render/YForwardRenderer.h"
ID3D11DeviceContext* g_deviceContext(nullptr);
IDXGISwapChain* g_swapChain(nullptr);
bool is_resizing = false;
ID3D11DepthStencilView* g_depthStencilView(nullptr);
ID3D11RenderTargetView* g_renderTargetView(nullptr);
std::unique_ptr<D3D11Device> device = nullptr;
std::vector<std::unique_ptr<YStaticMesh>> g_test_mesh;
std::unique_ptr<PerspectiveCamera> main_camera;
std::unique_ptr<CameraController> camera_controller;
std::chrono::time_point<std::chrono::high_resolution_clock> last_frame_time;
std::chrono::time_point<std::chrono::high_resolution_clock> game_start_time;
std::unique_ptr<IRenderInterface> renderer;
AverageSmooth<float> fps(1000);
bool show_demo_window = false;
bool show_another_window = false;
bool show_normal = false;
YVector light_dir(0.0, 0.0, 0.0);
bool InitIMGUI()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(device->GetDevice(), device->GetDC());
	return true;
}
bool InitD3D()
{
	// open console for debug
	//YSysUtility::AllocWindowsConsole();
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
#if 1
	//if (!OpenFbx())
	//{
		//return false;
	//}
#else
	if (!LoadMesh())
	{
		return false;
	}
#endif
	//for (std::unique_ptr<YStaticMesh>& mesh : g_test_mesh)
	//{
	//	if (!mesh->AllocGpuResource())
	//	{
	//		return false;
	//	}
	//}

	//caculate light
	YVector z_ori = YVector(0.f, 0.f, 1.0f);
	YVector dir = YVector(-1.0f, -1.0f, 1.0f).GetSafeNormal();
	YVector axis = z_ori ^ dir;
	axis = axis.GetSafeNormal();
	YQuat rotator = YQuat(axis, YMath::RadiansToDegrees(YMath::Acos(z_ori | dir)));
	light_dir = rotator.Euler();
	InitIMGUI();
	//YVector light_dir_calc = rotator.ToMatrix().TransformVector(YVector::forward_vector);

	//load world
	std::string world_map_path = "map/world.json";
	TRefCountPtr<SWorld> new_world = SObjectManager::ConstructUnifyFromPackage<SWorld>(world_map_path);
	SWorld::SetWorld(new_world);
	new_world->PostLoadOp();
	SWorld::GetWorld()->SetCamera(main_camera.get());
	renderer = std::make_unique<YForwardRenderer>();
	if (!renderer->Init())
	{
		ERROR_INFO("forward render init failed");
		return false;
	}
	game_start_time = std::chrono::high_resolution_clock::now();
	return true;
}

bool OpenFbx()
{
	std::unique_ptr<YFbxImporter> importer = std::make_unique<YFbxImporter>();
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\cube\maya_tube4.fbx)";
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\nija\nija_head_low.FBX)";
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\plane\plane.FBX)";
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\shader_ball_ue\shader_ball.FBX)";
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\shader_ball_ue\shader_ball_modify_vertex.FBX)";
	//const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\sp_shader_ball\sp_shader_ball.FBX)";
	const std::string file_path = R"(C:\Users\admin\Desktop\fbxtest\sp_shader_ball\blender_shader_ball.FBX)";
	if (!importer->ImportFile(file_path))
	{
		return 0;
	}
	const FbxImportSceneInfo* scene_info = importer->GetImportedSceneInfo();
	FbxImportParam import_param;
	import_param.model_name = scene_info->model_name;
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
		for (auto& mesh : g_test_mesh)
		{
			std::string relitave_path = "model";
			std::string dir_path = YPath::PathCombine(relitave_path, mesh->model_name);
			mesh->SaveV0(dir_path);
		}
	}
	return true;
}
bool LoadMesh()
{
	std::string file_path = R"(model\blender_shader_ball\blender_shader_ball.yasset)";
	std::unique_ptr<YStaticMesh> mesh_to_load = std::make_unique<YStaticMesh>();
	if (!mesh_to_load->LoadV0(file_path))
	{
		ERROR_INFO("load ", file_path, " failed!");
		return false;
	}
	g_test_mesh.push_back(std::move(mesh_to_load));
	return true;
}
void DrawUI()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
		ImGui::DragFloat3("Light dir", &light_dir.x, 1.0f, -180, 180);
		ImGui::Checkbox("show normal", &show_normal);
		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	// Rendering
	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Update(double delta_time)
{
	camera_controller->Update(delta_time);

	DrawUtility::DrawGrid();
	DrawUtility::DrawWorldCoordinate(main_camera.get());
	g_Canvas->Update();
	SWorld::GetWorld()->Update(delta_time);
}
void Render()
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
	Update(delta_time);

	//render
	std::unique_ptr<YRenderScene> render_scene = SWorld::GetWorld()->GenerateRenderScene();
	render_scene->deta_time = delta_time;
	render_scene->game_time = game_time;
	renderer->Render(std::move(render_scene));
	// 正式的场景绘制工作
	DrawUI();
	device->Present();
}
void Release()
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

void OnResize()
{
	device->OnResize(g_winWidth, g_winHeight);
	main_camera->SetAspect((float)g_winWidth / (float)g_winHeight);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	int x = (int)(short)LOWORD(lParam);
	int y = (int)(short)HIWORD(lParam);

	switch (msg)
	{
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
		g_winWidth = LOWORD(lParam);
		g_winHeight = HIWORD(lParam);
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
				OnResize();
				break;
			}
			case SIZE_RESTORED:
			{
				OnResize();
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
		is_resizing = true;
		break;
	}
	case WM_EXITSIZEMOVE:
	{
		is_resizing = false;
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
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
