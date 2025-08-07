#include "editor/editor.h"

#if ENABLE_EDITOR

#include "renderer.h"
#include "panel.h"
#include "viewportpanel.h"

#include "application/window.h"

#include "core/timer.h"
#include "core/inputmanager.h"

#include "graphics/model.h"
#include "graphics/modelloader_gltf.h"
#include "graphics/modelobject.h"
#include "graphics/scene.h"

namespace vkr::Editor
{
	Manager* Manager::g_Instance = nullptr;

	Manager::Manager()
		: m_InputManager(nullptr)
	{
		assert(g_Instance == nullptr);
		g_Instance = this;
	}

	Manager::~Manager()
	{
		g_Instance = nullptr;
	}

	bool Manager::Init(InputManager* inputManager, const Ref<Window>& window)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		io.ConfigWindowsMoveFromTitleBarOnly = true;
		io.ConfigViewportsNoAutoMerge = true;
		io.IniFilename = "./vkr_layout.ini";

		io.BackendRendererName = "VikingRenderer";
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

		const Vector2f& windowDpi = window->GetDpiScale();
		io.Fonts->AddFontFromFileTTF("../../../content/fonts/cascadia-code/CascadiaCode.ttf", 14.0f * windowDpi.x);
		io.FontDefault = io.Fonts->Fonts[0];

		SetStyle();

		m_Renderer = MakeUnique<Renderer>();
		if (!m_Renderer->Init())
		{
			return false;
		}

		m_Window = window;
		m_InputManager = inputManager;

		m_Scene = MakeUnique<Graphics::Scene>();
		m_Viewport = MakeRef<ViewportPanel>(m_Scene.get());

