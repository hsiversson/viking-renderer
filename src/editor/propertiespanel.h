#pragma once

#if ENABLE_EDITOR
#include "game/entity.h"
#include "panel.h"

namespace vkr::Editor
{
	class PropertiesPanel : public Panel
	{
	public:
		PropertiesPanel();
		~PropertiesPanel() override;

		void SetSelected(Game::Entity e);

	private:
		void OnDraw() override;

		Game::Entity m_SelectedEntity;
	};
}

#endif //ENABLE_EDITOR