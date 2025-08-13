#pragma once

#if ENABLE_EDITOR
#include "layout.h"

namespace vkr::Graphics
{
	class Scene;
}

namespace vkr::Editor
{
	class ContentBrowserPanel;
	class ViewportPanel;

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

		UniquePtr<Graphics::Scene> m_Scene;
		Ref<ViewportPanel> m_Viewport;
		Ref<ContentBrowserPanel> m_ContentBrowser;

		Mode m_Mode;
	};
}

#endif //ENABLE_EDITOR