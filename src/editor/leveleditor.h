#pragma once

#if ENABLE_EDITOR
#include "layout.h"

namespace vkr::Game
{
	class World;
}

namespace vkr::Editor
{
	class ContentBrowserPanel;
	class PropertiesPanel;
	class ViewportPanel;
	class WorldHierarchyPanel;

	class LevelEditor final : public Layout
	{
		enum class Mode
		{
			Editing,
			Playing,
		};

	public:
		LevelEditor();
		~LevelEditor() override;

	private:
		void OnUpdate() override;
		void OnDraw() override;

		void SetMode(Mode mode);

		UniquePtr<Game::World> m_World;
		Ref<ViewportPanel> m_Viewport;
		Ref<ContentBrowserPanel> m_ContentBrowser;
		Ref<WorldHierarchyPanel> m_WorldHierarchy;
		Ref<PropertiesPanel> m_Properties;

		Mode m_Mode;
	};
}

#endif //ENABLE_EDITOR