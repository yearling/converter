#pragma once
#include "YWidget.h"
class AssetWidget :public Widget
{

public:
	AssetWidget(Editor* editor);
	~AssetWidget() override;
	virtual void Update(double delta_time) override;


	virtual void UpdateAlways(double delta_time) override;


	virtual void UpdateVisible(double delta_time) override;


	virtual void OnShow() override;


	virtual void OnHide() override;


	virtual void OnPushStyleVar() override;

};