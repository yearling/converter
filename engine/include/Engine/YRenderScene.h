#pragma once
#include <vector>
#include "Engine/YPrimitiveElement.h"
#include "YLight.h"
#include "SObject/SStaticMeshComponent.h"
#include <unordered_set>
#include "YReferenceCount.h"
#include "SObject/SComponent.h"


class YRenderScene
{
public:
	YRenderScene();
//protected:
	std::vector<PrimitiveElementProxy> primitive_elements_;
	std::vector<DirectLightElementProxy> dir_light_elements_;
	std::unique_ptr<CameraElementProxy> camera_element;
	double deta_time = 0.0;
	double game_time = 0.0;
};
class YScene
{
public:
	YScene();
	std::unordered_set<SStaticMeshComponent*> static_meshes_components_;
	std::unordered_set<SDirectionLightComponent*> direct_light_components_;
	std::unique_ptr<YRenderScene> GenerateOneFrame() const;
	CameraBase* camera_ = nullptr;
	double deta_time = 0.0;
	double game_time = 0.0;
};