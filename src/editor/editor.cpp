#include "editor/editor.h"

#if ENABLE_EDITOR

#include "renderer.h"
#include "panel.h"
#include "viewportpanel.h"
#include "contentbrowserpanel.h"

#include "application/window.h"
#include "application/application.h"

#include "core/timer.h"
#include "core/inputmanager.h"

#include "graphics/model.h"
#include "graphics/modelloader_gltf.h"
#include "graphics/modelobject.h"
#include "graphics/scene.h"

namespace vkr::Editor
{
	static bool BeginMenuBar(const ImRect& aMenuBarRect)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		/*if (!(window->Flags & ImGuiWindowFlags_MenuBar))
			return false;*/

		IM_ASSERT(!window->DC.MenuBarAppending);
		ImGui::BeginGroup(); // Backup position on layer 0 // FIXME: Misleading to use a group for that backup/restore
		ImGui::PushID("##menubar");

		const ImVec2 padding = window->WindowPadding;

		// We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
		// We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.

		ImRect barRect = aMenuBarRect;
		barRect.Min.y += padding.y;
		barRect.Max.y += padding.y;

		ImRect clip_rect(IM_ROUND(ImMax(window->Pos.x, barRect.Min.x + window->WindowBorderSize + window->Pos.x - 10.0f)), IM_ROUND(barRect.Min.y + window->WindowBorderSize + window->Pos.y),
			IM_ROUND(ImMax(barRect.Min.x + window->Pos.x, barRect.Max.x - ImMax(window->WindowRounding, window->WindowBorderSize))), IM_ROUND(barRect.Max.y + window->Pos.y));

		clip_rect.ClipWith(window->OuterRectClipped);
		ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

