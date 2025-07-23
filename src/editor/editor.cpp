#include "editor/editor.h"

#if ENABLE_EDITOR

#include "editorrenderer.h"
#include "render/window.h"
#include "core/timer.h"
#include "core/inputmanager.h"

#include "imgui.h"

namespace vkr::Editor
{
	Manager::Manager()
		: m_InputManager(nullptr)
	{

	}

	Manager::~Manager()
	{

	}

	bool Manager::Init(InputManager* inputManager, const Ref<Render::Window>& window)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

		const Vector2f& windowDpi = window->GetDpiScale();
		ImFontConfig fontConfig;
		fontConfig.SizePixels = 14.0f * windowDpi.x;
		io.Fonts->AddFontDefault(&fontConfig);

		m_Renderer = MakeUnique<Renderer>();
		if (!m_Renderer->Init())
		{
			return false;
		}

		m_Window = window;
		m_InputManager = inputManager;
		return true;
	}

	void Manager::Update()
	{
		ImGuiIO& io = ImGui::GetIO();

		const Vector2u& windowSize = m_Window->GetSize();
		io.DisplaySize = ImVec2(windowSize.x, windowSize.y);
		io.DeltaTime = ElapsedTimer::DeltaTime();

		const Vector2f& windowDpi = m_Window->GetDpiScale();
		io.DisplayFramebufferScale = ImVec2(windowDpi.x, windowDpi.y);

		if (m_InputManager)
		{
			const Vector2f mousePos = m_InputManager->GetMousePosition();
			io.MousePos.x = mousePos.x;
			io.MousePos.y = mousePos.y;
			io.MouseDown[0] = m_InputManager->IsPressed(MouseLeft);
			io.MouseDown[1] = m_InputManager->IsPressed(MouseRight);
			io.MouseDown[2] = m_InputManager->IsPressed(MouseMiddle);
			io.MouseWheel = m_InputManager->GetMouseScrollDelta();
		}

		ImGui::NewFrame();

		ImGui::Begin("Debug Window");
		ImGui::Text("Delta time: %.6f", ElapsedTimer::DeltaTime());
		ImGui::Text("Elapsed time: %.3f", ElapsedTimer::ElapsedTime());
		ImGui::Text("FPS: %.1f", 1.0f / ElapsedTimer::DeltaTime());
		ImGui::End();
	}

	void Manager::Draw()
	{
		ImGui::Render();
		m_Renderer->Render();
	}

	void Manager::SetOutputTarget(Render::RenderTargetView* target)
	{
		m_Renderer->SetOutputTarget(target);
	}
}
#endif //ENABLE_EDITOR