#pragma once
#include <memory>
#include <chrono>
#include "RHI/DirectX11/D3D11Device.h"
#include "YCameraController.h"
#include "Render/YRenderInterface.h"
#include "Utility/YPickupOps.h"
#include "Utility/YAverageSmooth.h"

class IRenderInterface;


class YEngine
{
public:
	inline static YEngine* GetEngine()
	{
		static YEngine instance;
		return &instance;
	}

public:
	YEngine(const YEngine&) = delete;
	YEngine(YEngine&&) = delete;
	YEngine& operator=(const YEngine&) = delete;
	YEngine& operator=(YEngine&&) = delete;

public:
	bool Init();
	void Update();
	void ShutDown();
	IRenderInterface* GetRender() const { return renderer.get(); }

private:
	YEngine() = default;
	~YEngine() = default;
	std::unique_ptr<D3D11Device> device_;
	std::unique_ptr<CameraController> camera_controller;
	std::unique_ptr< YPickupShowMove> pickup;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_frame_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> game_start_time;
	std::unique_ptr<IRenderInterface> renderer;
	AverageSmooth<float> fps;
public:
	void SetApplication(class YApplication* app);
private:
	class YApplication* app_ = nullptr;
};
