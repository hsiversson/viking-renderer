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

namespace vkr::Editor
{
	class Renderer;
	class Manager
	{
	public:
		Manager();
		~Manager();

		bool Init(InputManager* inputManager, const Ref<Window>& window);

		void Update();

		void Draw();

		void SetOutputTarget(Render::RenderTargetView* target);

	private:
		UniquePtr<Renderer> m_Renderer;
		Ref<Window> m_Window;
		InputManager* m_InputManager;

		MovingAverage<uint32_t, 64> m_FpsMovingAverage;
	};
}
#endif //ENABLE_EDITOR