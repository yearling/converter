#pragma once
#include <vector>
#include <memory>
#include "YWidget.h"
#include <windows.h>

class Editor
{
public:
	Editor();
	~Editor();
	void Update(double delta_time);
	template<class T>
	T* GetWidget()
	{
		for(const auot& widget:)
	}
	bool Init(HWND hwnd);
protected:
	void BeginWindow();
	bool m_editor_begun = false; 
	std::vector<std::unique_ptr<Widget>> widgets_;

};