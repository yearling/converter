#include "YEditor.h"
#include <windows.h>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "RHI/DirectX11/D3D11Device.h"
#include "imgui_internal.h"
#include "widget/YConsoleWidget.h"
#include "Engine/YLog.h"
#include "widget/YViewportWidget.h"
//= EDITOR OPTIONS ========================================================================================
// Shapes
const float k_roundness = 2.0f;
// Font
const float k_font_size = 24.0f;
const float k_font_scale = 0.7f;
// Color
const ImVec4 k_color_text = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
const ImVec4 k_color_text_disabled = ImVec4(54.0f / 255.0f, 54.0f / 255.0f, 54.0f / 255.0f, 1.0f);
const ImVec4 k_color_dark_very = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);
const ImVec4 k_color_dark = ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 0.8f);
const ImVec4 k_color_mid = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
const ImVec4 k_color_light = ImVec4(47.0f / 255.0f, 47.0f / 255.0f, 47.0f / 255.0f, 1.0f);
const ImVec4 k_color_shadow = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
const ImVec4 k_color_interactive = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
const ImVec4 k_color_interactive_hovered = ImVec4(0.450f, 0.450f, 0.450f, 1.000f);
const ImVec4 k_color_check = ImVec4(26.0f / 255.0f, 140.0f / 255.0f, 192.0f / 255.0f, 1.0f);

static bool InitIMGUI(HWND hwnd)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
	//io.BackendFlags = ImGuiBackendFlags_PlatformHasViewports | ImGuiBackendFlags_RendererHasViewports | ImGuiBackendFlags_HasMouseCursors;
	io.ConfigWindowsResizeFromEdges = true;
	io.ConfigViewportsNoTaskBarIcon = true;
	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_device->GetDevice(), g_device->GetDC());


	return true;
}
static void ImGui_ApplyColors()
{
	// Use default dark style as a base
	ImGui::StyleColorsDark();
	ImVec4* colors = ImGui::GetStyle().Colors;

	// Colors
	colors[ImGuiCol_Text] = k_color_text;
	colors[ImGuiCol_TextDisabled] = k_color_text_disabled;
	colors[ImGuiCol_WindowBg] = k_color_dark;                  // Background of normal windows
	colors[ImGuiCol_ChildBg] = k_color_mid;                   // Background of child windows
	colors[ImGuiCol_PopupBg] = k_color_dark;                  // Background of popups, menus, tooltips windows
	colors[ImGuiCol_Border] = k_color_interactive;
	colors[ImGuiCol_BorderShadow] = k_color_shadow;
	colors[ImGuiCol_FrameBg] = k_color_dark_very;             // Background of checkbox, radio button, plot, slider, text input
	colors[ImGuiCol_FrameBgHovered] = k_color_interactive;
	colors[ImGuiCol_FrameBgActive] = k_color_dark_very;
	colors[ImGuiCol_TitleBg] = k_color_dark;
	colors[ImGuiCol_TitleBgActive] = k_color_dark;
	colors[ImGuiCol_TitleBgCollapsed] = k_color_light;
	colors[ImGuiCol_MenuBarBg] = k_color_dark;
	colors[ImGuiCol_ScrollbarBg] = k_color_mid;
	colors[ImGuiCol_ScrollbarGrab] = k_color_interactive;
	colors[ImGuiCol_ScrollbarGrabHovered] = k_color_interactive_hovered;
	colors[ImGuiCol_ScrollbarGrabActive] = k_color_dark_very;
	colors[ImGuiCol_CheckMark] = k_color_check;
	colors[ImGuiCol_SliderGrab] = k_color_interactive;
	colors[ImGuiCol_SliderGrabActive] = k_color_dark_very;
	colors[ImGuiCol_Button] = k_color_interactive;
	colors[ImGuiCol_ButtonHovered] = k_color_interactive_hovered;
	colors[ImGuiCol_ButtonActive] = k_color_dark_very;
	colors[ImGuiCol_Header] = k_color_light;                 // Header colors are used for CollapsingHeader, TreeNode, Selectable, MenuItem
	colors[ImGuiCol_HeaderHovered] = k_color_interactive_hovered;
	colors[ImGuiCol_HeaderActive] = k_color_dark_very;
	colors[ImGuiCol_Separator] = k_color_dark_very;
	colors[ImGuiCol_SeparatorHovered] = k_color_light;
	colors[ImGuiCol_SeparatorActive] = k_color_light;
	colors[ImGuiCol_ResizeGrip] = k_color_interactive;
	colors[ImGuiCol_ResizeGripHovered] = k_color_interactive_hovered;
	colors[ImGuiCol_ResizeGripActive] = k_color_dark_very;
	colors[ImGuiCol_Tab] = k_color_light;
	colors[ImGuiCol_TabHovered] = k_color_interactive_hovered;
	colors[ImGuiCol_TabActive] = k_color_dark_very;
	colors[ImGuiCol_TabUnfocused] = k_color_light;
	colors[ImGuiCol_TabUnfocusedActive] = k_color_light;                 // Might be called active, but it's active only because it's it's the only tab available, the user didn't really activate it
	colors[ImGuiCol_DockingPreview] = k_color_dark_very;             // Preview overlay color when about to docking something
	colors[ImGuiCol_DockingEmptyBg] = k_color_interactive;           // Background color for empty node (e.g. CentralNode with no window docked into it)
	colors[ImGuiCol_PlotLines] = k_color_interactive;
	colors[ImGuiCol_PlotLinesHovered] = k_color_interactive_hovered;
	colors[ImGuiCol_PlotHistogram] = k_color_interactive;
	colors[ImGuiCol_PlotHistogramHovered] = k_color_interactive_hovered;
	colors[ImGuiCol_TextSelectedBg] = k_color_dark;
	colors[ImGuiCol_DragDropTarget] = k_color_interactive_hovered;   // Color when hovering over target
	colors[ImGuiCol_NavHighlight] = k_color_dark;                  // Gamepad/keyboard: current highlighted item
	colors[ImGuiCol_NavWindowingHighlight] = k_color_dark;                  // Highlight window when using CTRL+TAB
	colors[ImGuiCol_NavWindowingDimBg] = k_color_dark;                  // Darken/colorize entire screen behind the CTRL+TAB window list, when active
	colors[ImGuiCol_ModalWindowDimBg] = k_color_dark;                  // Darken/colorize entire screen behind a modal window, when one is active
}

