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
#include "FreeImage.h"
#include "SObject/STexture.h"
#include "Engine/TaskGraphInterfaces.h"
#include "Platform/Windows/YSysUtility.h"
#include "Render/YRenderThread.h"
#include <thread>



struct RootTask
{
public: 
	RootTask() {};
	~RootTask() {};
	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::GameThread;
	}
	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::FireAndForget;
	}
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		// The arguments are useful for setting up other tasks.
		// Do work here, probably using SomeArgument.
		//MyCompletionGraphEvent->DontCompleteUntil(TGraphTask<FSomeChildTask>::CreateTask(NULL, CurrentThread).ConstructAndDispatchWhenReady());
		LOG_INFO("Root Task");
	}
};



struct BusyTask
{
public:
	BusyTask() {};
	~BusyTask() {};
	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}
	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::FireAndForget;
	}
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		// The arguments are useful for setting up other tasks.
		// Do work here, probably using SomeArgument.
		//MyCompletionGraphEvent->DontCompleteUntil(TGraphTask<FSomeChildTask>::CreateTask(NULL, CurrentThread).ConstructAndDispatchWhenReady());
		LOG_INFO("Busy Task",(int)FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
		//while (1);
		::Sleep(10000);
		TGraphTask< FReturnGraphTask>::CreateTask(NULL, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(ENamedThreads::GameThread);
	}
};


struct RenderThreadTaskNumber
{
public:
	RenderThreadTaskNumber(uint64_t in_num) :num(in_num) {}
	~RenderThreadTaskNumber() {}
	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::ActualRenderingThread;
		//return ENamedThreads::AnyThread;
	}
	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::TrackSubsequents;
	}
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		//::Sleep(500);
		LOG_INFO(" render thread number : ", num);
	}
	uint64_t num;
};

struct RenderThreadTaskLambdaBug
{
public:
	RenderThreadTaskLambdaBug(std::function<void(uint64_t)>&& func,uint64_t in_num) :func_(std::move(func)),num(in_num) {}
	~RenderThreadTaskLambdaBug() {}
	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::ActualRenderingThread;
		//return ENamedThreads::AnyThread;
	}
	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::TrackSubsequents;
	}
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		//::Sleep(30);
		func_(num);
	}
	std::function<void(uint64_t)> func_;
	uint64_t num;
};
bool YEngine::Init()
{
	//YSysUtility::AllocWindowsConsole();
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
	g_windows_event_manager->OnWindowSizeChange(width, height);

	if (!InitThridParty())
	{
		ERROR_INFO("Third party libaray initial failed!");
		return false;
	}

	// initialize task graph sub-system with potential multiple threads
	FTaskGraphInterface::Startup(FPlatformProcess::NumberOfCores());
	FTaskGraphInterface::Get().AttachToThread(ENamedThreads::GameThread);
	for (int i = 0; i < 14; ++i)
	{
		//TGraphTask<BusyTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady();
	}
	//TGraphTask<RootTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady();
	//TGraphTask<BusyTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady();

	render_thread_ = new YRenderThread();
	render_thread_->CreateThread();

	render_fence[0] = nullptr;
	render_fence[1] = nullptr;
	return true;
}


static void sleep_for(double dt)
{
	static constexpr std::chrono::duration<double> MinSleepDuration(0);
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	while (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() < dt) {
		std::this_thread::sleep_for(MinSleepDuration);
	}
}

