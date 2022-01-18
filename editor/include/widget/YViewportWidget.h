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


};