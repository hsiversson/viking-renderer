#pragma once

#if ENABLE_EDITOR

namespace vkr
{
	class InputManager;
}

namespace vkr::Render
{
	class Window;
	class RenderTargetView;
}

namespace vkr::Editor
{
	class Renderer;
	class Manager
	{
	public:
		Manager();
		~Manager();

		bool Init(InputManager* inputManager, const Ref<Render::Window>& window);

		void Update();

		void Draw();

		void SetOutputTarget(const Ref<Render::RenderTargetView>& target);

	private:
		UniquePtr<Renderer> m_Renderer;
		Ref<Render::Window> m_Window;
		InputManager* m_InputManager;
	};
}
#endif //ENABLE_EDITOR