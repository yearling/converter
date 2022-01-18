#pragma once
#include "YWidget.h"
#include "YEditor.h"

class ViewportWidget :public Widget
{
public:
	ViewportWidget(Editor* editor);
	~ViewportWidget() override;
	void UpdateVisible(double delta_time) override;
protected:
	float m_width = 0.0f;
	float m_height = 0.0f;

};