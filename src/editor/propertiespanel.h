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

		Game::Entity m_SelectedEntity;
		Vector3f m_EulerRotationCache;
		Quaternion m_PrevRotation;
		bool m_EulerRotationCacheInitialized = false;
	};
}

#endif //ENABLE_EDITOR