		{
			Render::GetDevice()->EndFrame();
			Graphics::ModelLoader_GLTF loader;
			Ref<Graphics::Model> model;
			model = loader.Load("../../../content/models/cp_noodles/scene.gltf");
			Ref<Graphics::ModelObject> modelinst = MakeRef<Graphics::ModelObject>();
			modelinst->SetLocalTransform(Compose(Mat33::Identity(), Vector3f(0.0f, 0.0f, 0.0f)));

			modelinst->SetModel(model);
			m_Scene->AddObject(modelinst);

			Render::GetDevice()->EndFrame();
		}
		return true;
	}

	void Manager::Update()
	{
		ImGuiIO& io = ImGui::GetIO();

		const Vector2u& windowSize = m_Window->GetSize();
		io.DisplaySize = ImVec2(windowSize.x, windowSize.y);
		io.DeltaTime = ElapsedTimer::DeltaTime();

		m_FpsMovingAverage.Add(static_cast<uint32_t>(std::roundf(1.0f / ElapsedTimer::DeltaTime())));
		//VKR_LOG("FPS: {}", m_FpsMovingAverage.GetAverage());

		const Vector2f& windowDpi = m_Window->GetDpiScale();
		io.DisplayFramebufferScale = ImVec2(windowDpi.x, windowDpi.y);

		if (m_InputManager)
		{
			const Vector2u mousePos = m_InputManager->GetMousePosition();
			io.MousePos.x = mousePos.x;
			io.MousePos.y = mousePos.y;
			io.MouseDown[0] = m_InputManager->IsMouseKeyPressed(INPUT_MOUSE_KEY_LEFT);
			io.MouseDown[1] = m_InputManager->IsMouseKeyPressed(INPUT_MOUSE_KEY_RIGHT);
			io.MouseDown[2] = m_InputManager->IsMouseKeyPressed(INPUT_MOUSE_KEY_MIDDLE);
			io.MouseWheel = m_InputManager->GetMouseScrollDelta();
		}

		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		m_Scene->Update();

		m_Viewport->Update();

		Draw();
	}

	void Manager::Render()
	{
		ImGui::Render();
		m_Renderer->Render();
	}

	InputManager* Manager::GetInputManager() const
	{
		return m_InputManager;
	}

	Manager* Manager::Get()
	{
		return g_Instance;
	}

	void Manager::Draw()
	{
		const bool isMaximized = false;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, isMaximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
		//windowFlags |= ImGuiWindowFlags_MenuBar;
		windowFlags |= ImGuiWindowFlags_NoDocking;
		windowFlags |= ImGuiWindowFlags_NoTitleBar;
		windowFlags |= ImGuiWindowFlags_NoCollapse;
		windowFlags |= ImGuiWindowFlags_NoResize;
		windowFlags |= ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		windowFlags |= ImGuiWindowFlags_NoNavFocus;

		ImGuiWindowClass wndClass;
		wndClass.ClassId = ImGui::GetID("_root");
		wndClass.DockingAllowUnclassed = true;
		wndClass.DockNodeFlagsOverrideSet = 0;

		ImGui::SetNextWindowClass(&wndClass);
		ImGui::Begin("_root", nullptr, windowFlags);
		ImGui::PopStyleColor(); // MenuBarBg
		ImGui::PopStyleVar(4);

		{
			ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(50, 50, 50, 255));
			// Draw window border if the window is not maximized
			//if (!isMaximized)
			//	RenderWindowOuterBorders(ImGui::GetCurrentWindow());

			ImGui::PopStyleColor(); // ImGuiCol_Border
		}

		DrawTitlebar();

		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		ImGui::DockSpace(wndClass.ClassId, ImVec2(0, 0), 0, &wndClass);
		style.WindowMinSize.x = minWinSizeX;

		// draw layout
		m_Viewport->Draw();

		ImGui::End();
	}

	void Manager::DrawTitlebar()
	{
	}

	void Manager::SetStyle()
	{
		ImGuiStyle& style = /*dst ? dst :*/ ImGui::GetStyle();

		//========================================================
		/// Colors
		//constexpr auto accent = IM_COL32(236, 158, 36, 255);
		constexpr auto highlight = IM_COL32(39, 185, 242, 255);
		//constexpr auto niceBlue = IM_COL32(83, 232, 254, 255);
		//constexpr auto compliment = IM_COL32(78, 151, 166, 255);
		constexpr auto background = IM_COL32(36, 36, 36, 255);
		constexpr auto backgroundDark = IM_COL32(26, 26, 26, 255);
		constexpr auto titlebar = IM_COL32(21, 21, 21, 255);
		constexpr auto propertyField = IM_COL32(15, 15, 15, 255);
		constexpr auto text = IM_COL32(192, 192, 192, 255);
		//constexpr auto textBrighter = IM_COL32(210, 210, 210, 255);
		//constexpr auto textDarker = IM_COL32(128, 128, 128, 255);
		//constexpr auto textError = IM_COL32(230, 51, 51, 255);
		//constexpr auto muted = IM_COL32(77, 77, 77, 255);
		constexpr auto groupHeader = IM_COL32(47, 47, 47, 255);
		//constexpr auto selection = IM_COL32(237, 192, 119, 255);
		//constexpr auto selectionMuted = IM_COL32(237, 201, 142, 23);
		constexpr auto backgroundPopup = IM_COL32(50, 50, 50, 255);
		//constexpr auto validPrefab = IM_COL32(82, 179, 222, 255);
		//constexpr auto invalidPrefab = IM_COL32(222, 43, 43, 255);
		//constexpr auto missingMesh = IM_COL32(230, 102, 76, 255);
		//constexpr auto meshNotSet = IM_COL32(250, 101, 23, 255);

		// Headers
		style.Colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(groupHeader);
		style.Colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(groupHeader);
		style.Colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(groupHeader);

		// Buttons
		style.Colors[ImGuiCol_Button] = ImColor(56, 56, 56, 200);
		style.Colors[ImGuiCol_ButtonHovered] = ImColor(70, 70, 70, 255);
		style.Colors[ImGuiCol_ButtonActive] = ImColor(56, 56, 56, 150);

		// Frame BG
		style.Colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(propertyField);
		style.Colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(propertyField);
		style.Colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(propertyField);

		// Tabs
		style.Colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(titlebar);
		style.Colors[ImGuiCol_TabHovered] = ImColor(255, 225, 135, 30);
		style.Colors[ImGuiCol_TabActive] = ImColor(255, 225, 135, 60);
		style.Colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(titlebar);
		style.Colors[ImGuiCol_TabUnfocusedActive] = style.Colors[ImGuiCol_TabHovered];

		// Title
		style.Colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(titlebar);
		style.Colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(titlebar);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Resize Grip
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

		// Scrollbar
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

		// Check Mark
		style.Colors[ImGuiCol_CheckMark] = ImColor(200, 200, 200, 255);

		// Slider
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

		// Text
		style.Colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(text);

		// Checkbox
		style.Colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(text);

		// Separator
		style.Colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(backgroundDark);
		style.Colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(highlight);
		style.Colors[ImGuiCol_SeparatorHovered] = ImColor(39, 185, 242, 150);

		// Window Background
		style.Colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(titlebar);
		style.Colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(background);
		style.Colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(backgroundPopup);
		style.Colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(backgroundDark);

		// Tables
		style.Colors[ImGuiCol_TableHeaderBg] = ImGui::ColorConvertU32ToFloat4(groupHeader);
		style.Colors[ImGuiCol_TableBorderLight] = ImGui::ColorConvertU32ToFloat4(backgroundDark);

		// Menubar
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };

		//========================================================
		/// Style
		style.FrameRounding = 2.5f;
		style.FrameBorderSize = 1.0f;
		style.IndentSpacing = 11.0f;
	}
}
#endif //ENABLE_EDITOR