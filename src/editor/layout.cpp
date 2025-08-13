#include "layout.h"

#if ENABLE_EDITOR
#include "panel.h"

namespace vkr::Editor
{
	void Layout::Update()
	{
		for (uint32_t i = 0; i < m_Panels.size(); ++i)
		{
			m_Panels[i]->Update();
		}
		OnUpdate();
	}

	void Layout::Draw()
	{
		for (uint32_t i = 0; i < m_Panels.size(); ++i)
		{
			m_Panels[i]->Draw();
		}
		OnDraw();
	}
}

#endif //ENABLE_EDITOR