void YEngine::Update()
{
	//int current_fence_index = frame_index % 2;
	int current_fence_index = 0;
	sleep_for(0.010);
	if (render_fence[current_fence_index])
	{
		std::chrono::time_point<std::chrono::high_resolution_clock> wait_time_before = std::chrono::high_resolution_clock::now();
		FTaskGraphInterface::Get().WaitUntilTaskCompletes(render_fence[current_fence_index]);
		std::chrono::time_point<std::chrono::high_resolution_clock> wait_time_end = std::chrono::high_resolution_clock::now();
	    double	wiat_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(wait_time_end - wait_time_before).count();
		wiat_time *= 0.001; //
		//LOG_INFO("wait time : ", wiat_time);
		//LOG_INFO("game thread ", frame_index);
	}
	double delta_time = 0.0;
	double game_time = 0.0;
	static bool first_frame = true;
	{
		std::chrono::time_point<std::chrono::high_resolution_clock> current_time = std::chrono::high_resolution_clock::now();
		delta_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_frame_time).count();
		delta_time *= 0.000001; // to second
		last_frame_time = current_time;
		fps.SmoothAcc((float)(1.0f / delta_time));

		game_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(current_time - game_start_time).count();
		game_time *= 0.000001;
		if (first_frame)
		{
			delta_time = 0.0;
			first_frame = false;
		}
	}
	LOG_INFO("fps: ", fps.Average());
	//FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
	//static int reentry = 1;
	//if (reentry == 1)
	//{
	//
	//	reentry++;
	//}
	//update
	//Update(delta_time);
	//FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
	camera_controller->Update(delta_time);
	SWorld::GetWorld()->Update(delta_time);
	pickup->Update(delta_time);

	DrawUtility::DrawGrid();
	DrawUtility::DrawWorldCoordinate(SWorld::GetWorld()->GetMainScene());
	//render
	//std::unique_ptr<YRenderScene> render_scene = SWorld::GetWorld()->GenerateRenderScene();
	//render_scene->deta_time = delta_time;
	//render_scene->game_time = game_time;
	//renderer->Render(std::move(render_scene));


	//ID3D11RenderTargetView* main_rtv = g_device->GetMainRTV();
	//ID3D11Resource* main_rt_color;
	//main_rtv->GetResource(&main_rt_color);
	//ID3D11Resource* rt_color = dynamic_cast<YForwardRenderer*>(this->GetRender())->GetRTs()->GetColorBuffer();
	//g_device->GetDC()->CopyResource(main_rt_color, rt_color);
	//g_device->Present();
	// 正式的场景绘制工作
	//DrawUI();
	//TGraphTask<RenderThreadTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady([=]() { LOG_INFO("render frame ",frame_index); });
	//TGraphTask<RenderThreadTaskLambdaBug>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady([frame_index](uint64_t s) { LOG_INFO("render frame ",frame_index, "pass: ",s); }, frame_index);
	FPlatformMisc::MemoryBarrier();

	auto local_frame_index = frame_index;
	//render_fence[current_fence_index] = TGraphTask<RenderThreadTaskNumber>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady(frame_index);
	render_fence[current_fence_index] = TGraphTask<RenderThreadTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady([this, delta_time, game_time,local_frame_index]() {
		//LOG_INFO("render thread ", local_frame_index);
		std::unique_ptr<YRenderScene> render_scene = SWorld::GetWorld()->GenerateRenderScene();
		render_scene->deta_time = delta_time;
		render_scene->game_time = game_time;
		renderer->Render(std::move(render_scene));

		ID3D11RenderTargetView* main_rtv = g_device->GetMainRTV();
		ID3D11Resource* main_rt_color;
		main_rtv->GetResource(&main_rt_color);
		ID3D11Resource* rt_color = dynamic_cast<YForwardRenderer*>(this->GetRender())->GetRTs()->GetColorBuffer();
		g_device->GetDC()->CopyResource(main_rt_color, rt_color);
		g_device->Present();
		});
	
	frame_index++;
}

void YEngine::ShutDown()
{
	delete g_Canvas;
	g_Canvas = nullptr;
	delete g_input_manager;
	g_input_manager = nullptr;
	renderer->Clearup();
	ShutDownThridParty();
	FTaskGraphInterface::Get().Shutdown();
}

void YEngine::TestLoad()
{
	std::string world_map_path = "/map/world.json";
	TRefCountPtr<SWorld> new_world = SObjectManager::ConstructFromPackage<SWorld>(world_map_path,nullptr);
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
	std::unique_ptr<YWindow>& main_window = app_->GetWindows()[0];
	int width = main_window->GetWidth();
	int height = main_window->GetHeight();
	g_windows_event_manager->OnWindowSizeChange(width, height);

	std::string test_pic = "/textures/uv.png";
	TRefCountPtr<STexture> texture = SObjectManager::ConstructFromPackage<STexture>(test_pic,nullptr);
	if (texture)
	{
		texture->UploadGPUBuffer();
	}
}

static void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char* message) {
	if (fif != FIF_UNKNOWN) {
		WARNING_INFO( FreeImage_GetFormatFromFIF(fif), "format ", message);
	}
}

bool YEngine::InitThridParty()
{
	//free image
	FreeImage_Initialise(false);
	const char* free_imge_version = FreeImage_GetVersion();
	LOG_INFO("Free Image Version: ", free_imge_version);
	FreeImage_SetOutputMessage(FreeImageErrorHandler);
	return true;
}

void YEngine::ShutDownThridParty()
{
	//free image
	FreeImage_DeInitialise();
}

void YEngine::SetApplication(class YApplication* app)
{
	app_ = app;
}


