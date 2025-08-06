#pragma once

#if ENABLE_EDITOR

namespace vkr::Editor
{
	class Panel
	{
	public:
		Panel(const char* name);
		virtual ~Panel() = default;

		void Update();
		void Draw();
				
	protected:
		virtual void OnUpdate() {}
		virtual void OnDraw() = 0;

		const char* m_Name;
		Vector2f m_ContentAreaPosition;
		Vector2f m_ContentAreaSize;

	private:
		bool m_IsOpen;
		bool m_SkipUpdate;
	};
}

#endif //ENABLE_EDITOR