#pragma once
#include <memory>
#include <chrono>
#include "RHI/DirectX11/D3D11Device.h"
#include "YCameraController.h"
#include "Render/YRenderInterface.h"
#include "MeshUtility/YPickupOps.h"
#include "Utility/YAverageSmooth.h"
#include "TaskGraphInterfaces.h"
#include "Render/YRenderThread.h"
#include "Render/RenderCommandFence.h"
#include "YSkeletonMesh.h"

class IRenderInterface;

class FFrameEndSync
{
	/** Pair of fences. */
	FRenderCommandFence Fence[2];
	/** Current index into events array. */
	int32 EventIndex;
public:
	/**
	 * Syncs the game thread with the render thread. Depending on passed in bool this will be a total
	 * sync or a one frame lag.
	 */
	void Sync(bool bAllowOneFrameThreadLag);
};


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
	bool PreInit();
	bool Init();
	void PostInitRHI();
	void Update();
	void Update1();
	void ShutDown();
	void TestLoad();
	IRenderInterface* GetRender() const { return renderer.get(); }
	double GetCurrentGameTime();
    //temp hold converted result
	std::unique_ptr<YSkeletonMesh> skeleton_mesh_;
    std::unique_ptr<YStaticMesh> static_mesh_;
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
	/** Holds the objects which need to be cleaned up when the rendering thread finishes the previous frame. */
	FPendingCleanupObjects* PendingCleanupObjects;

protected:
	bool InitThridParty();
	void ShutDownThridParty();
public:
	void SetApplication(class YApplication* app);
private:
	class YApplication* app_ = nullptr;
	class YRenderThread* render_thread_ = nullptr;
	uint64_t frame_index = 0;
	FGraphEventRef render_fence[2] = { nullptr,nullptr };
};
