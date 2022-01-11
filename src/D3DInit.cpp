#include "D3DInit.h"

HINSTANCE	g_hInstance(nullptr);
HWND		g_hWnd(nullptr);

std::string		g_clsName("d3d11");
std::string		g_wndTitle("1_D3DInit");

int		g_winWidth(2880);
int		g_winHeight(1620);


bool CreateWindows();

int Run()
{
	MSG msg = { 0 };
	//主消息循环，也是游戏当中的主循环
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}

	Release();

	return (int)msg.wParam;
}




bool CreateWindows()
{
	WNDCLASS wndcls;
	wndcls.cbClsExtra = 0;
	wndcls.cbWndExtra = 0;
	wndcls.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wndcls.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndcls.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wndcls.hInstance = g_hInstance;
	wndcls.lpfnWndProc = WinProc;
	wndcls.lpszClassName = g_clsName.c_str();
	wndcls.lpszMenuName = nullptr;
	wndcls.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClass(&wndcls))
	{
		MessageBox(nullptr, "Register window failed!", "error", MB_OK);
		return FALSE;
	}
	DWORD WindowsWithTitleAndBoderStyle = WS_TILEDWINDOW | WS_SIZEBOX;
	DWORD WindowsPOPStyle = WS_POPUP;
	g_hWnd = CreateWindow(g_clsName.c_str(), g_wndTitle.c_str(), WindowsWithTitleAndBoderStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		g_winWidth, g_winHeight,
		nullptr,
		nullptr,
		g_hInstance,
		nullptr);
	if (!g_hWnd)
	{
		MessageBox(nullptr, "Create window failed!", "error", MB_OK);
		return FALSE;
	}
	else {
		RECT window_rect = { 0, 0, (LONG)g_winWidth,(LONG)g_winHeight };
		// make the call to adjust window_rect
		::AdjustWindowRect(&window_rect, GetWindowStyle(g_hWnd), ::GetMenu(g_hWnd) != nullptr);
		MoveWindow(g_hWnd, 400,                           // x position
			200,                                   // y position
			window_rect.right - window_rect.left,  // width
			window_rect.bottom - window_rect.top,  // height
			FALSE);
	}

	ShowWindow(g_hWnd, SW_SHOW);
	UpdateWindow(g_hWnd);
	SetWindowPos(g_hWnd, HWND_TOP, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
	return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	g_hInstance = hInstance;

	if (!CreateWindows())
		return -1;
	if (!InitD3D())
	{
		return -1;
	}

	SetWindowPos(g_hWnd, HWND_TOP, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
	return Run();
}