static void ImGui_ApplyStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;
	style.ScrollbarSize = 20.0f;
	style.FramePadding = ImVec2(5, 5);
	style.ItemSpacing = ImVec2(6, 5);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.WindowRounding = k_roundness;
	style.FrameRounding = k_roundness;
	style.PopupRounding = k_roundness;
	style.GrabRounding = k_roundness;
	style.ScrollbarRounding = k_roundness;
	style.Alpha = 1.0f;
}

static bool ShutdownIMGUI()
{
	// Cleanup
	if (ImGui::GetCurrentContext())
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	return true;
}


Editor::Editor(YEngine* engine)
	:engine_(engine)
{

}

Editor::~Editor()
{
	ShutdownIMGUI();
}

void Editor::Update(double delta_time)
{

	engine_->Update();

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	BeginWindow();
		
	for (std::unique_ptr<Widget>& widget : widgets_)
	{
		widget->Update(delta_time);
	}
	// Editor - End
	if (m_editor_begun)
	{
		ImGui::End();
	}


	// ImGui - End/Render
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	
	g_device->Present();

	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();
}

bool Editor::Init(HWND hwnd)
{
	InitIMGUI(hwnd);
	ImGui_ApplyColors();
	ImGui_ApplyStyle();
	
	widgets_.emplace_back(std::make_unique<Widget_Console>(this));
	widgets_.emplace_back(std::make_unique<ViewportWidget>(this));
	return true;
}

void Editor::Close()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	YEngine* engine = YEngine::GetEngine();
	engine->ShutDown();
}

void Editor::BeginWindow()
{
	// Set window flags
	const auto window_flags =
		//ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	// Set window position and size
	//float offset_y = _editor::widget_menu_bar ? (_editor::widget_menu_bar->GetHeight() + _editor::widget_menu_bar->GetPadding()) : 0;
	float offset_y  = 0 ;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + offset_y));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - offset_y));
	ImGui::SetNextWindowViewport(viewport->ID);

	// Set window style
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowBgAlpha(0.0f);

	// Begin window
	std::string name = "##main_window";
	bool open = true;
	m_editor_begun = ImGui::Begin(name.c_str(), &open, window_flags);
	ImGui::PopStyleVar(3);

	// Begin dock space
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable && m_editor_begun)
	{
		// Dock space
		const auto window_id = ImGui::GetID(name.c_str());
		if (!ImGui::DockBuilderGetNode(window_id))
		{
			// Reset current docking state
			ImGui::DockBuilderRemoveNode(window_id);
			ImGui::DockBuilderAddNode(window_id, ImGuiDockNodeFlags_None);
			ImGui::DockBuilderSetNodeSize(window_id, ImGui::GetMainViewport()->Size);
			LOG_INFO("rebuild every time", window_id);

			// DockBuilderSplitNode(ImGuiID node_id, ImGuiDir split_dir, float size_ratio_for_node_at_dir, ImGuiID* out_id_dir, ImGuiID* out_id_other);
			ImGuiID dock_main_id = window_id;
			ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
			const ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.6f, nullptr, &dock_right_id);
			ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);
			const ImGuiID dock_down_right_id = ImGui::DockBuilderSplitNode(dock_down_id, ImGuiDir_Right, 0.6f, nullptr, &dock_down_id);

			// Dock windows
			ImGui::DockBuilderDockWindow("World", dock_right_id);
			ImGui::DockBuilderDockWindow("Properties", dock_right_down_id);
			ImGui::DockBuilderDockWindow("Console", dock_down_id);
			ImGui::DockBuilderDockWindow("Assets", dock_down_right_id);
			ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

			ImGui::DockBuilderFinish(dock_main_id);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ImGui::DockSpace(window_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::PopStyleVar();
	}
}

