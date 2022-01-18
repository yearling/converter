#include "widget/YViewportWidget.h"
#include "Render/YForwardRenderer.h"
#include "SObject/SWorld.h"
#include "Engine/YInputManager.h"

ViewportWidget::ViewportWidget(Editor* editor) :Widget(editor)
{
	m_title = "Viewport";
	m_size_initial = YVector2(800, 600);
	m_flags |= ImGuiWindowFlags_NoScrollbar;
	m_padding = YVector2(0.0f, 0.0f);
}

ViewportWidget::~ViewportWidget()
{

}

void ViewportWidget::UpdateVisible(double delta_time)
{
	IRenderInterface* render = YEngine::GetEngine()->GetRender();
	YForwardRenderer* forward_render = dynamic_cast<YForwardRenderer*>(render);

	if (!forward_render)
	{
		WARNING_INFO("DO NOT HAVE A RENDERER");
		return;
	}
	assert(forward_render);

	SWorld* world = SWorld::GetWorld();
	assert(forward_render);
	if (!world)
	{
		WARNING_INFO("DO NOT HAVE A WORLD");
		return;
	}
	YScene* main_scene = world->GetMainScene();
	assert(main_scene);
	if (!main_scene)
	{
		WARNING_INFO("DO NOT HAVE A SCENE");
		return;
	}

	// Get size
	float width = static_cast<float>(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
	float height = static_cast<float>(ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);

	if (m_width != width || m_height != height)
	{
		main_scene->OnViewPortChange(width, height);
		m_width = width;
		m_height = height;
	}

	YVector2 offset = ImGui::GetCursorScreenPos();

	g_input_manager->SetEditorViewportOffset(offset);
	//m_input->SetEditorViewportOffset(offset);
	// Draw the image after a potential resolution change call has been made
	ImGui::Image(
		static_cast<ImTextureID>(forward_render->GetRTs()->GetColorBufferRSV()),
		ImVec2(static_cast<float>(m_width), static_cast<float>(m_height)),
		ImVec2(0, 0),
		ImVec2(1, 1),
		ImVec4(255, 255, 255, 255),
		ImColor(0, 0, 0, 0)
	);
}

