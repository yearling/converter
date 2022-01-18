#include "Engine/YEngine.h"
#include "Engine/YInputManager.h"
#include "Engine/YWindowEventManger.h"
#include "Platform/Windows/YWindows.h"
#include "Engine/YLog.h"
#include "Engine/YCanvas.h"
#include "Utility/YPickupOps.h"
#include "Render/YForwardRenderer.h"
#include "SObject/SWorld.h"
#include "SObject/SObjectManager.h"
#include "Engine/YCanvasUtility.h"

bool YEngine::Init()
{
	device_= D3D11Device::CreateD3D11Device();
	//inmput manager
	g_input_manager = new InputManger();
	// windows_event_manager
	g_windows_event_manager = new WindowEventManager();
	std::unique_ptr<YWindow>& main_window = app_->GetWindows()[0];
	HWND hwnd = main_window->GetHWND();
	int width = main_window->GetWidth();
	int height = main_window->GetHeight();
	if (!device_->CreateSwapChain((void*)&hwnd))
	{
		ERROR_INFO("create swap chain failed!");
		return false;
	}
	//resize
	device_->OnResize(width, height);
	device_->RegisterEvents();

	//canvas
	g_Canvas = new YCamvas();

	pickup = std::make_unique<YPickupShowMove>();
	pickup->RegiesterEventProcess();
	renderer = std::make_unique<YForwardRenderer>();
	if (!renderer->Init())
	{
		ERROR_INFO("forward render init failed");
		return false;
	}
	game_start_time = std::chrono::high_resolution_clock::now();


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

	g_windows_event_manager->OnWindowSizeChange(width, height);
	return true;
}



void YEngine::Update()
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
}

void YEngine::ShutDown()
{
	delete g_Canvas;
	g_Canvas = nullptr;
	delete g_input_manager;
	g_input_manager = nullptr;
	renderer->Clearup();
}

void YEngine::SetApplication(class YApplication* app)
{
	app_ = app;
}


