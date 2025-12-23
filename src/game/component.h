#pragma once

namespace vkr::Game
{
	class IComponent
	{
	public:
		virtual void OnComponentAdded() {}
		virtual void OnComponentRemoved() {}
	};
}