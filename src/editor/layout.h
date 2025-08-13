#pragma once

#if ENABLE_EDITOR

namespace vkr::Editor
{
	class Panel;
	class Layout
	{
	public:
		Layout() = default;
		virtual ~Layout() = default;

		void Update();
		void Draw();

	protected:
		virtual void OnUpdate() {}
		virtual void OnDraw() = 0;

		std::vector<Ref<Panel>> m_Panels;
	};
}

#endif //ENABLE_EDITOR