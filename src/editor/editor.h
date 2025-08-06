#pragma once

#if ENABLE_EDITOR

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

		InputManager* GetInputManager() const;

		static Manager* Get();

	private:
		void Draw();
		void DrawTitlebar();

		void SetStyle();

		UniquePtr<Renderer> m_Renderer;
		Ref<Window> m_Window;
		InputManager* m_InputManager;

		////////////////////////////////////////////
		// TODO: not here, should go in level editor layout or something
		UniquePtr<Graphics::Scene> m_Scene;
		Ref<ViewportPanel> m_Viewport;
		////////////////////////////////////////////

		MovingAverage<uint32_t, 64> m_FpsMovingAverage;

		static Manager* g_Instance;
	};
}
#endif //ENABLE_EDITOR