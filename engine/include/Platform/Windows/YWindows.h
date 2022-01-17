#pragma once
#include <windows.h>
#include <memory>
#include <vector>

class YWindow
{
public:
	YWindow();
	HWND GetHWND() const;
	void SetHWND(HWND hwnd);
	void SetWidth(int width);
	void SetHeight(int height);
	int  GetWidth() const;
	int GetHeight() const;
protected:
	HWND hwnd_=nullptr;
	int width_ = -1;
	int height_ = -1;
};

class YApplication
{
public:
	friend class YWindow;
	YApplication();
	virtual ~YApplication(void);
	virtual bool WindowCreate(int width, int height);
	virtual void Update(float ElapseTime);
	virtual void Render();
	virtual bool Initial();
	virtual int Run();
	virtual void Exit();
	virtual void SetInstance(HINSTANCE hIns) { instance_ = hIns; }
protected:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT MyProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) throw();
protected:
	std::vector<std::unique_ptr<YWindow>> windows_;
	HINSTANCE instance_ = nullptr;

};