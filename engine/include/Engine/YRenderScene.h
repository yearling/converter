#pragma once
#include <vector>
#include "Engine/YPrimitiveElement.h"
#include "YLight.h"
#include "SObject/SStaticMeshComponent.h"
#include <unordered_set>
#include "YReferenceCount.h"
#include "SObject/SComponent.h"
#include "YViewPort.h"


class YRenderScene
{
public:
	YRenderScene();
//protected:
	std::vector<PrimitiveElementProxy> primitive_elements_;
	std::vector<DirectLightElementProxy> dir_light_elements_;
	std::unique_ptr<CameraElementProxy> camera_element;
	YViewPort view_port_;
	double deta_time = 0.0;
	double game_time = 0.0;
};
class YScene
{
public:
	YScene();
	std::unordered_set<SStaticMeshComponent*> static_meshes_components_;
	std::unordered_set<SDirectionLightComponent*> direct_light_components_;
	SPerspectiveCameraComponent* perspective_camera_components_ = nullptr;
	std::unique_ptr<YRenderScene> GenerateOneFrame() const;
	double deta_time = 0.0;
	double game_time = 0.0;
	YViewPort view_port_;
	void RegisterEvents();
	void OnViewPortChange(int width, int height);
	SPerspectiveCameraComponent* GetPerspectiveCameraComponent() { return perspective_camera_components_; }
protected:
	
};