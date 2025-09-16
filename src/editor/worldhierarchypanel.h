#pragma once

#if ENABLE_EDITOR
#include "game/entity.h"
#include "panel.h"

namespace vkr::Game
{
	class World;
}

namespace vkr::Editor
{
	class WorldHierarchyPanel : public Panel
	{
	public:
		WorldHierarchyPanel(Game::World& world);
		~WorldHierarchyPanel() override;

	private:
		void OnUpdate() override;
		void OnDraw() override;

		void DrawEntityNode(const Game::Entity& entity);

		std::vector<Game::Entity> m_SelectedEntities;
		Game::World& m_World;
	};
}

#endif //ENABLE_EDITOR