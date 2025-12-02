#pragma once

#if ENABLE_EDITOR
#include "broadcast.h"
#include "game/entity.h"
#include "panel.h"

namespace vkr::Editor
{
	class PropertiesPanel : public Panel, public BroadcastListener
	{
	public:
		PropertiesPanel();
		~PropertiesPanel() override;

	private:
		void OnDraw() override;

		void ReceiveMessage(const BroadcastMessage& message) override;

		std::vector<Game::Entity> m_SelectedEntities;
	};
}

#endif //ENABLE_EDITOR