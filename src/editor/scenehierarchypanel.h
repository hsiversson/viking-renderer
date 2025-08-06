#pragma once

#if ENABLE_EDITOR
#include "panel.h"

namespace vkr::Editor
{
	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel();
		~SceneHierarchyPanel() override;

	private:
	};
}

#endif //ENABLE_EDITOR