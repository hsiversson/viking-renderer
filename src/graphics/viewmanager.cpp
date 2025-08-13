#include "viewmanager.h"
#include "view.h"

namespace vkr::Graphics
{
	ViewManager::ViewManager(Scene& scene)
		: m_Scene(scene)
		, m_ViewIdCounter(0)
	{
	}

	View* ViewManager::CreateView()
	{
		VKR_ASSERT(m_ViewIdCounter < UINT32_MAX);
		m_Views.push_back(new View());
		return m_Views.back();
	}

	void ViewManager::DestroyView(View* view)
	{
		const auto& it = std::find(m_Views.begin(), m_Views.end(), view);
		if (it != m_Views.end())
		{
			m_Views.erase(it);
		}
		else
		{
			VKR_ASSERT(false && "View was not associated with this View Manager.");
		}
	}

	const std::vector<View*>& ViewManager::GetViews() const
	{
		return m_Views;
	}
}