		// We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar() would need something analogous here, maybe a BeginGroupEx() with flags).
		window->DC.CursorPos = window->DC.CursorMaxPos = ImVec2(barRect.Min.x + window->Pos.x, barRect.Min.y + window->Pos.y);
		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
		window->DC.MenuBarAppending = true;
		ImGui::AlignTextToFramePadding();
		return true;
	}

	static void EndMenuBar()
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;
		ImGuiContext& g = *GImGui;

		// Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
		if (ImGui::NavMoveRequestButNoResultYet() && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) && (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
		{
			// Try to find out if the request is for one of our child menu
			ImGuiWindow* nav_earliest_child = g.NavWindow;
			while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
				nav_earliest_child = nav_earliest_child->ParentWindow;
			if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal && (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded) == 0)
			{
				// To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
				// This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth bothering)
				const ImGuiNavLayer layer = ImGuiNavLayer_Menu;
				IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer)); // Sanity check
				ImGui::FocusWindow(window);
				ImGui::SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
				g.NavCursorVisible = false; // Hide highlight for the current frame so we don't see the intermediary selection.
				g.NavHighlightItemUnderNav = g.NavMousePosDirty = true;
				ImGui::NavMoveRequestForward(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags, g.NavMoveScrollFlags); // Repeat
			}
		}

		IM_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
		// IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar);
		IM_ASSERT(window->DC.MenuBarAppending);
		ImGui::PopClipRect();
		ImGui::PopID();
		window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x; // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.
		g.GroupStack.back().EmitItem = false;
		ImGui::EndGroup(); // Restore position on layer 0
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
		window->DC.MenuBarAppending = false;
	}

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
		io.Fonts->AddFontFromFileTTF(SystemPaths::GetInContentDirectory("fonts/cascadia-code/CascadiaCode.ttf").string().c_str(), 14.0f * windowDpi.x);
		io.FontDefault = io.Fonts->Fonts[0];

		SetStyle();

		m_Renderer = MakeUnique<Renderer>();
		if (!m_Renderer->Init())
		{
			return false;
		}

		m_Icons = MakeUnique<Icons>();
		if (!m_Icons->Init())
		{
			return false;
		}

		m_Window = window;
		m_InputManager = inputManager;

		m_Window->SetIsTitlebarHoveredCallback([this](uint32_t, uint32_t) { return m_IsTitlebarHovered; });

		m_Scene = MakeUnique<Graphics::Scene>();
		m_Viewport = MakeRef<ViewportPanel>(m_Scene.get());
		m_ContentBrowser = MakeRef<ContentBrowserPanel>();

		m_InitTask = std::async([this]()
			{
				Graphics::ModelLoader_GLTF loader;
				Ref<Graphics::Model> model;
				model = loader.Load(SystemPaths::GetInContentDirectory("models/cp_noodles/scene.gltf"));
				Ref<Graphics::ModelObject> modelinst = MakeRef<Graphics::ModelObject>();
				modelinst->SetLocalTransform(Compose(Mat33::Identity(), Vector3f(0.0f, 0.0f, 0.0f)));

				modelinst->SetModel(model);
				m_Scene->AddObject(modelinst);
			});
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
			const Vector2i mousePos = m_InputManager->GetMousePosition();
			io.AddMousePosEvent(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
			io.AddMouseButtonEvent(ImGuiMouseButton_Left, m_InputManager->IsMouseKeyPressed(INPUT_MOUSE_KEY_LEFT));
			io.AddMouseButtonEvent(ImGuiMouseButton_Right, m_InputManager->IsMouseKeyPressed(INPUT_MOUSE_KEY_RIGHT));
			io.AddMouseButtonEvent(ImGuiMouseButton_Middle, m_InputManager->IsMouseKeyPressed(INPUT_MOUSE_KEY_MIDDLE));
			io.AddMouseWheelEvent(0.0f, m_InputManager->GetMouseScrollDelta());
		}

		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		m_Scene->Update();

		m_Viewport->Update();
		m_ContentBrowser->Update();

		Draw();
	}

	void Manager::Render()
	{
		ImGui::Render();
		m_Renderer->Render();
	}

	Icons* Manager::GetIcons() const
	{
		return m_Icons.get();
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
		const bool isMaximized = m_Window->IsMaximized();

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
			ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(100, 100, 100, 255));
			// Draw window border if the window is not maximized
			if (!isMaximized)
				DrawWindowBorders();
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
		m_ContentBrowser->Draw();

		ImGui::End();
	}

	void Manager::DrawTitlebar()
	{
		ImGui::PushID("_rootTitlebar");
		const bool isMaximized = m_Window->IsMaximized();
		const float titlebarVerticalOffset = isMaximized ? -6.0f : 0.0f;
		const ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;

		const float titleBarHeight = 58.0f;
		const float logoSize = 48.0f;
		const float logoPadding = 16.0f;
		const float spacing = 8.0f;
		const float buttonsWidth = titleBarHeight * 3; // 3 buttons, all square
		const float fullWidth = ImGui::GetWindowWidth();
		const float leftStart = windowPadding.x + logoPadding;
		const float rightStart = fullWidth - windowPadding.x - buttonsWidth;

		ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));
		const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
		const ImVec2 titlebarMax = { ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetCursorScreenPos().y + titleBarHeight };

		auto* bgDrawList = ImGui::GetBackgroundDrawList();
		auto* fgDrawList = ImGui::GetForegroundDrawList();
		bgDrawList->AddRectFilled(titlebarMin, titlebarMax, IM_COL32(21, 21, 21, 255));

		{
			const ImVec2 logoPos(leftStart, windowPadding.y + -6.0f);
			const ImVec2 logoMax(logoPos.x + logoSize, logoPos.y + logoSize);
			fgDrawList->AddImage((ImTextureID)m_Icons->GetIcon(EDITOR_ICON_VKR_WHITE).m_Texture.get(), logoPos, logoMax);
		}

		ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));
		float dragZoneWidth = rightStart - windowPadding.x;
		ImGui::SetNextItemAllowOverlap();
		ImGui::InvisibleButton("##titleBarDragZone", ImVec2(dragZoneWidth, titleBarHeight));
		m_IsTitlebarHovered = ImGui::IsItemHovered();

		if (isMaximized)
		{
			float windowMousePosY = ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y;
			if (windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
			{
				m_IsTitlebarHovered = true; // Account for the top-most pixels which don't register
			}
		}

		{
			float menuStartX = leftStart + logoSize + logoPadding;
			ImGui::SetCursorPos(ImVec2(menuStartX, 6.0f + titlebarVerticalOffset));
			DrawTitleMenuBar();

			if (ImGui::IsItemHovered())
			{
				m_IsTitlebarHovered = false;
			}
		}

		{
			// Centered Window title
			ImVec2 textSize = ImGui::CalcTextSize("Viking Renderer");
			float centeredX = (fullWidth - textSize.x) * 0.5f;
			float centeredY = 2.0f + windowPadding.y + 6.0f;
			ImGui::SetCursorPos(ImVec2(centeredX, centeredY));
			ImGui::TextUnformatted("Viking Renderer");
		}

		// Window buttons
		ImVec2 buttonSize = ImVec2(titleBarHeight, titleBarHeight);
		ImVec2 iconSize = ImVec2(buttonSize.x * 0.25f, buttonSize.y * 0.25f);
		ImVec2 iconOffset = ImVec2(buttonSize.x * 0.375f, buttonSize.y * 0.375f);

		float buttonY = 0.0f;
		float buttonX = rightStart;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

		// Minimize Button
		ImGui::SetCursorPos(ImVec2(buttonX, buttonY));
		if (ImGui::Button("##Minimize", buttonSize))
			m_Window->Minimize();
		ImGui::SetCursorPos(ImVec2(buttonX + iconOffset.x, buttonY + iconOffset.y));
		ImGui::Image((ImTextureID)m_Icons->GetIcon(EDITOR_ICON_MINUS_WHITE).m_Texture.get(), iconSize);
		buttonX += titleBarHeight;

		// Maximize Button
		ImGui::SetCursorPos(ImVec2(buttonX, buttonY));
		if (ImGui::Button("##Maximize", buttonSize))
			m_Window->Maximize(!isMaximized);
		Render::TextureView* minimizeIcon = m_Icons->GetIcon(EDITOR_ICON_SQUARES_WHITE).m_Texture.get();
		Render::TextureView* maximizeIcon = m_Icons->GetIcon(EDITOR_ICON_SQUARE_WHITE).m_Texture.get();
		ImGui::SetCursorPos(ImVec2(buttonX + iconOffset.x, buttonY + iconOffset.y));
		ImGui::Image((ImTextureID)(isMaximized ? minimizeIcon : maximizeIcon), iconSize);
		buttonX += titleBarHeight;

		// Close Button
		ImGui::SetCursorPos(ImVec2(buttonX, buttonY));
		if (ImGui::Button("##Close", buttonSize))
		{
			Application::RequestQuit();
		}
		ImGui::SetCursorPos(ImVec2(buttonX + iconOffset.x, buttonY + iconOffset.y));
		ImGui::Image((ImTextureID)m_Icons->GetIcon(EDITOR_ICON_CROSS_WHITE).m_Texture.get(), iconSize);

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
		ImGui::SetCursorPosY(titleBarHeight);
		ImGui::PopID();
	}

	void Manager::DrawTitleMenuBar()
	{
		const ImRect menuBarRect = { ImGui::GetCursorPos(), { ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing() } };
		ImGui::BeginGroup();

		if (BeginMenuBar(menuBarRect))
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Window"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Settings"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem("About"))
				{
					//exampleLayer->ShowAboutModal();
				}
				ImGui::EndMenu();
			}
		}

		EndMenuBar();
		ImGui::EndGroup();
	}

	void Manager::DrawWindowBorders()
	{
		struct ResizeBorderDef
		{
			ImVec2 InnerDir;
			ImVec2 SegmentN1;
			ImVec2 SegmentN2;
			float OuterAngle;
		};

		static const ResizeBorderDef resizeBorderDef[4] =
		{
			{ ImVec2(+1, 0), ImVec2(0, 1), ImVec2(0, 0), PI * 1.00f }, // Left
			{ ImVec2(-1, 0), ImVec2(1, 0), ImVec2(1, 1), PI * 0.00f }, // Right
			{ ImVec2(0, +1), ImVec2(0, 0), ImVec2(1, 0), PI * 1.50f }, // Up
			{ ImVec2(0, -1), ImVec2(1, 1), ImVec2(0, 1), PI * 0.50f }  // Down
		};

		auto GetResizeBorderRect = [](ImRect windowRect, int border_n, float perp_padding, float thickness)
			{
				if (thickness == 0.0f)
				{
					windowRect.Max.x -= 1;
					windowRect.Max.y -= 1;
				}
				if (border_n == ImGuiDir_Left) 
				{ 
					return ImRect(windowRect.Min.x - thickness, windowRect.Min.y + perp_padding, windowRect.Min.x + thickness, windowRect.Max.y - perp_padding);
				}
				if (border_n == ImGuiDir_Right) 
				{ 
					return ImRect(windowRect.Max.x - thickness, windowRect.Min.y + perp_padding, windowRect.Max.x + thickness, windowRect.Max.y - perp_padding);
				}
				if (border_n == ImGuiDir_Up) 
				{ 
					return ImRect(windowRect.Min.x + perp_padding, windowRect.Min.y - thickness, windowRect.Max.x - perp_padding, windowRect.Min.y + thickness);
				}
				if (border_n == ImGuiDir_Down) 
				{ 
					return ImRect(windowRect.Min.x + perp_padding, windowRect.Max.y - thickness, windowRect.Max.x - perp_padding, windowRect.Max.y + thickness);
				}
				assert(false);
				return ImRect();
			};

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImGuiStyle& style = ImGui::GetStyle();
		float rounding = window->WindowRounding;
		float border_size = window->WindowBorderSize;
		if (border_size > 0.0f && !(window->Flags & ImGuiWindowFlags_NoBackground))
			window->DrawList->AddRect(window->Pos, { window->Pos.x + window->Size.x,  window->Pos.y + window->Size.y }, ImGui::GetColorU32(ImGuiCol_Border), rounding, 0, border_size);

		int border_held = window->ResizeBorderHeld;
		if (border_held != -1)
		{
			const ResizeBorderDef& def = resizeBorderDef[border_held];
			ImRect border_r = GetResizeBorderRect(window->Rect(), border_held, rounding, 1.0f);
			ImVec2 p1 = ImLerp(border_r.Min, border_r.Max, def.SegmentN1);
			const float offsetX = def.InnerDir.x * rounding;
			const float offsetY = def.InnerDir.y * rounding;
			p1.x += 0.5f + offsetX;
			p1.y += 0.5f + offsetY;

			ImVec2 p2 = ImLerp(border_r.Min, border_r.Max, def.SegmentN2);
			p2.x += 0.5f + offsetX;
			p2.y += 0.5f + offsetY;

			window->DrawList->PathArcTo(p1, rounding, def.OuterAngle - PI * 0.25f, def.OuterAngle);
			window->DrawList->PathArcTo(p2, rounding, def.OuterAngle, def.OuterAngle + PI * 0.25f);
			window->DrawList->PathStroke(ImGui::GetColorU32(ImGuiCol_SeparatorActive), 0, std::max(2.0f, border_size)); // Thicker than usual
		}
		if (style.FrameBorderSize > 0 && !(window->Flags & ImGuiWindowFlags_NoTitleBar) && !window->DockIsActive)
		{
			float y = window->Pos.y + window->TitleBarHeight - 1;
			window->DrawList->AddLine(ImVec2(window->Pos.x + border_size, y), ImVec2(window->Pos.x + window->Size.x - border_size, y), ImGui::GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
		}
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