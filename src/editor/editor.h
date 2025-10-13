#pragma once

#if ENABLE_EDITOR
#include "application/window.h"
#include "broadcast.h"
#include "icons.h"

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
	class Manager : public IMessageHandler
	{
	public:
		Manager();
		~Manager();

		bool Init(const Ref<Window>& window);

		void Update();

		void Render();

		Icons* GetIcons() const;

		void Broadcast(const BroadcastMessage& message);

		void RegisterBroadcastListener(BroadcastListener* listener);
		void UnregisterBroadcastListener(BroadcastListener* listener);

		static Manager* Get();

	private:
		void Draw();
		void DrawTitlebar();
		void DrawTitleMenuBar();
		void DrawWindowBorders();

		void SetStyle();

		void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;

		UniquePtr<Renderer> m_Renderer;
		UniquePtr<Icons> m_Icons;
		Ref<Window> m_Window;

		bool m_IsTitlebarHovered = false;

		Ref<Layout> m_CurrentLayout;

		MovingAverage<uint32_t, 64> m_FpsMovingAverage;

		std::vector<BroadcastListener*> m_BroadcastListeners;

		const std::filesystem::path m_EditorLayoutConfigPath;
		static Manager* g_Instance;
	};
}
#endif //ENABLE_EDITOR