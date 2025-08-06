#pragma once

namespace vkr::Graphics
{
	class Scene;
	class View;
	class ViewManager
	{
	public:
		ViewManager(Scene& scene);
		~ViewManager() = default;

		View* CreateView();
		void DestroyView(View* view);

		const std::vector<View*>& GetViews() const;

	private:
		std::vector<View*> m_Views;
		Scene& m_Scene;
		uint32_t m_ViewIdCounter;
	};
}