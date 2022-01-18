#include "Platform/Windows/YWindows.h"
#include "RHI/DirectX11/D3D11Device.h"
#include "Engine/YCameraController.h"
#include "Utility/YPickupOps.h"
#include <chrono>
#include "Render/YRenderInterface.h"
#include "YEditor.h"
#include "Utility/YAverageSmooth.h"
#include "Engine/YEngine.h"

class EditorApplication :public YApplication
{
public:
	EditorApplication();
	~EditorApplication() override;
	virtual LRESULT MyProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) throw();
	void Render() override;
	bool Initial() override;
	void Exit() override;
	
	std::unique_ptr<D3D11Device> device;
	std::unique_ptr<CameraController> camera_controller;
	std::unique_ptr< YPickupShowMove> pickup;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_frame_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> game_start_time;
	std::unique_ptr<IRenderInterface> renderer;
	std::unique_ptr<Editor> g_editor;
	const int defaut_windows_width = 1920;
	const int defaut_windows_height = 1080;
};