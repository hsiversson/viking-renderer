#pragma once

#if ENABLE_EDITOR
namespace vkr::Editor
{
	class Manager
	{
	public:
		Manager();
		~Manager();

		void Update();

		void Draw();

	private:
	};
}
#endif //ENABLE_EDITOR