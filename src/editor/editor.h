#pragma once

#if ENABLE_EDITOR
#include "icons.h"

namespace vkr
{
	class InputManager;
	class Window;
}

namespace vkr::Render
{
	class RenderTargetView;
}

namespace vkr::Graphics
{
	class Scene;
}

namespace vkr::Editor
{
	class ContentBrowserPanel;
	class Layout;
	class Renderer;
	class ViewportPanel;
	class Manager
	{
	public:
		Manager();
		~Manager();

		bool Init(InputManager* inputManager, const Ref<Window>& window);

		void Update();

		void Render();

		Icons* GetIcons() const;
		InputManager* GetInputManager() const;

		static Manager* Get();

	private:
		void Draw();
		void DrawTitlebar();
		void DrawTitleMenuBar();
		void DrawWindowBorders();

		void SetStyle();

		UniquePtr<Renderer> m_Renderer;
		UniquePtr<Icons> m_Icons;
		Ref<Window> m_Window;
		InputManager* m_InputManager;

		bool m_IsTitlebarHovered = false;

		Ref<Layout> m_CurrentLayout;

		MovingAverage<uint32_t, 64> m_FpsMovingAverage;

		static Manager* g_Instance;
	};
}
#endif //ENABLE_EDITOR