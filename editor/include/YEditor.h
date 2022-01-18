#pragma once
#include <vector>
#include <memory>
#include "YWidget.h"
#include <windows.h>
#include "Engine/YEngine.h"
class Editor
{
public:
	Editor(YEngine* engine);
	~Editor();
	void Update(double delta_time);
	template<class T>
	T* GetWidget()
	{
		for(const auot& widget:)
	}
	bool Init(HWND hwnd);
	void Close();
protected:
	void BeginWindow();
	bool m_editor_begun = false; 
	std::vector<std::unique_ptr<Widget>> widgets_;
	YEngine* engine_ = nullptr;

};