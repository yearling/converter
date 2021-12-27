#pragma once
#include <vector>
#include "Engine/YPrimitiveElement.h"
#include "YLight.h"
class YRenderScene
{
public:
	YRenderScene();
protected:
	std::vector<PrimitiveElement> primitive_elements_;
	std::vector<std::unique_ptr<LightBase>> lights_;
};