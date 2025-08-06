#include "panel.h"

#if ENABLE_EDITOR

namespace vkr::Editor
{
	Panel::Panel(const char* name)
		: m_Name(name)
		, m_IsOpen(true)
		, m_SkipUpdate(false)
		, m_ContentAreaSize(100, 100)
	{
	}

	void Panel::Update()
	{
		if (!m_SkipUpdate)
			OnUpdate();
	}

	void Panel::Draw()
	{
		if (ImGui::Begin(m_Name, &m_IsOpen, 0))
		{
			const ImVec2 windowPos = ImGui::GetWindowPos();
			const ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
			const ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();
			m_ContentAreaSize.x = contentRegionMax.x - contentRegionMin.x;
			m_ContentAreaSize.y = contentRegionMax.y - contentRegionMin.y;
			m_ContentAreaPosition.x = windowPos.x + contentRegionMin.x;
			m_ContentAreaPosition.y = windowPos.y + contentRegionMin.y;

			OnDraw();
		}
		ImGui::End();

		m_SkipUpdate = !m_IsOpen || m_ContentAreaSize.x <= 0.0f || m_ContentAreaSize.y <= 0.0f;
	}
}

#endif //ENABLE_EDITOR