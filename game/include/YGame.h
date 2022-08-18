#include "Platform/Windows/YWindows.h"
#include "RHI/DirectX11/D3D11Device.h"
#include "Engine/YCameraController.h"
#include "MeshUtility/YPickupOps.h"
#include <chrono>
#include "Render/YRenderInterface.h"
#include "Utility/YAverageSmooth.h"
#include "Engine/YEngine.h"

class GameApplication :public YApplication
{
public:
	GameApplication();
	~GameApplication() override;
	virtual LRESULT MyProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) throw();
	void Render() override;
	bool Initial() override;
	void Exit() override;
	
    void OnKeyDown(char c);
    void SwitchModel();
	std::unique_ptr<D3D11Device> device;
	std::unique_ptr<CameraController> camera_controller;
	std::unique_ptr< YPickupShowMove> pickup;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_frame_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> game_start_time;
	std::unique_ptr<IRenderInterface> renderer;
	//std::unique_ptr<Editor> g_editor;
	const int defaut_windows_width = 3840;
	const int defaut_windows_height = 2160;
    int current_index = 0;
    std::vector<std::string> static_modle_path;
};