#include "Platform/Windows/YWindows.h"
#include "Engine/YLog.h"
#include <windowsx.h>
#include <winuser.h>

YWindow::YWindow()
{

}

HWND YWindow::GetHWND() const
{
	return hwnd_;
}

void YWindow::SetHWND(HWND hwnd)
{
	hwnd_ = hwnd;
}

void YWindow::SetWidth(int width)
{
	width_ = width;
}

void YWindow::SetHeight(int height)
{
	height_ = height;
}

int YWindow::GetWidth() const
{
	return width_;
}

int YWindow::GetHeight() const
{
	return height_;
}

YApplication::YApplication(void)
{

}

YApplication::~YApplication(void)
{

}

bool YApplication::WindowCreate(int width, int height)
{
	WNDCLASS wndcls;
	wndcls.cbClsExtra = 0;
	wndcls.cbWndExtra = 0;
	wndcls.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wndcls.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndcls.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wndcls.hInstance = instance_;
	;
	wndcls.lpfnWndProc = WndProc;
	wndcls.lpszClassName = "D3D11Demo";
	wndcls.lpszMenuName = nullptr;
	wndcls.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClass(&wndcls))
	{
		ERROR_INFO("Register window failed!");
		return false;
	}
	DWORD WindowsWithTitleAndBoderStyle = WS_TILEDWINDOW | WS_SIZEBOX;
	DWORD WindowsPOPStyle = WS_POPUP;
	int default_win_width = 1920;
	int default_win_height = 1080;
	HWND hwnd = CreateWindow("D3D11Demo", "SolidAngleEngine", WindowsWithTitleAndBoderStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		default_win_width, default_win_width,
		nullptr,
		nullptr,
		instance_,
		this);
	if (!hwnd)
	{

		ERROR_INFO("Create window failed!");
		return false;
	}
	else {
		RECT window_rect = { 0, 0, (LONG)default_win_width,(LONG)default_win_height };
		// make the call to adjust window_rect
		::AdjustWindowRect(&window_rect, GetWindowStyle(hwnd), ::GetMenu(hwnd) != nullptr);
		MoveWindow(hwnd, 400,                           // x position
			200,                                   // y position
			window_rect.right - window_rect.left,  // width
			window_rect.bottom - window_rect.top,  // height
			FALSE);
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SetWindowPos(hwnd, HWND_TOP, 1, 1, 1, 1, SWP_NOMOVE | SWP_NOSIZE);
	std::unique_ptr<YWindow> new_window = std::make_unique<YWindow>();
	new_window->SetHWND(hwnd);
	new_window->SetWidth(default_win_width);
	new_window->SetHeight(default_win_height);
	windows_.push_back(std::move(new_window));
	return true;
}

void YApplication::Update(float ElapseTime)
{

}

void YApplication::Render()
{

}

bool YApplication::Initial()
{
	return true;
}

int YApplication::Run()
{
	MSG msg = { 0 };
	//����Ϣѭ����Ҳ����Ϸ���е���ѭ��
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

	return (int)msg.wParam;
}

void YApplication::Exit()
{

}

LRESULT CALLBACK YApplication::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//int x = (int)(short)LOWORD(lParam);
	//int y = (int)(short)HIWORD(lParam);

	//switch (message)
	//{
	//case WM_SYSCOMMAND:
	//{
	//	if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
	//		return 0;
	//	break;
	//}
	//case WM_DESTROY:
	//	PostQuitMessage(0);
	//	return 0;
	//}
	//return DefWindowProc(hWnd, message, wParam, lParam);
	YApplication* base = nullptr;
	if (message == WM_NCCREATE)
	{
		base = reinterpret_cast<YApplication*>((DWORD_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);
		SetWindowLongPtr(hWnd, -21, (DWORD_PTR)base);
	}
	else
	{
		base = reinterpret_cast<YApplication*>(GetWindowLongPtr(hWnd, -21));
	}

	if (base == NULL)
	{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	else
	{
		return base->MyProc(hWnd, message, wParam, lParam);
	}
}

LRESULT YApplication::MyProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) throw()
{
	return DefWindowProc(hWnd, message, wParam, lParam);
}